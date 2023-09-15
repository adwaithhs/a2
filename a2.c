#include <stdio.h>
#include <inttypes.h>

uint64_t *read_input(char *file) {
    FILE *fs = fopen(file, "r");
    uint64_t n;
    fscanf(fs, "%" SCNu64, &n);
    uint64_t* data = malloc(n*sizeof(*data));
    for (uint64_t i = 0; i < n; i++)
    {
        fscanf(fs, "%" SCNu64, data + i);
    }
    return data;
}

int main(int argc, char const *argv[]) {
    /* code */
    return 0;
}
