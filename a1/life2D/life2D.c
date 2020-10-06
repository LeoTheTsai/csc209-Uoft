#include <stdio.h>
#include <stdlib.h>

#include "life2D_helpers.h"


int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: life2D rows cols states\n");
        return 1;
    }

    int rows = strtol(argv[1], NULL, 10);
    int cols = strtol(argv[2], NULL, 10);
    int num_times = strtol(argv[3], NULL, 10);
    int board[cols * rows];

    for(int i = 0; i < cols * rows; i++){
	fscanf(stdin, "%d", &board[i]);
    }

    for(int i = 0; i < num_times; i++){
	print_state(board, rows, cols);
	update_state(board, rows, cols);
    }
    return 0;
}
