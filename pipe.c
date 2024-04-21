#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <errno.h>

int main(int argc, char *argv[])
{	
	if(argc == 1) {					// 0 ARG PIPE
		printf("Usage: ./pipe <command> ... <command>\n");
		return 1;
	}
	if(argc == 2) {					// 1 ARG PIPE
		system(argv[1]);
		return 0;
	}

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
			system(argv[1]);
			return 0;
		}
		else {
			// wait for first child process to finish
			waitpid(pids[0], &status[0], 0);

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
				// testing that write is closed
				close(pipefd[1]);

				// run the second arg
				system(argv[2]);
				return 0;
			}
			else {
				// wait for RHS child process to finish 
				waitpid(pids[1], &status[1], 0);
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
			// waitpid(pids[i-1], &status[i-1], 0);

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
				system(argv[i+1]);
				return 0;
			}
		}

		// wait for previous process to finish
		waitpid(pids[argc-2], &status[argc-2], 0);

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


