#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <dirent.h> 
#include <mpi.h>

#define DT_REG 8

int comp(const void *elem1, const void *elem2) 
{
    uint32_t f = *((uint32_t*)elem1);
    uint32_t s = *((uint32_t*)elem2);
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
}

void *read_input(char *file, uint64_t ** p_data, uint64_t *p_n) {
    FILE *fs = fopen(file, "r");
    fscanf(fs, "%" SCNu64, p_n);
    *p_data = malloc(*p_n*sizeof(uint64_t));
    for (uint64_t i = 0; i < *p_n; i++)
    {
        fscanf(fs, "%" SCNu64, *p_data + i);
    }
}

int main(int argc, char** argv) {
    int rank, p, message_Item;

    char input_path[] = "input/";
    char output_path[] = "output/";

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    DIR *d;
    struct dirent *dir;
    int i = 0;
    char *file = NULL;
    d = opendir(input_path);
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            if (i == rank) {
               file = dir->d_name;
               break;
            }
            i++;
        }
    }
    closedir(d);
    uint64_t* data;
    uint64_t n;
    read_input(file, &data, &n);

    qsort(data, n, sizeof(uint64_t), comp);
    uint64_t* ps = malloc(p*sizeof(uint64_t));
    uint64_t* pseudo_splitters;
    if (rank == 0) {
        pseudo_splitters = malloc(p*p*sizeof(uint64_t));
    }
    MPI_Gather(ps, p, MPI_INT64_T, pseudo_splitters, p, MPI_INT64_T, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        qsort(pseudo_splitters, p*p, sizeof(uint64_t), comp);
    }

    if(rank == 0){
        message_Item = 42;
        MPI_Send(&message_Item, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
        printf("Message Sent: %d\n", message_Item);
    }

    else if(rank == 1){
        MPI_Recv(&message_Item, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Message Received: %d\n", message_Item);
    }

    MPI_Finalize();
    return 0;
}
