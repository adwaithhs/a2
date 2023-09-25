#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <mpi.h>

int main(int argc, char** argv) {
    int rank, size, message_Item;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int* a = malloc(size*sizeof(int));
    int* b = malloc(size*sizeof(int));
    int x = rank*size;
    for (int i = 0; i < size; i++) {
        a[i] = x+i;
    }

    for (int i = 0; i < size; i++) {
        if (rank == i) {
            for (int j = 0; j < size; j++) {
                printf("%d ", a[j]);
            }
            printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Alltoall(a, 1, MPI_INT, b, 1, MPI_INT, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < size; i++) {
        if (rank == i) {
            for (int j = 0; j < size; j++) {
                printf("%d ", b[j]);
            }
            printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
