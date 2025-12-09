#include "malloc.h"
#include "mem_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Adds a string to the dynamic list, reallocating as needed
char **add_string(char **list, int *size, int *capacity, const char *str) {
    if (*size >= *capacity) {
        *capacity = (*capacity == 0) ? 2 : (*capacity * 2);
        char **tmp = realloc(list, *capacity * sizeof(char *));
        if (!tmp) {
            perror("Realloc failed");
            // Free old list to avoid leak if realloc fails
            for (int i = 0; i < *size; i++) free(list[i]);
            free(list);
            exit(1);
        }
        list = tmp;
    }
    list[*size] = malloc(strlen(str) + 1);
    if (!list[*size]) {
        perror("Malloc failed");
        // Cleanup on failure
        for (int i = 0; i < *size; i++) free(list[i]);
        free(list);
        exit(1);
    }
    strcpy(list[*size], str);
    (*size)++;
    return list;
}

void print_list(char **list, int size) {
    printf("List contents (%d items):\n", size);
    for (int i = 0; i < size; i++) {
        printf("  [%d] %s\n", i, list[i]);
    }
}

void free_list(char **list, int size) {
    for (int i = 0; i < size; i++) {
        free(list[i]);
    }
    free(list);
}

int main() {
    char **string_list = NULL;
    int size = 0, capacity = 0;

    string_list = add_string(string_list, &size, &capacity, "Hello");
    string_list = add_string(string_list, &size, &capacity, "Dynamic");
    string_list = add_string(string_list, &size, &capacity, "Memory");
    string_list = add_string(string_list, &size, &capacity, "Management");
    string_list = add_string(string_list, &size, &capacity, "In");
    string_list = add_string(string_list, &size, &capacity, "C");

    print_list(string_list, size);

    // Let's simulate removing last 2 items by shrinking size and reallocating smaller
    size -= 2;
    char **tmp = realloc(string_list, size * sizeof(char *));
    if (tmp) {
        string_list = tmp;
    }
    print_list(string_list, size);

    free_list(string_list, size);

	mem_logger_dump();

    return 0;
}
