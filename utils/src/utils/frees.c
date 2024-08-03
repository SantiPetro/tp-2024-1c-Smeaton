#include "../include/frees.h"

void string_split_free(char*** arr_ptr) {
    char** arr = *arr_ptr;

    for(uint8_t i = 0; arr[i] != NULL; i++)
        free(arr[i]);
    free(*arr_ptr);
}