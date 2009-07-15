/*
 * \file rline.c
 * \date 2007
 *
 * Implementation of TAB completion and history using lib readline.
 */
/*  Copyright (C) 2007  Martin Zibricky, Martin Satke, Zdenek Straka
*/



#include	<dirent.h>
#include	<string.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/errno.h>

#include 	<readline/readline.h>
#include	<readline/history.h>

#include    "rline.h"

#define SHELL_CMD_CNT	6

char *shell_commands[] = { "cd", "exit", "history","quit","set" ,"help"};

/* All possible commands for TAB completion. */
char **commands;

/* Generator function for command completion.  STATE lets us
   know whether to start from scratch; without any state
   (i.e. STATE == 0), then we start at the top of the list. */
char *command_generator (const char *text, int state)
{
	static int list_index, len;
	char *name;

	/* If this is a new word to complete, initialize now.  This
	 * includes saving the length of TEXT for efficiency, and
	 * initializing the index variable to 0. */
	if (!state)
	{
		list_index = 0;
		len = strlen (text);
	}

	/* Return the next name which partially matches from the
	 * command list. */
	while ((name = commands[list_index]))
	{
		list_index++;

		if (strncmp (name, text, len) == 0)
			return (strdup(name));
	}

	/* No names matched. */
	return NULL;
}

/* Attempt to complete on the contents of TEXT.  START and END
   bound the region of rl_line_buffer that contains the word to
   complete.  TEXT is the word to complete.  We can use the entire
   contents of rl_line_buffer in case we want to do some simple
   parsing.  Return the array of matches, or NULL if there aren't any. */
char **tab_completion (const char *text, int start, int end)
{
	char **matches;

	matches = (char **)NULL;

	/* If this word is at the start of the line, then it is a command
	 * to complete.  Otherwise it is the name of a file in the current
	 * directory. */
	if (start == 0)
		matches = rl_completion_matches (text, command_generator);

	return (matches);
}

/* Filter for directory entries. */
static int one (const struct dirent *unused)
{
	return 1;
}

/* Init list with commands for completion and tell libreadline how to complete. */
void initialize_readline ()
{
	/* Add completion for all commands from /bin and /usr/bin. */

	struct dirent **bin, **usr_bin;
	int b, u;

	b = scandir ("/bin", &bin, one, alphasort);
	u = scandir ("/usr/bin", &usr_bin, one, alphasort);

	if (b >= 0 && u >= 0)
	{
		int cnt = SHELL_CMD_CNT + b + u + 1;	/* NULL terminated array. */
		commands = (char **) xmalloc ((size_t) cnt * sizeof (void *));
		/* Shell cmds. */
		int i, limit;
		limit = SHELL_CMD_CNT;
		for (i = 0; i < limit; i++)
			commands[i] = shell_commands[i];
		/* /bin */
		int j = 0;
		limit += b;
		for (i = SHELL_CMD_CNT; i < limit; i++, j++)
			commands[i] = bin[j]->d_name;
		/* /usr/bin */
		j = 0;
		limit += u;
		for (i = SHELL_CMD_CNT + b; i < limit; i++, j++)
			commands[i] = usr_bin[j]->d_name;
		/* NULL terminated array */
		commands[i] = NULL;
	}
	else
	{
		perror ("Couldn't open the directory");
		exit (EXIT_FAILURE);
	}

	/* Register for libreadline function for completion. */
	rl_attempted_completion_function = tab_completion;
}
