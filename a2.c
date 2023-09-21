#include <stdio.h>
#include <inttypes.h>
#include <dirent.h> 
#include <mpi.h>

#define DT_REG 8

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

int main(int argc, char** argv) {
    int rank, size, message_Item;

    char input_file[] = "input/";
    char output_file[] = "output/";

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        DIR *d;
        struct dirent *dir;
        d = opendir(input_file);
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                //file = dir->d_name;
            }
        }
        closedir(d);
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
