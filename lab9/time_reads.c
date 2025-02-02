/* The purpose of this program is to practice writing signal handling
 * functions and observing the behaviour of signals.
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

/* Message to print in the signal handling function. */
#define MESSAGE "%ld reads were done in %ld seconds.\n"

/* Global variables to store number of read operations and seconds elapsed. 
 */
long num_reads, seconds;

void err(int sig){
	fprintf(stderr, MESSAGE, num_reads, seconds);
	exit(0);
}
/* The first command-line argument is the number of seconds to set a timer to run.
 * The second argument is the name of a binary file containing 100 ints.
 * Assume both of these arguments are correct.
 */

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: time_reads s filename\n");
        exit(1);
    }
    seconds = strtol(argv[1], NULL, 10);
    num_reads = 0;
    FILE *fp;
    if ((fp = fopen(argv[2], "r")) == NULL) {
      perror("fopen");
      exit(1);
    }
    struct sigaction sa;
    sa.sa_handler = err;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPROF, &sa, NULL);

    struct itimerval new;
    signal(SIGALRM, err);
    new.it_interval.tv_usec = 0;
    new.it_interval.tv_sec = 0;
    new.it_value.tv_usec = 0;
    new.it_value.tv_sec = seconds;
    if(setitimer(ITIMER_PROF, &new, NULL) == -1){
    	perror("setitimer");
	exit(1);
    }
    


    /* In an infinite loop, read an int from a random location in the file,
     * and print it to stderr.
     */
    for (;;) {
	int location = rand() % 100;
	int value;
	fseek(fp, location * sizeof(int), SEEK_SET);
	fread(&value, sizeof(int), 1, fp);
	num_reads += 1;
	fprintf(stderr, "%d\n", value);
    }
    return 1; // something is wrong if we ever get here!
}
