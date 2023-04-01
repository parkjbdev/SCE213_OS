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
#include <stdbool.h>

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
int builtin_cd(char *dir) {
    if (strcmp(dir, "~") == 0) {
      dir = getenv("HOME");
    } else if (strcmp(dir, "-") == 0) {
      dir = getenv("OLDPWD");
    }

    int result = chdir(dir) == 0 ? 1 : -1;
    if (result < 0) perror("cd");
    else setenv("OLDPWD", getenv("PWD"), 0);

    return result;
}

typedef struct _alias {
  char *alias;
  int nr_tokens;
  char **tokens;
} Alias;

int alias_cnt = 0;
Alias *aliases = NULL;

char* concat_strings(int argc, char* argv[]) {
  int len = 0;
  for (int i = 0;i < argc;i++) 
    len += strlen(argv[i]) + 1;
  char *result = (char *)malloc(sizeof(char) * len);
  strcpy(result, argv[0]);
  for (int i = 1;i < argc;i++) {
    strcat(result, " ");
    strcat(result, argv[i]);
  }
  return result;
}

int builtin_alias_add(char *alias, int argc, char *argv[]) {
    for (int i = 0;i < alias_cnt;i++) {
      if (strcmp(aliases[i].alias, alias) == 0) {
        for (int j = 0;j < aliases[i].nr_tokens;j++)
          free(aliases[i].tokens[j]);
        aliases[i].nr_tokens = argc;
        aliases[i].tokens = (char **)realloc(aliases[i].tokens, sizeof(char *) * argc);
        for(int j = 0;j < argc;j++) {
          aliases[i].tokens[j] = (char *)malloc(sizeof(char) * (strlen(argv[j]) + 1));
          strcpy(aliases[i].tokens[j], argv[j]);
        }

        return 1;
      }
    }

    aliases = (Alias *)realloc(aliases, sizeof(Alias) * (alias_cnt + 1));
    aliases[alias_cnt].alias = (char *)malloc(sizeof(char) * (strlen(alias) + 1));
    strcpy(aliases[alias_cnt].alias, alias);
    aliases[alias_cnt].nr_tokens = argc;
    aliases[alias_cnt].tokens = (char **)malloc(sizeof(char *) * argc);
    for(int i = 0;i < argc;i++) {
      aliases[alias_cnt].tokens[i] = (char *)malloc(sizeof(char) * (strlen(argv[i]) + 1));
      strcpy(aliases[alias_cnt].tokens[i], argv[i]);
    }

    alias_cnt++;

    return 1;
}

int builtin_alias_print() {
  for (int i = 0;i < alias_cnt;i++) {
    fprintf(stderr, "%s:", aliases[i].alias);
    for (int j = 0;j < aliases[i].nr_tokens;j++) {
      fprintf(stderr, " %s", aliases[i].tokens[j]);
    }
    fprintf(stderr, "\n");
  }
  return 1;
}

Alias *find_alias(char *alias) {
  for (int i = 0;i < alias_cnt;i++) {
    if (strcmp(aliases[i].alias, alias) == 0) {
      return &aliases[i];
    }
  }
  return NULL;
}

int run_command(int nr_tokens, char *tokens[])
{
  if (nr_tokens == 0) return 1;
	if (strcmp(tokens[0], "exit") == 0) return 0;
  if (strcmp(tokens[0], "cd") == 0) {
    if (nr_tokens == 1) return builtin_cd("~");
    else if (nr_tokens == 2) return builtin_cd(tokens[1]);
    else return -1;
  }
  if (strcmp(tokens[0], "alias") == 0) {
    if (nr_tokens == 1) return builtin_alias_print();
    else if (nr_tokens > 2) return builtin_alias_add(tokens[1], nr_tokens - 2, tokens + 2); 
    else return -1;
  }

  // replace alias
  int new_nr_tokens = nr_tokens;
  char **new_tokens = (char **)malloc(sizeof(char *) * new_nr_tokens);
  for (int i = 0;i < nr_tokens;i++) {
    new_tokens[i] = (char *)malloc(sizeof(char) * (strlen(tokens[i]) + 1));
    strcpy(new_tokens[i], tokens[i]);
  }

  for (int i = 0;i < new_nr_tokens;i++) {
    Alias *alias = find_alias(tokens[i]);

    if (alias == NULL) continue;

    new_nr_tokens = new_nr_tokens - 1 + alias->nr_tokens;
    new_tokens = (char **)realloc(new_tokens, sizeof(char *) * new_nr_tokens);

    for (int j = i + alias->nr_tokens;j < new_nr_tokens;j++) {
      new_tokens[j] = (char *)malloc(sizeof(char) * (strlen(tokens[j - alias->nr_tokens + 1]) + 1));
      strcpy(new_tokens[j], tokens[j - alias->nr_tokens + 1]);
    }

    for (int j = i;j < i + alias->nr_tokens;j++) {
      new_tokens[j] = (char *)malloc(sizeof(char) * (strlen(alias->tokens[j - i]) + 1));
      strcpy(new_tokens[j], alias->tokens[j - i]);
    }

    i = i + alias->nr_tokens - 1;
  }

  for (int i = 0;i < new_nr_tokens;i++) {
    fprintf(stderr, "%s \n", new_tokens[i]);
  }

    int pid = fork();
    if (pid < 0) return -1;
    else if (pid == 0) {
        if (execvp(new_tokens[0], new_tokens) < 0){
            fprintf(stderr, "Unable to execute %s\n", new_tokens[0]);
            exit(1);
        } else exit(0);
    } else {
        wait(0);
        return 1;
    }
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
int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
void finalize(int argc, char * const argv[])
{
}
