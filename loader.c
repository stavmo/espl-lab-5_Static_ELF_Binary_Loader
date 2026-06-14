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


extern int startup(int argc, char **argv, void (*start)());

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
    if (f & PF_X) str[idx++] = 'E';

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

//task 2b
void load_phdr(Elf32_Phdr *phdr, int fd) {
    if (phdr->p_type != PT_LOAD) {
        return; 
    }

    // Print the segment information
    readelf_l(phdr, 0);

    // Calculate system page-aligned memory boundaries
    unsigned int page_size = 0x1000;
    unsigned int aligned_vaddr = phdr->p_vaddr & ~(page_size - 1);
    unsigned int offset_diff = phdr->p_vaddr - aligned_vaddr;
    unsigned int aligned_offset = phdr->p_offset - offset_diff;
    unsigned int aligned_memsz = phdr->p_memsz + offset_diff;

    // Call mmap to allocate and map the segment into memory
    void *map_address = mmap(
        (void *)aligned_vaddr, 
        aligned_memsz, 
        elf_flag_to_mmap_prot(phdr->p_flags), 
        mflags, 
        fd, 
        aligned_offset
    );

    if (map_address == MAP_FAILED) {
        perror("mmap failed within load_phdr");
        exit(1);
    }

    // Allocate and zero out the uninitialized global data (.bss)
    if (phdr->p_memsz > phdr->p_filesz) {
        unsigned int bss_start = phdr->p_vaddr + phdr->p_filesz;
        unsigned int bss_size = phdr->p_memsz - phdr->p_filesz;
        
        char *bss = (char *)bss_start;
        for (unsigned int k = 0; k < bss_size; k++) {
            bss[k] = 0;
        }
    }
}


int main(int argc, char** argv) {
    int fd;
    struct stat st;
    void *map_start;

    // Change constraint check to accept subsequent args passed to your loaded binary
    if (argc < 2) { 
        fprintf(stderr, "Usage: %s <elf_executable> [args...]\n", argv[0]);
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

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || 
        ehdr->e_ident[EI_MAG1] != ELFMAG1 || 
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || 
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Error: Not a valid ELF binary.\n");
        munmap(map_start, st.st_size);
        close(fd);
        return 1;
    }

    for (int idx = 0; idx < ehdr->e_phnum; idx++) {
        Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff + (idx * ehdr->e_phentsize));
        if (phdr->p_type == PT_INTERP || phdr->p_type == PT_DYNAMIC) {
            fprintf(stderr, "Error: Dynamic binaries are unsupported.\n");
            munmap(map_start, st.st_size);
            close(fd);
            return 1;
        }
    }

    printf("Type Offset VirtAddr PhysAddr FileSiz MemSiz Flg Align\n");
    foreach_phdr(map_start, readelf_l, 0);
    printf("\n--- Loading Program Segments ---\n");

    // Pass the active file descriptor (fd) context as the third argument 
    foreach_phdr(map_start, (void (*)(Elf32_Phdr *, int))load_phdr, fd);


    //task 2c
    void (*entry_point)() = (void (*)())ehdr->e_entry;

    startup(argc - 1, argv + 1, entry_point);

    // Fallback cleanup if needed
    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}
