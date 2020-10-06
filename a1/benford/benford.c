#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }

    int num_arr[10] = {0};
    FILE *filename;
    int error;
    int index = strtol(argv[1], NULL, 10);
    filename = fopen(argv[2], "r");

    if (filename == NULL){
	filename = stdin;
    }
    int input;
    while(fscanf(filename, "%d", &input) == 1){
    	add_to_tally(input, index, num_arr);
    }


    error = fclose(filename);
    if(error != 0){
    	fprintf(stderr, "fclose failed on input file\n");
	return 1;
    }

   for (int i = 0; i < BASE; i++){
   	printf("%ds: %d\n", i, num_arr[i]);
   }

   return 0;
}
