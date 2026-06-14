#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <elf.h> 


int i = 0;


void test_func(Elf32_Phdr *phdr, int index) {
    printf("Program header number %d at address %p\n", i, (void *)phdr);
    i++;
}

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;

    for (int idx = 0; idx < ehdr->e_phnum; idx++) {
        Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start +
                                          ehdr->e_phoff +
                                          idx * ehdr->e_phentsize);
        func(phdr, arg);
    }
    return ehdr->e_phnum;
}

int main(int argc, char** argv) {
    int fd;
    struct stat st;
    void *map_start;

    if (argc != 2) {
        perror("argv");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    foreach_phdr(map_start, test_func, i);

    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}
