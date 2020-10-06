#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256
#define MAX_PASSWORD 10


#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void) {
  char user_id[MAXLINE];
  char password[MAXLINE];

  /* The user will type in a user name on one line followed by a password 
     on the next.
     DO NOT add any prompts.  The only output of this program will be one 
	 of the messages defined above.
   */

  if(fgets(user_id, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  if(fgets(password, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  } 
  int fd[2], r;
  if ((pipe(fd) == -1)){
  	perror("pipe");
	exit(1);
  }
  if((r = fork()) > 0){
  	if ((close(fd[0])) == -1){
		perror("close");
	}

	if (write(fd[1], user_id, MAX_PASSWORD) == -1){
		perror("write");
	}
	if (write(fd[1], password, MAX_PASSWORD) == -1){
		perror("write");
	}
	if((close(fd[1])) == -1){
		perror("close");
	}
	int status;
	if (wait(&status) != -1){
		if (WIFEXITED(status)){
			if (WEXITSTATUS(status) == 0){
				fprintf(stdout, "%s", SUCCESS);
			}else if (WEXITSTATUS(status) == 2){
				fprintf(stdout, "%s", INVALID);
			}else if (WEXITSTATUS(status) == 3){
				fprintf(stdout, "%s", NO_USER);
			}
		}
	}else{
		perror("wait");
		exit(1);
	}
  }else if (r == 0){
  	if ((dup2(fd[0], fileno(stdin))) == -1){
		perror("dup2");
		exit(1);
	}
	if ((close(fd[0])) == -1){
		perror("close");
	}
	if ((close(fd[1])) == -1){
		perror("close");
	}
	execl("./validate", "validate", NULL);
	fprintf(stderr, "ERROR: exec should not return \n");
  }else{
  	perror("fork");
	exit(1);
  }

  return 0;
}
