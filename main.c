/*
	TP de système d'exploitation
		 Mini-shell
	Mars 2009
	Afifi Sohaib    :  me [at]  sohaibafifi [dot]  com
	Arbaoui Taha   :  tahaarbaoui [at]  gmail  [dot]  com

 */


#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/wait.h>   /* pour  wait  */
#include <ctype.h>      /* pour  isspace */
#include <string.h>     // pour strerror
#include <errno.h>      /* pour errno */
#include <signal.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "rline.h"

extern char *readline (const char *prompt);
#define MAXELEMS 32
#define STDIN 0
#define STDOUT 1

char   ligne[4096];     /* contient la ligne d'entrée */
char Prompt[255];
static char *dir;
const char *history_filename = "History";



char* elems[MAXELEMS]; /* pointeurs sur les mots de ligne (voir decoupe) */


void prompt()
{
    char *user;
    char *host;
    char *at = "@";
    char *space = " ";
    char *mode = "$";
    //printf("Grab current environment variables: host, user, cwd, for our prompt \n");
    dir = getcwd(NULL, 0); /* NULL param allows for dynamic mem alloc */

    user = getenv("USER");
    if (user==NULL)
        user = getenv("USERNAME");
    if (user==NULL)
        user = "me";

    host = getenv("HOSTNAME");
    if (host==NULL)
        host = "MiniShell" ;

    sprintf(Prompt,"\n[%s %s %s]%s%s: ",user,at,host,dir,mode);
    //fflush(stdout);
}

void lire()
{
    /* fgets peut être interrompu par le signal child_signal => on boucle */
    /*int fin = 0;
    while (!fin) {
      if (fgets(ligne,sizeof(ligne)-1,stdin)) break; /* lecture OK */
    /*if (feof(stdin)) { // la fin de lecture
      printf("\n");
      fin = 1;
    }
    }
    *history[cpthis++] = *ligne;*/
    //printf("\n");
    char *newligne = (char *)NULL;
    //free(ligne);
    *ligne = (char *)NULL;
    newligne = readline(Prompt);
    sprintf (ligne, "%s\n", newligne);
    if (ligne && *ligne)
        add_history(ligne);


}

/* découpe ligne en mots  et les mettre dans elems
   elems doit se termine par NULL pour execvp
 */
void decoupe()
{
    char* first = ligne;
    int i;
    for (i=0; i<MAXELEMS-1; i++)
    {

        /* saute les espaces */
        while (*first && isspace(*first)) first++;

        /* fin de ligne ? */
        if (!*first) break;

        /* on se souvient du début de ce mot */
        elems[i] = first;

        /* cherche la fin du mot */
        while (*first && !isspace(*first)) first++; /* saute le mot */

        /* termine le mot par un \0 et passe au suivant */
        if (*first)
        {
            *first = 0;
            first++;
        }
    }

    elems[i] = NULL; // le dernier pour execvp
}


/* XXX
   fg_pid = pid du processus au premier plan

   permet à child_signal de notifier à execute que la commande au premier
   plan vient de se terminer

   la variable étant partagée entre le programme principal (lance_commande) et
   un handler de signal asynchrone (child_signal), elle dont être marquée volatile
*/
volatile int fg_pid = -1;

void child_signal(int signal)
{
    /* un signal peut correspondre à plusieurs fils finis, donc on boucle */
    while (1)
    {
        printf(" wait ");
        int status;
        pid_t pid = waitpid(-1,&status,WNOHANG);
        if (pid<0)
        {
            if (errno==EINTR) continue; /* interrompu => on recommence */
            if (errno==ECHILD) break;   /* plus de fils terminé => on quitte */
            printf("erreur de wait : (%s)\n",strerror(errno));
            break;
        }

        if (pid==0) break; /* plus de fils terminé => on quitte */

        /* XXX signale à execute que fg_pid s'est terminé */
        if (pid==fg_pid) fg_pid = -1;

        if (WIFEXITED(status))
            printf("terminaison normale, pid=%i, status %i\n",pid,WEXITSTATUS(status));
        if (WIFSIGNALED(status))
            printf("terminaison par signal %i, pid=%i\n",WTERMSIG(status),pid);
    }
}

void execute()
{
    sigset_t sigset;
    pid_t pid;
    int en_fond;

    /* XXX détection du & */
    if (strchr(ligne,'&'))
    {
        *strchr(ligne,'&') = 0;
        en_fond = 1;
    }
    else
    {
        /* bloque child_signal jusqu'à ce que le père ait placé le pid du
           fils dans fg_pid
           sinon on risque de manquer la notification de fin du fils
           (race condition)
        */
        sigemptyset(&sigset);
        sigaddset(&sigset,child_signal);
        sigprocmask(SIG_BLOCK,&sigset,NULL);
        en_fond = 0;
    }

    decoupe();
    if (!elems[0]) return; /* ligne vide */
    // traite la commande interne  : cd
    if ((strncmp(elems[0],"cd", 2))==0) //strncmp returns 0 if equal
    {
        if (strncmp(elems[1] , "~" ,1) == 0)
            dir = getenv("HOME");
        else
            dir = elems[1];

        if (chdir(dir) == 0)
        {
            dir = getcwd(NULL, 0);
        }
        else
            printf("MiniShell: directory change failed.\n");
        return;
    }
    if ((strncmp(elems[0],"history", 7))==0) //strncmp returns 0 if equal
    {
        HIST_ENTRY **histlist;
        int i;
        histlist =  history_list ();
          if (histlist)
            for (i = 0; histlist[i]; i++)
              printf ("%d: %s\n", i , histlist[i]->line);

        return;
    }

    pid = fork();
    if (pid < 0)
    {
        printf("fork a échoué (%s)\n",strerror(errno));
        return;
    }

    if (pid==0)
    {
        /* fils */

        if (en_fond)
        {
            /* XXX redirection de l'entrée standard sur /dev/null */
            printf("redirection de l'entrée standard sur /dev/null");
            int devnull = open("/dev/null",O_RDONLY);
            if (devnull != -1)
            {
                close(0);
                dup2(devnull,0);
            }
        }
        else
        {
            /* XXX réactivation de SIGINT & débloque SIGCLHD   Contrôle+C  */
            struct sigaction sig;
            sig.sa_flags = 0;
            sig.sa_handler = SIG_DFL;
            sigemptyset(&sig.sa_mask);
            sigaction(SIGINT,&sig,NULL);
            sigprocmask(SIG_UNBLOCK,&sigset,NULL);
        }

        execvp(elems[0], /* programme à exécuter */
               elems     /* argv du programme à exécuter */
              );
        printf("impossible d'éxecuter \"%s\" (%s)\n",elems[0],strerror(errno));
        exit(1);
    }

    else
    {

        if (!en_fond)
        {
            fflush(stdout);

            /* XXX attent la fin du processus au premier plan */
            printf("attent la fin du processus au premier plan \n");
            waitpid(pid, NULL, 0);
            /*printf("pid %i\n",pid);
            fg_pid = pid;
            sigprocmask(SIG_UNBLOCK,&sigset,NULL);

            /* attente bloquante jusqu'à ce que child_signal signale que fg_pid
               s'est terminé */
            while (fg_pid!=-1)
            {
//		pause();
                fg_pid = pid;
            }

        }

    }
}
int runPipedCommands(char *items1[],char *items2[]) {

  if ( items2[0] == NULL) {
    printf("%s command not found.n", items2[0]);
    return 1;
  }

  int fd[2];
  int pid, pid2;
  if (pipe(fd) == -1) {
    printf("Error: Pipe failed.n");
    return 1;
  }

  if ((pid = fork()) < 0) {
    printf("Error: Fork failed.n");
    return 1;
  }

  if (pid == 0) {
    // child process
    close(fd[1]);
    dup2(fd[0], 0);
    close(fd[0]);
    execvp( items2[0], items2);
    printf("Error: execvp failed.n");
    return 1;
  } else {
    // parent process
    // need to fork again, so the shell isn't replaced
    if ((pid2 = fork()) < 0) {
      printf("Error: Fork failed.n");
      return 1;
    }
    if (pid2 == 0) {
      // child process
      close(fd[0]);
      dup2(fd[1], 1);
      close(fd[1]);
      execvp( items1[0], items1);
      printf("Error: execvp failed.n");
      return 1;
    } else {
      // parent process (the shell)
      close(fd[0]);
      close(fd[1]);
      //while (wait(&status) != pid2);
    }
  }
  return 0;
}

// redirects STDIN or STDOUT for a command
int runRedirectedCommand(char *items[], char* file, int fileid) {


  int pid;
  if ((pid = fork()) < 0) {
    printf("Error: Fork failed.n");
    return 1;
  }
  if (pid == 0) {
    // child process
    int fid;
    if (fileid == STDIN) {
      fid = open(file, O_RDONLY, 0600);
    } else if (fileid == STDOUT) {
      fid = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0600);
    }
    dup2(fid, fileid);
    close(fid);
    execvp(items[0], items);
    printf("Error: execvp failed.n");
    return 1;
  } else {
    // parent process (shell)
   // while (wait(&status) != pid);
  }
}



////===========================================================================================
int main()
{
    /* XXX installation du handler pour le signal child_signal */
    struct sigaction sig;
    sig.sa_flags = 0;
    sig.sa_handler = child_signal;
    sigemptyset(&sig.sa_mask);
    sigaction(child_signal,&sig,NULL);

    /* XXX désactivation l'interruption par Contrôle+C */
    sig.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sig, NULL);

    initialize_readline ();
    using_history ();

    printf("Lecture de fichier d'histoire : (%s)\n",strerror(read_history (history_filename)));

    while (1)
    {
        prompt();
        lire();
        if (strncmp(ligne , "exit" ,4) ==0) break;
        execute();
    }
    printf("Sauvegarde de fichier d'histoire : (%s)\n",strerror(write_history(history_filename)));


    return 0;
}
