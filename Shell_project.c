/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/wait.h>




#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------
job * lista;

void manej(){
	//printf( ANSI_COLOR_YELLOW " Comprobacion, dentro del manejador\n" ANSI_COLOR_RESET );
	enum status status_res; 
	int estado, info;
	job * current;
	int x = list_size(lista);
	for (x; x != 0; x--){
		current = get_item_bypos(lista, x);
		status_res = current -> state;
		if(current -> state != FOREGROUND){
				int pid_wait = waitpid(current -> pgid, &estado  ,WNOHANG | WUNTRACED | WCONTINUED);
				status_res = analyze_status(estado, &info);
		
				if (pid_wait == current -> pgid){
					printf("Process pid: %s, pid=%i, estado = %s\n", current->command, pid_wait, status_strings[status_res]);
					fflush(stdout);
					if (status_res == CONTINUED) current -> state = BACKGROUND; //Check if its running in background and updates state to background
					else if (status_res == SUSPENDED || status_res == SIGTSTP ||status_res == SIGNALED ){ 
					current -> state = STOPPED; //Checks if it's suspended or signaled and changes status to stopped, not working
					add_job(lista, current);
					}
					else if((status_res == EXITED /*|| status_res == SIGNALED*/)) { //if it has exited or being signaled, it is deleted from the list
						block_SIGCHLD();
						int x = delete_job(lista, current);
						unblock_SIGCHLD();
					}
				}
		}
	}
	
}

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
	char pwd[MAX_LINE];
	ignore_terminal_signals();
	lista = new_list("lista");
	printf(ANSI_COLOR_MAGENTA "Welcome to the best shell you'll ever see. Use ctrl^D or 'exit' to exit the Shell, use any unix command to execute it and use 'zurprise' for a... surprise ;)\n");
	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   	
		signal(SIGCHLD, manej);	
		
		
		getcwd(pwd,MAX_LINE);
		printf(ANSI_COLOR_GREEN "%s" ,pwd);
		printf(ANSI_COLOR_BLUE " COMMAND:" ANSI_COLOR_RESET);
		fflush(stdout);
		
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL){ continue;   // if empty command
		}else if(strcmp(args[0], "cd") == 0){ 
			if(args[1] == NULL){ continue;}
			else if(chdir(args[1])!=0) printf( ANSI_COLOR_RED "Error, cant access %s\n" ANSI_COLOR_RESET, args[1]);
		}else if(strcmp(args[0], "jobs") == 0){ 
			print_job_list(lista);

		}else if(strcmp(args[0], "fg") == 0){ //FG
				int position, status;
				if(empty_list(lista)){ 
					printf("The list is empty.\n");
					continue;
				}else if (args[1] == NULL){ 
					position = 1;
				}else{ 
					position = atoi(args[1]);
				}
				if (get_item_bypos(lista, position) == NULL) printf("Couldn't find element number %d\n", position);
				else{
					job * task = get_item_bypos(lista, position);
				
					set_terminal(task -> pgid);
					task -> state = FOREGROUND;
					killpg(task->pgid, SIGCONT);
					waitpid(task->pgid, &status, WUNTRACED); //esperamos a que se termine o se bloquee
					set_terminal(getpid()); //AQUI EL HIJO RECUPERA LA SHELL
				
					status_res = analyze_status(status, &info); //depende de como haya terminado
					printf("Foreground pid: %d, command: %s, %s, info: %d\n", task->pgid, task->command, status_strings[status_res], info);
					
				if(status_res == SUSPENDED){ //suspendida ponemos stopped
					task->state = STOPPED;
				} else if(status_res == EXITED || status_res == SIGNALED){ //si ha terminado, borrar de la lista.
					block_SIGCHLD();
					delete_job(lista, task);
					unblock_SIGCHLD();
				}
				} continue;
		}else if (strcmp(args[0], "bg") == 0){ 
			int position, status;
			if (args[1] == NULL){
				position = 1;
			} else {
				position = atoi(args[1]);
			}
			job * task = get_item_bypos(lista, position);
			
			task->state = BACKGROUND;
			killpg(task->pgid, SIGCONT); //continue signal
			printf("Background pid: %d, command: %s\n", task->pgid, task->command);

		}else if((strcmp(args[0], "exit") == 0)){ 
			printf(ANSI_COLOR_YELLOW "According to quantum superposition you're in and out of the Shell at the same time. However, once we've measured you, we can affirm you're out. Bye product of the BigBang.\n\n"); 
			exit(0);
		}else if(strcmp(args[0], "zurprise") == 0) {
				printf("Auto redirecting would've been nice but the internet seems to not know how to do that so: https://tinyurl.com/zurprise\n");
		}
		else if(strcmp(args[0], "idk") == 0){ 
			printf( ANSI_COLOR_MAGENTA "Me neither...\n");
		}
		else if(strcmp(args[0], "hakuna") == 0){ 
			printf( ANSI_COLOR_MAGENTA "MATATA! \n");
		}
		else if(strcmp(args[0], "gg") == 0){ 
			printf( ANSI_COLOR_MAGENTA "ez\n");
		}
		else if(strcmp(args[0], "idc") == 0){ 
			printf( ANSI_COLOR_MAGENTA "Who asked lmao\n");
		}
		else if(strcmp(args[0], "never") == 0){ 
			printf( ANSI_COLOR_MAGENTA "gonna give you up!\n");
		}
		else if(strcmp(args[0], "i") == 0){ 
			printf( ANSI_COLOR_MAGENTA "wanna be the very best, like noone ever was!\n");
		}
		else if(strcmp(args[0], "ping") == 0){ 
			printf( ANSI_COLOR_MAGENTA "pong\n");
		}
		else{
		// Below is my useless code
		pid_fork = fork();	
		if(pid_fork == 0){ //We are in the child process
			new_process_group(getpid());
				
		if( background == 0){ 
			set_terminal(getpid()); // works properly
			restore_terminal_signals(); 
		}
			execvp(args[0], args);	
			// cant insert variables in perror so there it goes, a beautiful printf
			printf(ANSI_COLOR_RED "Error: command not found: %s\n" ANSI_COLOR_RESET, args[0]);
			
			exit(EXIT_FAILURE);

			//continue;
		}else if (pid_fork > 0){
			 //We are in the parent process
			
				if(background != 1){	
				pid_wait = waitpid(pid_fork,&status,WUNTRACED);
				status_res = analyze_status(status, &info);
				if(status == SIGNALED) manej(status_res);
				set_terminal(getpid());
				
				
				printf("Foreground pid: %d command: %s, %d , info: %d\n", pid_fork, args[0], status_res, info);
				}else{
					set_terminal(getpid()); 
					status_res = analyze_status(status, &info);
					add_job(lista, new_job( pid_fork, args[0], BACKGROUND));
					if(status_res == SIGCHLD) manej();
					printf("Background job running... pid: %d command: %s\n", pid_fork, args[0]); 

				}
			
			
		}else {
			printf("Error creating child\n");
		}continue;
	}
		/* the steps are:
			 (1) fork a child process using fork() 
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/

	} // end while
}
