#include <stdio.h>
#include <stdlib.h>

int check_turn(int total, int value){
    if(value == 1){
	if(total < 2 || total > 3){
	    return 1;
	}
    }else{
	if(total == 2 || total == 3){
	    return 1;
	}
    }
    return 0;
}

int num_neighbor(int *board, int current_index, int num_cols){
    int total = 0;
    if(board[current_index - 1] == 1){
    total += 1;
    }
    if(board[current_index + 1] == 1){
    total += 1;
    }
    if(board[current_index - num_cols] == 1){
    total += 1;
    }
    if(board[current_index - num_cols - 1] == 1){
    total += 1;
    }
    if(board[current_index - num_cols + 1] == 1){
    total += 1;
    }
    if(board[current_index + num_cols] == 1){
    total += 1;
    }
    if(board[current_index + num_cols - 1] == 1){
    total += 1;
    }
    if(board[current_index + num_cols + 1] == 1){
    total += 1;
    }
    return total;
}



void print_state(int *board, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows * num_cols; i++) {
        printf("%d", board[i]);
        if (((i + 1) % num_cols) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void update_state(int *board, int num_rows, int num_cols) {
    int change_state[num_rows * num_cols]; 
    for(int row = 2; row < num_rows; row ++){
	for(int col = 2; col < num_cols; col++){
	    int current_index = num_cols * (row - 1) + col - 1;
	    int total_neighbor = num_neighbor(board, current_index, num_cols);
	    change_state[current_index] = check_turn(total_neighbor, board[current_index]);    
	}
    }

    for(int row = 2; row < num_rows; row++){
	for(int col = 2; col < num_cols; col++){
	    int current_index = num_cols * (row - 1) + col - 1;
	    if(board[current_index] == 1 && change_state[current_index]){
		board[current_index] = 0;
	    }
	    else if(board[current_index] == 0 && change_state[current_index]){
		board[current_index] = 1;
	    }
	}
    }
    
    return;
}








