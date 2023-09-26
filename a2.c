#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <dirent.h> 
#include <mpi.h>

// #define DT_REG 8

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

uint64_t find(uint64_t *S, uint64_t p, uint64_t x) {
    uint64_t l = 0;
    uint64_t r = p-1;
    while (l < r) {
        uint64_t m = l + (r - l) / 2;
        if (S[m] == x)
            return m;
        if (S[m] < x)
            l = m + 1;
        else
            r = m;
    }
    return r;

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
    uint64_t* real_splitters = malloc((p-1)*sizeof(uint64_t));
    if (rank == 0) {
        pseudo_splitters = malloc(p*p*sizeof(uint64_t));
    }
    MPI_Gather(ps, p, MPI_UINT64_T, pseudo_splitters, p, MPI_UINT64_T, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        qsort(pseudo_splitters, p*p, sizeof(uint64_t), comp);
        for (int i = 0; i < p - 1; i++) {
            real_splitters[i] = pseudo_splitters[(i+1)*p];
        }
    }
    MPI_Bcast(real_splitters, p-1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    
    uint64_t* part_sizes = malloc(p*sizeof(uint64_t));
    uint64_t prev = 0;
    for (int i = 0; i < p - 1; i++) {
        uint64_t j = find(data, n, real_splitters[i]);
        part_sizes[i] = j - prev;
        prev = j;
    }
    part_sizes[p-1] = n - prev;
    uint64_t* recv_sizes = malloc(p*sizeof(uint64_t));
    
    MPI_Alltoall(part_sizes, 1, MPI_UINT64_T, recv_sizes, 1, MPI_UINT64_T, MPI_COMM_WORLD);

    uint64_t m = 0;
    for (int i = 0; i < p; i++) {
        m += recv_sizes[i];
    }
    MPI_Request* reqs = malloc(p*sizeof(MPI_Request));
    MPI_Request* reqs2 = malloc(p*sizeof(MPI_Request));
    uint64_t* sorted = malloc(m*sizeof(uint64_t));
    uint64_t recv_start = 0;
    uint64_t send_start = 0;
    for (int other = 0; other < rank; other++) {
        MPI_Irecv(sorted + recv_start, recv_sizes[other], MPI_UINT64_T, other, 1, MPI_COMM_WORLD, reqs + other);
        MPI_Isend(data + send_start, part_sizes[other], MPI_UINT64_T, other, 1, MPI_COMM_WORLD, reqs2 + other);
        recv_start += recv_sizes[other];
        send_start += part_sizes[other];
    }

    memcpy(sorted + recv_start, data + send_start, part_sizes[rank]*sizeof(uint64_t));
    recv_start += recv_sizes[rank];
    send_start += part_sizes[rank];

    for (int other = rank + 1; other < p; other++) {
        MPI_Isend(data + send_start, part_sizes[other], MPI_UINT64_T, other, 1, MPI_COMM_WORLD, reqs2 + other);
        MPI_Irecv(sorted + recv_start, recv_sizes[other], MPI_UINT64_T, other, 1, MPI_COMM_WORLD, reqs + other);
        recv_start += recv_sizes[other];
        send_start += part_sizes[other];
    }
    
    for (int i = 0; i < p; i++) {
        if (i == rank) continue;
        MPI_Wait(reqs + i, MPI_STATUS_IGNORE);
        MPI_Wait(reqs2 + i, MPI_STATUS_IGNORE);
    }

    MPI_Finalize();
    return 0;
}
