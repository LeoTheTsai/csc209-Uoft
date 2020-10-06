#include <stdio.h>

#include "benford_helpers.h"

int count_digits(int num) {
    // TODO: Implement.
    int total = 1;
    while(num > BASE - 1){
    	total += 1;
	num /= BASE;
    }

    return total;
}

int get_ith_from_right(int num, int i) {
    // TODO: Implement.
	int final;
	while(i >= 0){
		final = num % BASE;
		num /= BASE;
		i -= 1;
	}
	return final;
}

int get_ith_from_left(int num, int i) {
    // TODO: Implement.
	int total_dig = count_digits(num);
    	int right_dig = total_dig - i - 1;
	return get_ith_from_right(num, right_dig);	
}

void add_to_tally(int num, int i, int *tally) {
    // TODO: Implement.
    int update_index = get_ith_from_left(num, i);
    tally[update_index] += 1;
    return;
}
