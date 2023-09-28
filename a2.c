#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <dirent.h> 
#include <mpi.h>
#include <getopt.h>

#define DT_REG 8

int comp(const void *elem1, const void *elem2) 
{
    uint32_t f = *((uint32_t*)elem1);
    uint32_t s = *((uint32_t*)elem2);
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
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

char* join(char *path, char *file) {
    int n = strlen(path);
    int m = strlen(file);
    char *ret = malloc((n+m+2)*sizeof(char));
    memcpy(ret, path, n*sizeof(char));
    if (path[n-1] == '/') {
        memcpy(ret + n, file, (m+1)*sizeof(char));
    } else {
        ret[n] = '/';
        memcpy(ret + n + 1, file, (m+1)*sizeof(char));
    }
    return ret;
}

int main(int argc, char** argv) {
    static struct option long_options[] = {
        {"inputpath",  required_argument, 0, 'i'},
        {"outputpath",  required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };
    int rank, p;
    char _i[] = "input";
    char _o[] = "output";
    char *input_path = _i;
    char *output_path = _o;
    while (1) {
        int c = getopt_long(argc, argv, "", long_options, NULL);
        if (c == -1) break;
        switch (c)
        {
        case 'i':
            input_path = malloc((strlen(optarg)+1)*sizeof(char));
            strcpy(input_path, optarg);
            break;
        case 'o':
            output_path = malloc((strlen(optarg)+1)*sizeof(char));
            strcpy(output_path, optarg);
            break;
        
        default:
            break;
        }
    }

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
                file = join(input_path, dir->d_name);
                break;
            }
            i++;
        }
    }
    closedir(d);
    uint64_t* data;
    uint64_t n;

    FILE *fs = fopen(file, "r");
    fscanf(fs, "%" SCNu64, &n);
    data = malloc(n*sizeof(uint64_t));
    for (uint64_t i = 0; i < n; i++) {
        fscanf(fs, "%" SCNu64, data + i);
    }
    fclose(fs);

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

    uint64_t *data_starts = malloc(p*sizeof(uint64_t));
    uint64_t *srtd_starts = malloc(p*sizeof(uint64_t));
    data_starts[0] = 0;
    srtd_starts[0] = 0;
    for (int i = 1; i < p; i++) {
        data_starts[i] = data_starts[i-1] + part_sizes[i-1];
        srtd_starts[i] = srtd_starts[i-1] + recv_sizes[i-1];
    }
    uint64_t m = srtd_starts[p-1] + recv_sizes[p-1];
    
    MPI_Request *reqs = malloc(p*sizeof(MPI_Request));
    MPI_Request *reqs2 = malloc(p*sizeof(MPI_Request));
    uint64_t *srtd = malloc(m*sizeof(uint64_t));

    for (int other = 0; other < rank; other++) {
        MPI_Irecv(srtd + srtd_starts[other], recv_sizes[other], MPI_UINT64_T, other, 1, MPI_COMM_WORLD, reqs + other);
        MPI_Isend(data + data_starts[other], part_sizes[other], MPI_UINT64_T, other, 1, MPI_COMM_WORLD, reqs2 + other);
    }

    for (int other = rank + 1; other < p; other++) {
        MPI_Isend(data + data_starts[other], part_sizes[other], MPI_UINT64_T, other, 1, MPI_COMM_WORLD, reqs2 + other);
        MPI_Irecv(srtd + srtd_starts[other], recv_sizes[other], MPI_UINT64_T, other, 1, MPI_COMM_WORLD, reqs + other);
    }

    memcpy(srtd + srtd_starts[rank], data + data_starts[rank], part_sizes[rank]*sizeof(uint64_t));

    for (int i = 0; i < p; i++) {
        if (i == rank) continue;
        MPI_Wait(reqs + i, MPI_STATUS_IGNORE);
        MPI_Wait(reqs2 + i, MPI_STATUS_IGNORE);
    }
    uint64_t *final = malloc(m*sizeof(uint64_t));
    uint64_t *idx = malloc(p*sizeof(uint64_t));
    for (int i = 0; i < p; i++) {
        idx[i] = 0;
    }
    for (uint64_t j = 0; j < m; j++) {
        uint64_t min = UINT64_MAX;
        int min_pos = -1;
        for (int i = 0; i < p; i++) {
            if (idx[i] < recv_sizes[i]) {
                uint64_t temp = srtd[srtd_starts[i] + idx[i]];
                if (temp <= min) {
                    min = temp;
                    min_pos = i;
                }
            }
        }
        final[j] = min;
        idx[i]++;
    }
    char outfile[20];
    sprintf(outfile, "out_%d.txt", rank);

    
    fs = fopen(join(output_path, outfile), "w");
    fprintf(fs, "%" PRIu64 "\n", m);
    for (uint64_t i = 0; i < m; i++) {
        fprintf(fs, "%" PRIu64 "\n", final[i]);
    }
    fclose(fs);

    MPI_Finalize();
    return 0;
}
