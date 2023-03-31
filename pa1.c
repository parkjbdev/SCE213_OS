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
  char *string;
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
    char* string = concat_strings(argc, argv);

    for (int i = 0;i < alias_cnt;i++) {
      if (strcmp(aliases[i].alias, alias) == 0) {
        free(aliases[i].string);
        aliases[i].string = string;

        return 1;
      }
    }

    aliases = (Alias *)realloc(aliases, sizeof(Alias) * (alias_cnt + 1));
    aliases[alias_cnt].alias = (char *)malloc(sizeof(char) * strlen(alias) + 1);
    aliases[alias_cnt].string = (char *)malloc(sizeof(char) * strlen(string) + 1);

    strcpy(aliases[alias_cnt].alias, alias);
    strcpy(aliases[alias_cnt].string, string);

    alias_cnt++;

    return 1;
}

int builtin_alias_get() {
  for (int i = 0;i < alias_cnt;i++) {
    fprintf(stderr, "%s %s\n", aliases[i].alias, aliases[i].string);
  }
  return 1;
}

int run_command(int nr_tokens, char *tokens[])
{
	if (strcmp(tokens[0], "exit") == 0) return 0;
  if (strcmp(tokens[0], "cd") == 0) {
    if (nr_tokens == 1) return builtin_cd("~");
    else if (nr_tokens == 2) return builtin_cd(tokens[1]);
    else return -1;
  }
  if (strcmp(tokens[0], "alias") == 0) {
    if (nr_tokens == 1) return builtin_alias_get();
    else if (nr_tokens > 2) return builtin_alias_add(tokens[1], nr_tokens - 2, tokens + 2); 
    else return -1;
  }

    int pid = fork();
    if (pid < 0) return -1;
    else if (pid == 0) {
        if (execvp(tokens[0], tokens) < 0){
            fprintf(stderr, "Unable to execute %s\n", tokens[0]);
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
