/**********************************************************************
 * Copyright (c) 2020-2023
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#define MAX_BUFFER 1024

/***********************************************************************
 * builtin_alias
 */

typedef struct _alias {
    char *alias;
    int nr_tokens;
    char **tokens;
} Alias;

int alias_cnt = 0;
Alias **aliases = NULL;

Alias *new_alias(char *alias, int argc, char *argv[]) {
    Alias *new = (Alias *) malloc(sizeof(Alias));

    new->alias = strdup(alias);
    new->nr_tokens = argc;
    new->tokens = (char **) malloc(sizeof(char *) * argc);
    for (int i = 0; i < argc; i++) new->tokens[i] = strdup(argv[i]);

    return new;
}

void delete_alias(Alias *target) {
    for (int i = 0; i < target->nr_tokens; i++) {
        free(target->tokens[i]);
    }
    free(target->tokens);
    free(target->alias);
    free(target);
}

Alias *find_alias(char *alias) {
    for (int i = 0; i < alias_cnt; i++) {
        if (strcmp(aliases[i]->alias, alias) == 0) {
            return aliases[i];
        }
    }
    return NULL;
}

int builtin_alias_add(char *alias, int argc, char *argv[]) {
    Alias *found = find_alias(alias);

    if (found == NULL) {
        aliases = (Alias **) realloc(aliases, sizeof(Alias *) * (alias_cnt + 1));
        aliases[alias_cnt++] = new_alias(alias, argc, argv);
        return 1;
    }

    for (int i = 0; i < found->nr_tokens; i++) {
        free(found->tokens[i]);
    }
    found->nr_tokens = argc;
    found->tokens = (char **) realloc(found->tokens, sizeof(char *) * argc);
    for (int i = 0; i < argc; i++) found->tokens[i] = strdup(argv[i]);

    return 1;
}

void print_alias(Alias *alias) {
    fprintf(stderr, "%s:", alias->alias);
    for (int i = 0; i < alias->nr_tokens; i++) {
        fprintf(stderr, " %s", alias->tokens[i]);
    }
    fprintf(stderr, "\n");
}

int builtin_alias_print(char *alias) {
    if (alias == NULL) {
        if (alias_cnt == 0) return 0;
        for (int i = 0; i < alias_cnt; i++) {
            print_alias(aliases[i]);
        }
        return 1;
    } else {
        Alias *found = find_alias(alias);
        if (found == NULL) return -1;
        print_alias(found);
        return 1;
    }
}


/***********************************************************************
 * builtin_cd()
 * @details change directory, set OLDPWD
 * @param dir ~ to home directory, - to OLDPWD
 * @return 1 if succeed -1 if failed
 */

int builtin_cd(char *dir) {
    if (strcmp(dir, "~") == 0) {
        dir = getenv("HOME");
    } else if (strcmp(dir, "-") == 0) {
        dir = getenv("OLDPWD");
    }

    char old_pwd[255];
    getcwd(old_pwd, 255);

    int result = chdir(dir) == 0 ? 1 : -1;
    if (result < 0) perror("cd");
    else {
        char pwd[MAX_BUFFER];
        getcwd(pwd, MAX_BUFFER);
        setenv("PWD", pwd, 1);
        setenv("OLDPWD", old_pwd, 1);
    }

    return result;
}

int count_pipelines(int nr_tokens, char *tokens[]) {
    int cnt = 0;
    for (int i = 0; i < nr_tokens; i++)
        if (strcmp(tokens[i], "|") == 0) cnt++;

    return cnt;
}

typedef struct command {
    char **tokens;
    int nr_tokens;
} Command;

typedef struct commands {
    Command **list;
    int nr_commands;
} Commands;

Commands *parse_commands(int nr_tokens, char *tokens[]) {
    Commands *commands = (Commands *) malloc(sizeof(Commands));

    commands->nr_commands = count_pipelines(nr_tokens, tokens) + 1;
    commands->list = (Command **) malloc(sizeof(Command *) * commands->nr_commands);

    int command_idx = 0;
    int pipe_idx1 = 0;
    int pipe_idx2 = 0;

    for (int i = 0; i < nr_tokens; i++) {
        if (!strcmp(tokens[i], "|") || i == nr_tokens - 1) {
            pipe_idx1 = pipe_idx2 == 0 ? 0 : pipe_idx2 + 1;
            pipe_idx2 = i != nr_tokens - 1 ? i : nr_tokens;

            commands->list[command_idx] = (Command *) malloc(sizeof(Command));
            commands->list[command_idx]->tokens = (char **) malloc(sizeof(char *) * (pipe_idx2 - pipe_idx1 + 1));
            commands->list[command_idx]->nr_tokens = pipe_idx2 - pipe_idx1;
            for (int j = 0; j < pipe_idx2 - pipe_idx1; j++) {
                commands->list[command_idx]->tokens[j] = tokens[j + pipe_idx1];
            }

            commands->list[command_idx]->tokens[pipe_idx2 - pipe_idx1] = NULL;
            command_idx++;
        }
    }

    return commands;
}

void delete_commands(Commands *target) {
    for (int i = 0; i < target->nr_commands; i++) {
        free(target->list[i]->tokens);
        free(target->list[i]);
    }
    free(target->list);
    free(target);
}

int exec_command(Commands *commands, int idx) {
    Command *command = commands->list[idx];

    if (execvp(command->tokens[0], command->tokens) < 0) {
        fprintf(stderr, "Unable to execute %s\n", command->tokens[0]);
        delete_commands(commands);
        exit(EXIT_FAILURE);
    } else {
        delete_commands(commands);
        exit(EXIT_SUCCESS);
    }
}

int execute(int nr_tokens, char *tokens[]) {
    Commands *commands = parse_commands(nr_tokens, tokens);

    pid_t pid;
    int pipe_fd[2];

    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < commands->nr_commands - 1; i++) {
        pid = fork();
        if (pid < 0) {
            delete_commands(commands);
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (i != 0)
                dup2(pipe_fd[0], STDIN_FILENO);
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[0]); // EOF of pipe read_end

            exec_command(commands, i);
        } else {
            wait(NULL);
            close(pipe_fd[1]); // EOF of pipe write_end
        }
    }

    pid = fork();

    if (pid < 0) {
        delete_commands(commands);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (commands->nr_commands > 1) {
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]); // EOF of pipe read_end
        }
        exec_command(commands, commands->nr_commands - 1);
    } else {
        wait(NULL);
        delete_commands(commands);
        return 1;
    }

    return -1;
}


/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

int run_command(int nr_tokens, char *tokens[]) {
    if (nr_tokens == 0) return 1;
    if (strcmp(tokens[0], "exit") == 0) return 0;
    if (strcmp(tokens[0], "cd") == 0) {
        if (nr_tokens == 1) return builtin_cd("~");
        else if (nr_tokens == 2) return builtin_cd(tokens[1]);
        else return -1;
    }
    if (strcmp(tokens[0], "alias") == 0) {
        if (nr_tokens == 1) return builtin_alias_print(NULL);
        else if (nr_tokens == 2) return builtin_alias_print(tokens[1]);
        else if (nr_tokens > 2) return builtin_alias_add(tokens[1], nr_tokens - 2, tokens + 2);
        else return -1;
    }

    // Replace Alias
    for (int i = 0; i < nr_tokens; i++) {
        Alias *alias = find_alias(tokens[i]);
        if (alias == NULL) continue;
        nr_tokens += alias->nr_tokens - 1;
        for (int j = nr_tokens; j > i + alias->nr_tokens - 1; j--) {
            tokens[j] = tokens[j - alias->nr_tokens + 1];
        }
        for (int j = i; j < i + alias->nr_tokens; j++) {
            tokens[j] = strdup(alias->tokens[j - i]);
        }
        i += alias->nr_tokens - 1;
    }

    return execute(nr_tokens, tokens);
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
int initialize(int argc, char *const argv[]) {
    return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
void finalize(int argc, char *const argv[]) {
    // Free Aliases
    for (int i = 0; i < alias_cnt; i++) {
        delete_alias(aliases[i]);
    }
    alias_cnt = 0;
    free(aliases);
}
