#include "../include/utils.h"
#include <stdlib.h>

// assign random color to vertex
vertex_t get_random_vertex(void) {
    int lower = 0;
    int upper = 2;

    return ((vertex_t) (random() % (upper + 1 - lower) + lower));
}
