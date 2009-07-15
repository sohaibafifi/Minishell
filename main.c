/*
	TP de système d'exploitation
		 Mini-shell
	Avril 2009
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
#include <signal.h>     /* traitement des signaux*/
#include <fcntl.h>
#include <glob.h>       /*pour les meta caracteres et les motifs */
#include <readline/readline.h> /*pour la lecture avancer des commande et le autocompletion*/
#include <readline/history.h>  /*gestion de l'historique*/
#include "rline.h"       /* pour l'auto completion*/

////===========================================================================================
#define MAXELEMS 32
#define STDIN 0
#define STDOUT 1
#define MAXCMD 55

#define attr_reset 0
#define attr_bright 1
#define attr_dim 2
#define attr_underline 3
#define attr_blink 5
#define attr_reverse 7
#define attr_hidden 8

#define fg_black 30
#define fg_red 31
#define fg_green 32
#define fg_yellow 33
#define fg_blue 34
#define fg_magenta 35
#define fg_cyan 36
#define fg_white 37

#define bg_black 40
#define bg_red 41
#define bg_green 42
#define bg_yellow 43
#define bg_blue 44
#define bg_magenta 45
#define bg_cyan 46
#define bg_white 47
////===========================================================================================


char   ligne[MAXCMD];     /* contient la ligne d'entrée */
char   ligne_tmp[MAXCMD];
char   ligne_tmp2[MAXCMD];
char*   ligne_tab[MAXELEMS];
char cmds[MAXELEMS][MAXCMD];
char cmds_pipe[MAXELEMS][MAXCMD];
char cmds_seq[MAXELEMS][MAXCMD];
char cmds_par[MAXELEMS][MAXCMD];
int numcmd;
int numcmd_seq , numcmd_pipe, numcmd_par;
char  ligne_glb[MAXCMD];
char  input[MAXCMD];
char Prompt[MAXCMD];
static char *dir;
const char *history_filename = "History";



char* elems[MAXELEMS]; /* pointeurs sur les mots de la ligne (voir decoupe_cmd) */
char* elems1[MAXELEMS]; /* pointeurs sur les mots de la commande1 (voir decoupe_cmd) */
char* elems2[MAXELEMS]; /* pointeurs sur les mots de la commande2 (voir decoupe_cmd) */


int seq=0;
int next=0;

//booleans pour les types de commandes
int piped = 0;
int par = 0;
int redirected = 0;
int double_redirection = 0;
int red_type;
char *red_file;
int use_glob = 0;
int en_fond = 0;
int Exit=0;

////===========================================================================================
// colorer un texte
char* colorer(int attribute, int foreground, int background, char *text) {
    char string[4096];
    sprintf(string, "%c[%d;%d;%dm%s%c[0m", 27, attribute, foreground, background, text, 27);
    return string;
}

////===========================================================================================
//initialiser les variables globals
void init() {
    int i,j;
    char null[55];
    null[0] = '\0';
    for (i=0;i<MAXELEMS;i++)
        strcpy(cmds[i],null);
    for (i=0;i<MAXELEMS;i++) {
        elems[i] = NULL;
        elems2[i] = NULL;
        elems1[i] = NULL;
    }
    *ligne = (char *)NULL;
    en_fond = 0;


    numcmd=0;
    piped = 0;
    redirected = 0;
    double_redirection = 0;
    use_glob = 0;
    red_file= (char *)malloc(sizeof(char));

    *ligne_glb = (char *)NULL;
}

////===========================================================================================
//affichage du prompe
void prompt() {
    char *user;
    char *host;
    char *at = "@";

    char *mode = "$";
    //Grab current environment variables: host, user, cwd, for our prompt
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

////===========================================================================================
// netoyer le nom du fichier
char *clean_filename(char *filename) {
    int cpt = 0;
    int cpt2=0;
    char *newfilename;
    char *first = filename;
    char *i = filename;
    while (*first && isspace(*first))first++;

    //if (!*first) ;

    newfilename = first;
    i = first;

    while (*i && (isalnum(*i)|| *i == '.' || *i == '/'))
        i++; /* saute le mot */

    i = '\0';

    return newfilename;

}

////===========================================================================================
// lire une ligne des commandes a partire du clavier
void lire() {

    char *newligne = (char *)NULL;
    *ligne_glb = (char *)NULL;
    newligne = readline(colorer(attr_underline, fg_green, bg_black, Prompt));
    sprintf (ligne_glb, "%s\n", newligne);
    sprintf (input, "%s\n", newligne);

    if (ligne_glb && *ligne_glb)
        add_history(ligne_glb);
}

////===========================================================================================
//decouper la ligne et extraire les commandes sequentielles
void decoupe_ligne_seq() {
    if (!ligne_glb) exit;
    numcmd_seq = 0;
    next = 0;
    char temp[55];

    if (strchr(ligne_glb , '|')) piped = 1;
    if ((strchr(ligne_glb , '>'))||(strchr(ligne_glb , '>'))) redirected = 1;

    else
        if (strchr(ligne_glb , ';')) seq = 1;

    do {

        int i=0;
        while (ligne_glb[next]==';')next++;

        for (;((ligne_glb[next]!=';')&&(ligne_glb[next]!='\n'));next++) {
            temp[i]=ligne_glb[next];
            i++;
        }
        if (i!=0)temp[i]='\0';
        else break;//commande vide

        strcpy(cmds_seq[numcmd_seq],temp);
        numcmd_seq++;
    } while (ligne_glb[next]!='\n');

}
////===========================================================================================
//decouper la ligne et extraire les commandes en pipes
void decoupe_ligne_pipe(char input[]) {
    if (!input) exit;
    numcmd_pipe = 0;
    int cpt = 0;
    char temp[55];

    if (strchr(input , '|')) piped = 1;
    do {

        int i=0;
        while (input[cpt]=='|')cpt++;

        for (;((input[cpt]!='|')&&(input[cpt]!='\0'));cpt++) {
            temp[i]=input[cpt];
            i++;
        }
        if (i!=0)temp[i]='\0';
        else break;//commande vide

        strcpy(cmds_pipe[numcmd_pipe],temp);
        numcmd_pipe++;
    } while (input[cpt]!='\n');

}

////===========================================================================================
//decouper la ligne et extraire les commandes en parallele
void decoupe_ligne_par(char input[]) {
    if (!input) exit;
    numcmd_par = 0;
    int cpt = 0;
    char temp[55];

    if (strchr(input , '&')) par = 1;
    do {

        int i=0;
        while (input[cpt]=='&')cpt++;

        for (;((input[cpt]!='&')&&(input[cpt]!='\0'));cpt++) {
            temp[i]=input[cpt];
            i++;
        }
        if (i!=0)temp[i]='\0';
        else break;//commande vide

        strcpy(cmds_pipe[numcmd_par],temp);
        numcmd_par++;
    } while (input[cpt]!='\n');

}

////===========================================================================================
/* découpe la commande en mots  et les mettre dans elem
   elem doit se termine par NULL pour execvp
 */
void decoupe_cmd(char line[] , char *elem[]) {
    if (!line) exit;
    char* first = line;
    char* cpt = line;

    if (strchr(line , '&')) en_fond = 1;
    if (strchr(line , '*')) use_glob = 1;

    if (strchr(line , '>')) {
        redirected = 1;
        red_type = STDOUT;
    }
    if (strchr(line , '<')) {
        redirected = 1;
        red_type = STDIN;
    }

    if (strchr(line , '>')==(strrchr(line , '>')-1)) {
        double_redirection = 1;
        printf("first = %d ; last %d",strchr(line , '>') , strrchr(line , '>'));
        fflush(stdout);
    }
    int i;

    if (redirected) {
        if (red_type == STDOUT)first = strtok( line, ">");
        else first = strtok( line, "<");

    }

    if ((redirected && red_type == STDIN)) {
        redirected = 1;
        if (red_file!= NULL ) {
            char *_red_file;
            _red_file = strtok( NULL, "<" );
            red_file = clean_filename(_red_file);

            exit;
        }
    }
    if (redirected && red_type == STDOUT) {
        redirected = 1;

        if (red_file!= NULL ) {
            char *_red_file;


            _red_file = strtok( NULL, ">" );


            red_file = clean_filename(_red_file);
            exit;
        }

    }
    //ligne = strtok(ligne , '|');
    for (i=0;i<MAXELEMS-1; i++) {
        /* saute les espaces */
        while (*first && (isspace(*first) || (*first=='&')))
            first++;
        /* fin de ligne ? */
        if (!*first) break;
        /* on se souvient du début de ce mot */
        elem[i] = first;
        cpt = first;

        /* cherche la fin du mot */
        while (*cpt && !isspace(*cpt) )
            cpt++; /* saute le mot */

        /* termine le mot par un \0 et passe au suivant */
        if (*cpt) {
            *cpt = 0;
            cpt++;
        }
        first = cpt;
    }

    elem[i] = NULL; // le dernier doit etre NULL pour execvp

    if ((i!=0)&&(((strncmp(elem[0],"ls", 2))==0)||((strncmp(elem[0],"grep", 4))==0))) { //Ajouter la colorisation
        int fin = 0;
        char* temp;
        do {
            fin++;
        } while (elem[fin]!=NULL);
        int i;
        for (i=fin;i>1;i--)
            elem[i]=elem[i-1];
        elem[1]="--color=auto";
        elem[fin+1] = NULL;
    }

}


/*
   fg_pid = pid du processus au premier plan

   permet à child_signal de notifier à execute que la commande au premier
   plan vient de se terminer

   la variable étant partagée entre le programme principal (lance_commande) et
   un handler de signal asynchrone (child_signal), elle dont être marquée volatile
*/
////===========================================================================================
volatile int fg_pid = -1;
////===========================================================================================
void child_signal(int signal) {
    /* un signal peut correspondre à plusieurs fils finis, donc on boucle */
    while (1) {
        printf(" wait ");
        int status;
        pid_t pid = waitpid(-1,&status,WNOHANG);
        if (pid<0) {
            if (errno==EINTR) continue; /* interrompu => on recommence */
            if (errno==ECHILD) break;   /* plus de fils terminé => on quitte */
            printf("erreur de wait : (%s)\n",strerror(errno));
            break;
        }

        if (pid==0) break; /* plus de fils terminé => on quitte */

        /*  signale à execute que fg_pid s'est terminé */
        if (pid==fg_pid) fg_pid = -1;

        if (WIFEXITED(status))
            printf("terminaison normale, pid=%i, status %i\n",pid,WEXITSTATUS(status));
        if (WIFSIGNALED(status))
            printf("terminaison par signal %i, pid=%i\n",WTERMSIG(status),pid);
    }
}
////===========================================================================================
// executer une commande simple
void execute(char* elem[]) {
    sigset_t sigset;
    pid_t pid;
    if (!elem[0]) exit;
    if (!en_fond) {
        /* bloque child_signal jusqu'à ce que le père ait placé le pid du
           fils dans fg_pid
           sinon on risque de manquer la notification de fin du fils
           (race condition)
        */
        sigemptyset(&sigset);
        sigaddset(&sigset,child_signal);
        sigprocmask(SIG_BLOCK,&sigset,NULL);

    }

    if (!elem[0]) return; /* ligne vide */
    if (((strncmp(elem[0],"exit", 4))==0)||((strncmp(elem[0],"quit", 4))==0)) {
        Exit = 1;
    }
    // traite la commande interne  : cd
    if ((strncmp(elem[0],"cd", 2))==0) { //strncmp returns 0 if equal
        if (strncmp(elem[1] , "~" ,1) == 0)
            dir = getenv("HOME");
        else
            dir = elem[1];

        if (chdir(dir) == 0) {
            dir = getcwd(NULL, 0);
        } else
            printf("MiniShell: directory change failed.\n");
        return;
    }
    // traite la commande interne  : history
    if ((strncmp(elem[0],"history", 7))==0) { //strncmp returns 0 if equal
        HIST_ENTRY **histlist;
        int i;
        histlist =  history_list();
        if (histlist)
            for (i = 0; histlist[i]; i++)
                printf ("%d: %s\n", i , histlist[i]->line);

        return;
    }
    // traite la commande interne  : set
    if ((strncmp(elem[0],"set", 3))==0) { //strncmp returns 0 if equal
        char env[100];
        sprintf(env ,"%s=%s",elem[1],elem[2]) ;
        if (putenv(env)!=0)
            printf("set %s as %s (%s): %s\n",elem[1],elems[2],env,strerror(errno));
        printf("\n[%s]",env);
        return;
    }
    // traite la commande interne  : help
    if ((strncmp(elem[0],"help", 4))==0) { //strncmp returns 0 if equal
        printf("MiniShell, version 0.2 (Tp systeme d'exploitation)\n");
        printf("Ces commandes de shell sont définies de manière interne.Tapez « help » pour voir cette liste.\n");
        printf("Utilisez « man » ou « info » pour en savoir plus sur les commandes qui\n");
        printf("ne font pas partie de cette liste.\n");
        printf("\n");
        printf("history         Affiche l'historique des commandes  \n");
        printf("set var val     Definire des variables d'environnement\n");
        printf("quit,exit       Sortir du shell\n");
        printf("cd dossier      Changer le repertoire courant\n");
        printf("help            Afficher cette liste d'aide\n");
        fflush(stdout);
        return;
    }
    if (!Exit) {
        pid = fork();
        if (pid < 0) {
            printf("fork a échoué (%s)\n",strerror(errno));
            return;
        }

        if (pid==0) {
            /* fils */

            if (en_fond) {
                /*  redirection de l'entrée standard sur /dev/null */
                printf("redirection de l'entrée standard sur /dev/null \n %s [%d]",elem[0],getpid());
                fflush(stdout);
                int devnull = open("/dev/null",O_RDONLY);
                if (devnull != -1) {
                    close(0);
                    dup2(devnull,0);
                }
            } else {
                /* réactivation de SIGINT & débloque SIGCLHD   Contrôle+C  */
                struct sigaction sig;
                sig.sa_flags = 0;
                sig.sa_handler = SIG_DFL;
                sigemptyset(&sig.sa_mask);
                sigaction(SIGINT,&sig,NULL);
                sigprocmask(SIG_UNBLOCK,&sigset,NULL);
            }
            // gerer les motifs (les meta caracteres)
            if (use_glob) {
                glob_t globbuf;
                int i;
                int num=0;
                for (i = 1 ; elem[i] !=NULL ; i++) {
                    if (strchr(elem[i] , '*')!=NULL) {
                        num++;
                    }
                }
                globbuf.gl_offs = num;
                printf("\nTraduction des meta-caracteres avec off = %d ,%d\n",globbuf.gl_offs,num);
                fflush(stdout);
                int cptm =0;
                for (i = 1 ; elem[i] !=NULL ; i++)
                    if (strchr(elem[i] , '*')) {
                        cptm++;
                        if (cptm==1)
                            glob(elem[i],  GLOB_DOOFFS, NULL, &globbuf);
                        else
                            glob(elem[i],  GLOB_DOOFFS|GLOB_APPEND, NULL, &globbuf);
                        printf("%d...traduction de [%s] \n",cptm,elem[i]);

                    }
                int cptg = 1;
                globbuf.gl_pathv[0] = elem[0];
                for (i = 1 ; elem[i] !=NULL ; i++)
                    if (strchr(elem[i] , '*')==NULL) {
                        globbuf.gl_pathv[cptg] = elem[i];
                        cptg++;
                }


                execvp(elem[0], &globbuf.gl_pathv[0]);
            } else
                execvp(elem[0], /* programme à exécuter */
                       elem    /* argv du programme à exécuter */
                      );
            printf("impossible d'éxecuter \"%s\" (%s)\n",elem[0],strerror(errno));
            exit(1);
        }

        else {

            if (!en_fond) {
                /*  attent la fin du processus au premier plan */
                fflush(stdout);
                printf("attent la fin du processus au premier plan \n");
                fflush(stdout);
                waitpid(pid, NULL, 0);
                /*printf("pid %i\n",pid);
                fg_pid = pid;
                sigprocmask(SIG_UNBLOCK,&sigset,NULL);

                /* attente bloquante jusqu'à ce que child_signal signale que fg_pid
                   s'est terminé */
                while (fg_pid!=-1) {
                    //		pause();
                    fg_pid = pid;
                }

            }

        }
    }
}

////===========================================================================================
// executer les pipes recursivement
void recursive_pipe(int i) {
    char   line[55];
    // if(!cmds_pipe[i])
    strcpy(line,cmds_pipe[i]);
    char* elem[MAXELEMS];
    decoupe_cmd(line,elem);
    printf("\n %d Execution de %s  \n",i,line);
    fflush(stdout);
    int fd[2];
    int pid;
    int Pipe;
    Pipe=pipe(fd) ;
    if (Pipe== -1) {
        printf("Error: Pipe failed.n");
        return 1;
    }
    if (i!=(numcmd_pipe-1)) {
        pid=fork();
        if (pid  < 0) {
            printf("Error: Fork failed.n");
            return 1;
        }

        if (pid == 0) { // child process
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            recursive_pipe(i+1);
            printf("Error: execvp failed.n");
            return 1;
        } else { // parent process
            close(fd[0]);
            dup2(fd[1], 1);
            close(fd[1]);

            int erreur = execvp( elem[0], elem);
            printf("Error: execvp failed : (%s)\n",strerror(erreur));
        }
    } else {
        close(fd[0]);
        execvp( elem[0], elem);
        printf("Error: execvp failed.n");

    }

}

////===========================================================================================
// executer les commandes en pipes
void runPipe() {
    if (numcmd_pipe>0) {
        int pid=fork();
        if (pid  < 0) {
            printf("Error: Fork failed.n");
            return 1;
        }

        if (pid == 0) { // child process
            recursive_pipe(0);
        } else { //parent (Shell)
            waitpid(pid, NULL, 0);
        }
    }
}
////===========================================================================================
// deuxieme methode pour executer les commandes en pipes
void ExecutePipes() {
    int cpt;
    char *file1 = ".tmp1";
    char *file2 = ".tmp2";
    for (cpt=0;cpt<numcmd;cpt++) {

        int pid = fork();
        if (pid < 0) {
            printf("Error: Fork failed.n");
            return 1;
        }
        if (pid == 0) {
            // child process
            // redirection des sorties

            if (cpt!=(numcmd-1)) {
                int fid;
                fid = open(file1, O_CREAT | O_TRUNC | O_WRONLY, 0600);
                dup2(fid, STDOUT);
                close(fid);
            }
            // redirection des entrees
            if (cpt!=0) {
                int fidIn;

                fidIn = open(file2, O_RDONLY, 0600);
                fprintf(stdout,"Lecture a partire de %s",file2);
                fflush(stdout);
                dup2(fidIn, STDIN);
                close(fidIn);
            }

            strcpy(ligne,cmds[cpt]);
            decoupe_cmd(ligne,elems);
            execvp(elems[0], elems);
            printf("Error: execvp failed.n");
            return 1;
        } else {
            waitpid(pid, NULL, 0);
        }

        rename(file1,file2);
    }
    piped = 0;
    dup2(STDOUT, STDOUT);
}
////===========================================================================================
// executer une commande contient une redirection fileid (STDOUT ou STDIN) vers file
int runRedirectedCommand(char *items[], char *file, int fileid) {

    int pid = fork();
    if (pid < 0) {
        printf("Error: Fork failed.n");
        return 1;
    }
    if (pid == 0) {
        // child process
        int fid;
        if (fileid == STDIN) {
            fid = open(file, O_RDONLY, 0600);
            printf("\n lecture a partir du fichier %s\n",file);
            fflush(stdout);
        } else if (fileid == STDOUT) {
            if (double_redirection==0) {
                fid = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0600);
                printf("\ncreation de fichier %s\n",file);
            } else {
                fid = open(file, O_APPEND| O_WRONLY, 0600);
                printf("\nAjoute au fichier %s\n",file);
            }
            fflush(stdout);
        }
        dup2(fid, fileid);
        close(fid);
        printf("\nExecution de %s sur le fichier [%s] fileid=%d\n",items[0],file,fileid);
        fflush(stdout);
        execvp(items[0], items);
        printf("Error: execvp failed.n");
        return 1;
    } else {
        // parent process (shell)
        //waitpid(pid, NULL, 0);
        // while (wait(&status) != pid);
    }
}



////===========================================================================================
int main() {
    /* installation du handler pour le signal child_signal */
    struct sigaction sig;
    sig.sa_flags = 0;
    sig.sa_handler = child_signal;
    sigemptyset(&sig.sa_mask);
    sigaction(child_signal,&sig,NULL);

    /* désactivation l'interruption par Contrôle+C */
    sig.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sig, NULL);

    initialize_readline ();
    using_history ();

    printf("Lecture de fichier d'histoire : (%s)\n",strerror(read_history (history_filename)));

    while (!Exit) {
        init();
        prompt();
        next=0;
        lire();
        decoupe_ligne_seq();
        int i=0;
        for (;i<numcmd_seq;i++) {
            char *line;
            char* elem[MAXELEMS];
            //strcpy(&line,cmds_seq[i]);
            decoupe_ligne_pipe(cmds_seq[i]);
            if (piped) {
                runPipe();
            } else {
                decoupe_cmd(cmds_seq[i],elem);
                if (redirected) {
                    runRedirectedCommand(elem,red_file,red_type);
                } else {
                    execute(elem);
                }
            }
        }
    }
    printf("Sauvegarde de fichier d'histoire : (%s)\n",strerror(write_history(history_filename)));
    return 0;
}
