#include <dirent.h> 
#include <stdio.h> 
#include <string.h>

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
  return(0);
}