#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"

double pipe_com(int fd[2], struct Point *p, int pdmax, int *pcount, int n);

/*
 * An error checking function for wait in parent. 
 */
void _wait_wrapper(int *status){
	if(wait(status) == -1){
		perror("wait");
		exit(1);
	}
}

/*
 * An error checking function for reading from pipe.
 */
void _read_pipe(int fd, double *min){
	if(read(fd, min, sizeof(*min)) == -1){
		perror("read");
		exit(1);
	}
}

/*
 * An error checking function for writing to the pipe.
 */
void _write_pipe(int fd, double min){
	if((write(fd, &min, sizeof(min))) == -1){
		perror("write");
		exit(1);
	}
}

/*
 * An error checking function for closing the pipe.
 */
void _close_pipe(int fd){
	if((close(fd)) == -1){
		perror("close");
		exit(1);
	}
}

/*
 * An error checking function for fork.
 */
int _fork(){
	int res;
	if((res = fork()) == -1){
		perror("fork");
		exit(1);
	}
	return res;
}

/*
 * An error checking function for pipe.
 */
void _pipe(int *fd){
	if(pipe(fd) == -1){
		perror("pipe");
		exit(1);
	}
}


/*
 * An error checking function for malloc. 
 */
void *_malloc(int size){
	void *ptr;
	if((ptr = malloc(size)) == NULL){
		perror("malloc");
		exit(1);
	}
	return ptr;
}

/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {
    if(n < 4 || pdmax == 0){
    	return closest_serial(p, n);
    }else{
	int midpoint = n / 2;
	struct Point mid_point = p[midpoint];
	int left_len = midpoint;
	int right_len = n - left_len;
	double left_min, right_min;
	int fd1[2], fd2[2];
	_pipe(fd1);
	_pipe(fd2);
	left_min = pipe_com(fd1, p, pdmax, pcount, left_len);
	right_min = pipe_com(fd2, p + midpoint, pdmax, pcount, right_len);
	double new_min = min(left_min, right_min);
	struct Point *strip = _malloc(sizeof(struct Point) * n);
	
	int j = 0;
	for (int i = 0; i < n; i++) {
		if (abs(p[i].x - mid_point.x) < new_min) {
			strip[j] = p[i], j++;
		}
	}
	new_min = min(new_min, strip_closest(strip, j, new_min));
    	free(strip);
	return new_min;
    }   
    return 0.0;
}


/*
 * Implementation of child and parent process where child recursively called on closest_parallet
 * wiht one less depth and send the min to the pipe, then parent receive the min from the pipe 
 * and update the pcount and return the minimum.
 */
double pipe_com(int fd[2], struct Point *p, int pdmax, int *pcount, int n){
    int r = _fork();
    if(r == 0){
	*pcount += 1;
	double min = closest_parallel(p, n, pdmax - 1, pcount);
	_close_pipe(fd[0]);
	_write_pipe(fd[1], min);
	_close_pipe(fd[1]);
	exit(*pcount);
    }else{
	double min;
	int status;
	_close_pipe(fd[1]);	
	_read_pipe(fd[0], &min);
	_close_pipe(fd[0]);	
	_wait_wrapper(&status);
	if (WIFEXITED(status)) {
            	*pcount = WEXITSTATUS(status);
        }
    	
	return min;
    }
}

