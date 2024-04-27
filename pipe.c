#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <errno.h>

int main(int argc, char *argv[])
{							
	int returnval;

	if(argc == 1) {					// 0 ARG PIPE
		printf("Usage: ./pipe <command> ... <command>\n");
		return 1;
	}

	if(argc == 2) {					// 1 ARG PIPE
		returnval = system(argv[1]);
		if(returnval == -1) {
			fprintf(stderr, "%s returned -1", argv[1]);
			return -1;
		}
		else if(WIFEXITED(returnval)) {
			if(WEXITSTATUS(returnval) == 127) {
				fprintf(stderr, "Command not found or not executed: %s\n", argv[1]);
				return EINVAL;
			}
			else {
				return 0;
			}
		}
	}

	int term_status;

	if(argc == 3) {                   // 2 ARG PIPE
		int pipefd[2];
		int pids[2];
		int status[2];

		if(pipe(pipefd) == -1) {
			perror("Error opening pipe\n");
			return 1;
		}
		
		pids[0] = fork();

		if( pids[0] == -1 ) {
			perror("Error opening first child process\n");
			return 1;
		}
		// if pid1 == 0 then child process running
		else if ( pids[0] == 0 ) {
			// close the read end of the pipe in child process (unneeded)
			close(pipefd[0]);

			// redirecting stdout of LHS to pipe's write
			if(dup2(pipefd[1], STDOUT_FILENO) == -1) {
				perror("Error connecting ls's stdout to pipe's write\n");
				return 1;
			}
			// closing pipe's write in child process (unneeded)
			close(pipefd[1]);


			// run LHS
			returnval = system(argv[1]);
			if(returnval == -1) {
				fprintf(stderr, "%s returned -1\n", argv[1]);
				return -1;
			}
			else if(WIFEXITED(returnval)) {
				if(WEXITSTATUS(returnval) == 127) {
					fprintf(stderr,"invalid command: %s\n", argv[1]);
					return EINVAL;
				}
				else 
					return 0;
			}
		}
		else {
			// wait for first child process to finish
			term_status = waitpid(pids[0], &status[0], 0);
			if(term_status > 0) {
				if (WIFEXITED(status[0])) {
					if(WEXITSTATUS(status[0]) == EINVAL) {
						fprintf(stderr,"process PID: %d exited with status: %d\n", term_status, WEXITSTATUS(status[0]));
						return(WEXITSTATUS(status[0]));
					}
				} else {
					printf("process PID: %d terminated abnormally\n", term_status);
					return -1;
				}
			} else {
				perror("waitpid returned abnormal value");
				return 1;
			}
			
			// close write fd for parent process
			close(pipefd[1]);
			// 2nd child process
			pids[1] = fork();

			if (pids[1] == -1 ) {
				perror("Error opening second child process\n");
				return 1;
			}
			else if(pids[1] == 0) {
				// redirect stdin to pipe's read
				if(dup2(pipefd[0], STDIN_FILENO) == -1) {
					perror("Error connecting ls's stdout to pipe's write\n");
					return 1;
				}
				// close child process's read
				close(pipefd[0]);

				// return to parent process
				returnval = system(argv[2]);
				if(returnval == -1) {
					fprintf(stderr, "%s returned -1\n", argv[2]);
					return -1;
				}
				else if(WIFEXITED(returnval)) {
					if(WEXITSTATUS(returnval) == 127) {
						fprintf(stderr,"invalid command: %s\n", argv[2]);
						return EINVAL;
					}
					else 
						return 0;
				}
			}
			else {
				// wait for RHS child process to finish 
				term_status = waitpid(pids[1], &status[1], 0);
				if(term_status > 0) {
					if (WIFEXITED(status[1])) {
						if(WEXITSTATUS(status[1]) == EINVAL) {
							fprintf(stderr,"process PID: %d exited with status: %d\n", term_status, WEXITSTATUS(status[1]));
							return(WEXITSTATUS(status[1]));
						}
					} else {
						printf("process PID: %d terminated abnormally\n", term_status);
						return -1;
					}
				} else {
					perror("waitpid returned incorrect value");
					return 1;
				}

				// close read pipe in parent process
				close(pipefd[0]);
				return 0;
			}
		}
	}

	int (*pipes)[2] = malloc((argc - 2) * sizeof(*pipes));				// 3 OR MORE ARG PIPE
	pid_t *pids = malloc((argc - 1) * sizeof(*pids));
	int *status = malloc((argc - 1) * sizeof(*status));

	if(pipe(pipes[0]) == -1) {
		perror("Error opening pipe\n");
		return 1;
	}
	
	// LHS process of pipe
	pids[0] = fork();

	if( pids[0] == -1 ) {
		printf("Error opening first child process\n");
		return 1;
	}
	// if pid1 == 0 then child process running
	else if ( pids[0] == 0 ) {
		// close the read end of the pipe in child process (unneeded)
		close(pipes[0][0]);

		// redirecting stdout of LHS to pipe's write
		if(dup2(pipes[0][1], STDOUT_FILENO) == -1) {
			printf("Error connecting ls's stdout to pipe's write\n");
			return 1;
		}
		// closing pipe's write in child process (unneeded)
		close(pipes[0][1]);
		// run LHS
		returnval = system(argv[1]);
		if(returnval == -1) {
			fprintf(stderr, "%s returned -1", argv[1]);
			return -1;
		}
		else if(WIFEXITED(returnval)) {
			if(WEXITSTATUS(returnval) == 127) {
				fprintf(stderr, "Command not found or not executed: %s\n", argv[1]);
				return EINVAL;
			}
			else {
				return 0;
			}
		}
	}
	// parent process running
	else {
		term_status = waitpid(pids[0], &status[0], 0);
		if(term_status > 0) {
			if (WIFEXITED(status[0])) {
				if(WEXITSTATUS(status[0]) == EINVAL) {
					fprintf(stderr,"process PID: %d exited with status: %d\n", term_status, WEXITSTATUS(status[0]));
					return(WEXITSTATUS(status[0]));
				}
			} else {
				printf("process PID: %d terminated abnormally\n", term_status);
				return -1;
			}
		} else {
			perror("waitpid returned incorrect value");
			return 1;
		}

		for (int i = 1; i < argc - 2; ++i ) {
			// if not the first child, close parent process's read  
			if(i != 1) {
				close(pipes[i-2][0]);
			}

			// closing pipe's write in parent process
			close(pipes[i-1][1]);

			// wait for LHS of pipe to finish
			// waitpid(pids[i-1], &status[i-1], 0);

			if(pipe(pipes[i]) == -1) {
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
					printf("Error redirecting %s's stdin to pipe %d read\n", argv[i], i-1);
					return 1;
				}
				// closing previous pipe's read in child process
				close(pipes[i-1][0]);

				// redirecting stdout of child to pipe's write
				if(dup2(pipes[i][1], STDOUT_FILENO) == -1) {
					printf("Error redirecting %s's stdout to pipe %d's write\n", argv[i], i);
					return 1;
				}
				// close current pipe's write in child process
				close(pipes[i][1]);
				
				// run middle commands 
				returnval = system(argv[i+1]);
				if(returnval == -1) {
					fprintf(stderr, "%s returned -1", argv[i+1]);
					return -1;
				}
				else if(WIFEXITED(returnval)) {
					if(WEXITSTATUS(returnval) == 127) {
						fprintf(stderr, "Command not found or not executed: %s\n", argv[i+1]);
						return EINVAL;
					}
					else {
						return 0;
					}
				}
			}
			else {
				term_status = waitpid(pids[i], &status[i], 0);
				if(term_status > 0) {
					if (WIFEXITED(status[i])) {
						if(WEXITSTATUS(status[i]) == EINVAL) {
							fprintf(stderr,"process PID: %d exited with status: %d\n", term_status, WEXITSTATUS(status[i]));
							return(WEXITSTATUS(status[i]));
						}
					} else {
						printf("process PID: %d terminated abnormally\n", term_status);
						return -1;
					}
				} else {
					perror("waitpid returned incorrect value");
					return 1;
				}
			}
		}

		const int SECONDLASTPIPE = argc-4;
		const int LASTPIPE = argc-3;
		const int LASTPID = argc-2;

		// close second to last pipe's read in parent process
		close(pipes[SECONDLASTPIPE][0]);
		// close previous pipe's write in parent process
		close(pipes[LASTPIPE][1]);

		pids[LASTPID] = fork();
		if( pids[LASTPID] == -1 ) {
			printf("Error opening final process\n");
			return 1;
		}
		else if( pids[LASTPID] == 0 ) {
			if(dup2(pipes[LASTPIPE][0], STDIN_FILENO) == -1) {
				perror("Error redirecting last process's stdin to last pipe's read");
				return 1;
			}
			// close last pipe's read in child process
			close(pipes[LASTPIPE][0]);
			
			// Run last command
			returnval = system(argv[argc-1]);
			if(returnval == -1) {
				fprintf(stderr, "%s returned -1", argv[argc-1]);
				return -1;
			}
			else if(WIFEXITED(returnval)) {
				if(WEXITSTATUS(returnval) == 127) {
					fprintf(stderr, "Command not found or not executed: %s\n", argv[argc-1]);
					return EINVAL;
				}
				else {
					return 0;
				}
			}
		}
		else {
		term_status = waitpid(pids[LASTPID], &status[LASTPID], 0);
		if(term_status > 0) {
			if (WIFEXITED(status[LASTPID])) {
				if(WEXITSTATUS(status[LASTPID]) == EINVAL) {
					fprintf(stderr,"process PID: %d exited with status: %d\n", term_status, WEXITSTATUS(status[LASTPID]));
					return(WEXITSTATUS(status[LASTPID]));
				}
			} else {
				printf("process PID: %d terminated abnormally\n", term_status);
				return -1;
			}
		} else {
			perror("waitpid returned incorrect value");
			return 1;
		}
			// close last pipe's read in parent process
			close(pipes[LASTPIPE][0]);
		}
		
		free(pipes);
		free(pids);
		free(status);

		return 0;
	}
}
