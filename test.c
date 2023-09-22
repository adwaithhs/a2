#include <dirent.h> 
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <inttypes.h>

void *read_input(char *file, uint64_t ** p_data, uint64_t *p_n) {
    FILE *fs = fopen(file, "r");
    fscanf(fs, "%" SCNu64, p_n);
    *p_data = malloc(*p_n*sizeof(uint64_t));
    for (uint64_t i = 0; i < *p_n; i++)
    {
        fscanf(fs, "%" SCNu64, *p_data + i);
    }
}

int main(void) {
  DIR *d;
  struct dirent *dir;
  d = opendir("input");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG)
            printf("%s\n", dir->d_name);
    }
    closedir(d);
  }
  printf("%lu\n", strlen("test"));
  uint64_t *data;
  uint64_t n;
  read_input("input/in_0.txt", &data, &n);
  for (uint64_t i = 0; i < n; i++)
  {
    printf("%" PRIu64 "\n", data[i]);
  }
  
  return(0);
}