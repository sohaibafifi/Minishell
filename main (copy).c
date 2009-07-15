/* Nick Bargnesi
 * smallshell
 * Copyright (C) 2002  Nick Bargnesi
 *	smallshell is distributed under the terms of the GPL.
 *	Please see file GPL.
 *	maitre@attbi.com
 *
 * NOTES: No known bugs / Limited Functionality
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <regex.h>


#define EOL		1
#define ARG		2
#define AMPERSAND	3
#define SEMICOLON	4
#define MAXARG		512
#define MAXBUF		512
#define FOREGROUND	0
#define BACKGROUND	1
#define true		1
#define false		0

static char *dir;
static char *user;
static char *host;
static char *at = "@";
static char *space = " ";
static char *mode = "$" ;
char *onearg;
static char *prompt;

/* decompose takes an input array argument and chunks it into an array */
void decompose(char breakthis[][MAXARG], char input[]);
/* void executer will take three parameters, the command, arguments,
*  and a special character, executing and waiting or returning */
void executer(char in1[MAXBUF], char args[MAXARG], char special);
int is_special_char(char special);
void updateprompt(char *newdir);
void setvars();
int valide(char input[]);
pid_t forked;
void print_r(char array[][MAXBUF])
{
    int i;
    for (i=0 ; i<sizeof(array);i++)
        printf("[%d]->%s\n",i,array[i]);

}
void main()
{
	setvars();
	char input[MAXBUF], ch; // these are the vars for the input line and loop
	int i = 0; // the CIS lab compiler won't let creations happen
				// below where they were before
	int loopout = 0, loopnest = 0, loopclear = 0;
	char broken[MAXARG][MAXBUF]; int switchthis = 0;
	//char in1[MAXARG],in2[MAXARG]; // parsing the array vars

	//printf("allocate memory for the prompt using the length of the environ vars \n");
	prompt = (char*)calloc(100000,sizeof(char)); // 128 characters max directory depth
	//printf("Update prompt to %s \n",dir);

	updateprompt(dir);
	printf(" .... ok\n");
	printf("%s", prompt);

	//printf(" command line input loop...\n");
	while ((ch = getchar()) != '\n')
		input[i++] = ch;
	input[i] = '\0'; i = 0;

	decompose(broken, input); // returns an array of broken strings

	while ((strncmp(input,"exit", 4))!=0)//strncmp returns 0 if equal
	{
		char cmdliteral[MAXBUF];
		char cmdarg[MAXARG];
		int y = 0, mark = 0, m = 0;

		while (broken[m][0] != '\0')
		{
			/* clear cmd arrays */
			for (loopclear = 0; loopclear < MAXARG; loopclear++)
			{
				cmdliteral[loopclear] = '\0';
				cmdarg[loopclear] = '\0';
			}

			while (broken[m][y] != ' ' && broken[m][y] != '\0')
			{
				if ( broken[m][y] != '\0')
				{
					// copy into cmdliteral
					cmdliteral[y] = broken[m][y];
					y++;
				}
			}
			cmdliteral[y] = '\0';
			mark = y;
			y = 0; // reset marker for cmdarg array

			//grab the rest and treat as args...
			while (broken[m][mark] != '\0')
			{
				if (broken[m][mark] == ' ' && switchthis == 0)
				{
					broken[m][mark] = '.'; // so we know to never come back!
					mark++;
					switchthis = 1;
				}
				cmdarg[y] = broken[m][mark];
				mark++;
				y++;
			}

			/* we now have two char arrays, one the literal, one the args,
			   and we special character in addition */

			m++;
			y = 0;
			/* now our position moves down and marker is at zero */
			if (is_special_char(broken[m][y]) == 1)
			{
				executer(cmdliteral, cmdarg, broken[m][y]);

			}
			else
			{
				printf("in else\n");
				//execute & come back

				executer(cmdliteral, cmdarg, '\0');
			}
			m = m + 1;

		}

		printf("%s", prompt);
		/* we now loop taking care of command line input... */
		while ((ch = getchar()) != '\n')
			input[i++] = ch;
		input[i] = '\0'; i = 0;
		for ( loopout = 0; loopout < MAXBUF; loopout++)
		{
			for ( loopnest = 0; loopnest < MAXARG; loopnest++)
			{
				broken[loopout][loopnest] = '\0';
			}
		}
		decompose(broken, input); // rebreak string
		switchthis = 0;
		printf("\nvalide = %d\n",valide(input));

	}
	free((void *)prompt); /* free the callocs */

	//return 0;
}
int valide(char input[]){
    char *pattern = "*\\w+[\\w.-]*( (-(\\w+|-\\w+)( \"[ \'\\w]*\"| \'[ \"\\w]*\'| +\\w*|) +)*(-(\\w+|-\\w+)( \"[ \'\\w]*\"| \'[ \"\\w]*\'| +\\w*|))( \"[ \'\\w]*\"| \'[ \"\\w]*\'| +\\w*|)*| *)";
    //(-(\w|-\w+)( "[ '\w]*"| '[ "\w]*'| \w*|) )*(-(\w|-\w+)( "[ '\w]*"| '["\w]*'| \w*|))( "[ '\w]*"| '[ "\w]*'| \w*|)*
    int status;
    regex_t re;

    if(regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0)
        return false;
    status = regexec(&re, input, (size_t)0, NULL, 0);
    regfree(&re);
    if(status != 0) return false;
    return true;

    }



void decompose(char breakthis[][MAXARG], char input[])
{
	/* Function Definition**************************************************
	* Iterate through the input array provided until we reach the null \0  *
	* character.  On each iteration, perform the appropriate operation     *
	* concerning the new array. For example, if we had the command line ls *
	* & ls we would like to do some type of an exec and nowait and then an *
	* exec and wait so copy ls into cell 0, & into cell 1, and ls into     *
	* cell 2, to be called later from the calling function.                *
	***********************************************************************/
	int loopout = 0, loopnest = 0;
	/* clear array breakthis */
	for ( loopout = 0; loopout < MAXBUF; loopout++)
	{
		for ( loopnest = 0; loopnest < MAXARG; loopnest++)
		{
			breakthis[loopout][loopnest] = '\0';
		}
	}

	// NOTE: A drawback of passing our multidimensional array here is that
	// we must guarantee to the compiler that the rightmost size is defined:
	// hence MAXARG.
	int j = 0, k = 0, l = 0;  // outer loop var, and one for each dimension

	while (input[j] != '\0')
	{

		if (is_special_char(input[j]) == 0)
		{
			if (l == 0 && input[j] == ' ')
			{
				//found space - do nothing
			}
			else
			{
				// then its not a special character... copy into new array
				breakthis[k][l] = input[j];
				//printf("Character: %c: copied into breakthis %d, %d.\n",
					//breakthis[k][l], k, l);
				l++;
			}
		}

		else
		{
			// its a special char so increment k to move down in multi array
			// add the character, terminate the string, then increment k
			// again and also make sure l = 0, otherwise we don't know where
			// the its going in the second dim. array printf("current k is
			// %d\n", k);  but before we move vertically in double array,
			// terminate last string
			breakthis[k][l-1] = '\0';
			k++;
			l = 0;
			breakthis[k][l] = input[j];
			l++;
			breakthis[k][l] = '\0';
			l = 0; // move down and reset marker l
			k++;

		}

		j++;

	}

	k = 0; l = 0;
	while (breakthis[k][0] != '\0')
	{
		k++;
	}

}

/* Define the characters we consider special to the input line */
int is_special_char(char special)
{
	return ((special == '&')|(special == ';')|(special == '|')|(special == '<')|(special == '>')|(special == '\0'));
}

void executer(char in1[MAXBUF], char args[MAXARG], char special)
{
	int index_cmdarray = 0, index_argarray = 0, singleargout = 0, singleargin = 0, flag1 = 0;
	char singlearg[MAXARG][MAXBUF];
	if (args[0] == '\0')
	{
		char * cmd[] = {in1, (char *)0 };
		int forked = 0, exec_return = 0;
		if ((strncmp(in1,"cd", 2))==0) //strncmp returns 0 if equal
		{
			dir = getenv("HOME");
			if (chdir(dir) == 0)
			{
				dir = getcwd(NULL, 0);
				updateprompt(dir);
			}
			else
				printf("smallshell: directory change failed.\n");
			return;
		}

		forked = fork();
		if (forked < 0)
			perror("smallshell");
		if (forked == 0)
		{
			/* we're in the child process */
			exec_return = execvp(in1, cmd);
			if (exec_return == -1) //linux hard error
			{
				printf("smallshell: %s: command not found.\n", in1);
				exit(0);
			}
		}
		else
		{
			/* we're in the parent process */
			if (special == ';')
			{
				waitpid(forked, NULL, 0);
			}
			else if (special == '&')
			{
				return;
			}
			else if (special == '\0')
			{
				/* perform default action and wait until its finished
				*  then return and print prompt */
				waitpid(forked, NULL, 0);
			}
		}
	}
	else
	{
		/* the command has arguments */
		/*
			Implementation of multiple arguments: create our array of pointers
		to strings specifying maxarg size.  Add the command to execute to
		the first position at index 0. Loop until args at index == null or
		until we've reached the maximum amount of arguments allowable.  At
		each iteration, if the character to be copied is not a space, copy
		it into our singleargument array, increment the index of singlearg,
		increment the index of argarray and loop.
		If the character is a space, terminate singlearg with a null, point
		the index we're at in cmd to singlearg, increment both arrays and
		reset the single one.
		*/
		char * cmd[MAXARG];
		if (strcmp(in1,"dir") == 0) { flag1 = 1; in1 = "ls"; }
		cmd[index_cmdarray] = in1;
		index_cmdarray++;
		for (;;)
		{
				if (args[index_argarray] != ' ' && args[index_argarray] != '\0')
				{
					singlearg[singleargout][singleargin] = args[index_argarray];
					singleargin++; index_argarray++;
				}
				else if (args[index_argarray] == '\0' || args[index_argarray] == ' ')
				{
					/* terminate singlearg at singleargarray, add it to
					the cmd pointer array, increment index_cmdarray, and
					reset index_single to 0 */
					singlearg[singleargout][singleargin] = '\0';
					if (flag1 == 1)
					{
						/* lets check the argument for switches for those dos junkies */
						if (chdir(singlearg[singleargout]) == -1)
						{
							/* directory is not there! */
							/* treat as switch */
							if (singlearg[singleargout][0]  == '/')
							{
								singlearg[singleargout][0] = '-';
							}
						}
						else
						{
							/* directory exists - so change back */
							if (chdir(dir) == -1)
								printf("this can't be good...\n");
						}
					}
					cmd[index_cmdarray] = singlearg[singleargout];
					index_cmdarray++;
					singleargout++; singleargin = 0;
					if (args[index_argarray] == '\0')
					{
						break;
					}
					index_argarray++;
				}
		} //while (args[index_argarray] != '\0' && index_cmdarray != MAXARG);
		cmd[index_cmdarray] = (char *)0;
		int forked = 0, exec_return = 0;
		if ((strncmp(in1,"cd", 2))==0)//strncmp returns 0 if equal
		{
			if (chdir(args) == 0)
			{
				dir = getcwd(NULL, 0); /* NULL param allows for dynamic mem alloc */
				updateprompt(dir);
			}
			else
				printf("MiniShell: directory change failed.\n");

			return;
		}
		forked = fork();
		if (forked < 0)
			perror("MiniShell");
		if (forked == 0)
		{
			/* we're in the child process */
//			print_r(cmd);
			exec_return = execvp(in1, cmd);
			if (exec_return == -1) //linux hard error
			{
				perror("MiniShell: ");
				exit(0);
			}
		}
		else
		{
			/* we're in the parent process */
			if (special == ';')
			{
				waitpid(forked, NULL, 0);
			}
			else if (special == '&')
			{
				return; //right away
			}
			else if (special == '\0')
			{
				/* perform default action of waiting */
				waitpid(forked, NULL, 0);
			}
		}
	}
}


void setvars(){
	printf("Grab current environment variables: host, user, cwd, for our prompt \n");
	dir = getcwd(NULL, 0); /* NULL param allows for dynamic mem alloc */

	user = getenv("USER");
	if(user==NULL)
		user = getenv("USERNAME");
	if(user==NULL)
		user = "me";

	host = getenv("HOSTNAME");
	if(host==NULL)
		host = "MiniShell" ;


}
void updateprompt(char *newdir)
{

	*prompt = '\0';
	strcat(prompt, "[");
	strcat(prompt, user);
	strcat(prompt, at);
	strcat(prompt, host);
	strcat(prompt, " : ");
	strcat(prompt, newdir);
	strcat(prompt, "]");
	strcat(prompt, mode);
	strcat(prompt, space);
}
