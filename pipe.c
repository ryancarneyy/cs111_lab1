#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <errno.h>

int main(int argc, char *argv[])
{
	int (*pipes)[2] = malloc((argc - 2) * sizeof(*pipes));
	pid_t *pids = malloc((argc - 1) * sizeof(*pids));
	int *status = malloc((argc - 1) * sizeof(*status));

	// checking number of arguments
	if(argc == 1) {
		printf("Usage: ./pipe <command> ... <command>\n");
		return 1;
	}
	if(argc == 2) {
		system(argv[1]);
		return 0;
	}

	if(argc == 3) {
		//TODO
	}

	if(pipe(pipes[0]) == -1) {
		printf("Error opening pipe");
		return 1;
	}
	
	// LHS process of pipe
	pids[0] = fork();

	if( pids[0] == -1 ) {
		printf("Error opening first child process");
		return 1;
	}
	// if pid1 == 0 then child process running
	else if ( pids[0] == 0 ) {
		// close the read end of the pipe in child process (unneeded)
		close(pipes[0][0]);

		// redirecting stdout of LHS to pipe's write
		if(dup2(pipes[0][1], STDOUT_FILENO) == -1) {
			printf("Error connecting ls's stdout to pipe's write");
			return 1;
		}
		// closing pipe's write in child process (unneeded)
		close(pipes[0][1]);
		// run LHS
		system(argv[1]);
		return 0;
	}
	else {
		for (int i = 1; i < argc - 2; ++i ) {
			// if not the first child, close parent process's read  
			if(i != 1) {
				close(pipes[i-2][0]);
			}

			// closing pipe's write in parent process
			close(pipes[i-1][1]);

			// wait for LHS of pipe to finish
			waitpid(pids[i-1], &status[i-1], 0);

			if( pipe(pipes[i]) == -1) {
				printf("Error creating pipe %d\n", i);
			}
			// open new child process in 
			pids[i] = fork();

			// middle child process
			if( pids[i] == 0) {
				// close read for current pipe for child process(unneeded)
				close(pipes[i][0]);

				// redirecting stdin of child to pipe's read
				if(dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
					printf("Error redirecting %s's stdin to pipe %d read", argv[i], i-1);
					return 1;
				}
				// closing previous pipe's read in child process
				close(pipes[i-1][0]);

				// redirecting stdout of child to pipe's write
				if(dup2(pipes[i][1], STDOUT_FILENO) == -1) {
					printf("Error redirecting %s's stdout to pipe %d's write", argv[i], i);
					return 1;
				}
				// close current pipe's write in child process
				close(pipes[i][1]);
				system(argv[2]);
				return 0;
			}
			else {

				// wait for child process 2 to finish
				waitpid(pids[argc-3], &status[argc-3], 0);

				const int SECONDLASTPIPE = argc-4;
				const int LASTPIPE = argc-3;
				const int LASTPID = argc-2;

				// close second to last pipe's read in parent process
				close(pipes[SECONDLASTPIPE][0]);
				// close previous pipe's write in parent process
				close(pipes[LASTPIPE][1]);

				pids[LASTPID] = fork();

				if( pids[LASTPID] == -1 ) {
					printf("Error opening final process");
					return 1;
				}
				else if( pids[LASTPID] == 0) {
					if(dup2(pipes[LASTPIPE][0], STDIN_FILENO) == -1) {
						printf("Error redirecting last process's stdin to last pipe's read");
						return 1;
					}
					// close last pipe's read in child process
					close(pipes[LASTPIPE][0]);

					system(argv[argc-1]);
					return 0;
				}
				else {
					// close last pipe's read in parent process
					waitpid(pids[LASTPID], &status[LASTPID], 0);
					close(pipes[LASTPIPE][0]);
				}

				return 0;
			}
		}
	}

}


