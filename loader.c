#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <elf.h> 

// typedef struct {
// Elf32_Word p_type; /* entry type */
// Elf32_Off p_offset; /* file offset */
// Elf32_Addr p_vaddr; /* virtual address */
// Elf32_Addr p_paddr; /* physical address */
// Elf32_Word p_filesz; /* file size */
// Elf32_Word p_memsz; /* memory size */
// Elf32_Word p_flags; /* entry flags */
// Elf32_Word p_align; /* memory/file alignment */
// } Elf32_Phdr;

int i = 0;
int mflags = MAP_PRIVATE | MAP_FIXED;

int elf_flag_to_mmap_prot(int p_flags) {
    int prot = 0;
    if (p_flags & PF_R) prot |= PROT_READ;
    if (p_flags & PF_W) prot |= PROT_WRITE;
    if (p_flags & PF_X) prot |= PROT_EXEC;
    return prot;
}


char *phdr_type_to_str(Elf32_Word t) {
    switch (t) {
        case PT_NULL:   return "NULL";
        case PT_LOAD:   return "LOAD";
        case PT_DYNAMIC:return "DYNAMIC";
        case PT_INTERP: return "INTERP";
        case PT_NOTE:   return "NOTE";
        case PT_PHDR:   return "PHDR";
        case PT_TLS:    return "TLS";
        default:        return "OTHER";
    }
}

char *flags_to_str(Elf32_Word f) {
     static char str[4];
    int idx = 0;

    if (f & PF_R) str[idx++] = 'R';
    if (f & PF_W) str[idx++] = 'W';
    if (f & PF_X) str[idx++] = 'E';   /* or 'X' if you prefer */

    str[idx] = '\0';
    return str;
}



void test_func(Elf32_Phdr *phdr, int index) {
    printf("Program header number %d at address %p\n", i, (void *)phdr);
    i++;
}

void readelf_l(Elf32_Phdr *phdr, int index) {
    int prot = elf_flag_to_mmap_prot(phdr->p_flags);
    int map_flags = (phdr->p_type == PT_LOAD) ? mflags : 0;

    printf("%s 0x%06x 0x%08x 0x%08x "
           " 0x%05x 0x%05x %s 0x%x prot=0x%x mflags=0x%x\n",
           phdr_type_to_str(phdr->p_type),
           phdr->p_offset,
           phdr->p_vaddr,
           phdr->p_paddr,
           phdr->p_filesz,
           phdr->p_memsz,
           flags_to_str(phdr->p_flags),
           phdr->p_align,
           prot,
           map_flags);
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

    foreach_phdr(map_start, readelf_l, i);

    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}
