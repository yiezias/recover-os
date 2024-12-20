#include "exec.h"
#include "debug.h"
#include "file.h"
#include "global.h"
#include "memory.h"
#include "string.h"

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef int64_t Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;

#define ET_EXEC 2    /* Executable file */
#define EM_X86_64 62 /* AMD x86-64 architecture */

/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct {
	unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
	Elf64_Half e_type;		  /* Object file type */
	Elf64_Half e_machine;		  /* Architecture */
	Elf64_Word e_version;		  /* Object file version */
	Elf64_Addr e_entry;		  /* Entry point virtual address */
	Elf64_Off e_phoff;		  /* Program header table file offset */
	Elf64_Off e_shoff;		  /* Section header table file offset */
	Elf64_Word e_flags;		  /* Processor-specific flags */
	Elf64_Half e_ehsize;		  /* ELF header size in bytes */
	Elf64_Half e_phentsize;		  /* Program header table entry size */
	Elf64_Half e_phnum;		  /* Program header table entry count */
	Elf64_Half e_shentsize;		  /* Section header table entry size */
	Elf64_Half e_shnum;		  /* Section header table entry count */
	Elf64_Half e_shstrndx; /* Section header string table index */
} Elf64_Ehdr;

typedef struct {
	Elf64_Word p_type;    /* Segment type */
	Elf64_Word p_flags;   /* Segment flags */
	Elf64_Off p_offset;   /* Segment file offset */
	Elf64_Addr p_vaddr;   /* Segment virtual address */
	Elf64_Addr p_paddr;   /* Segment physical address */
	Elf64_Xword p_filesz; /* Segment size in file */
	Elf64_Xword p_memsz;  /* Segment size in memory */
	Elf64_Xword p_align;  /* Segment alignment */
} Elf64_Phdr;

#define PT_LOAD 1 /* Loadable program segment */


static void segment_load(size_t fd, size_t offset, size_t filesz,
			 size_t vaddr) {
	size_t page_start = vaddr & (~0xfff);
	size_t page_end = PG_SIZE * DIV_ROUND_UP((vaddr + filesz), PG_SIZE);
	size_t pg_cnt = (page_end - page_start) / PG_SIZE;
	for (size_t i = 0; i != pg_cnt; ++i) {
		page_map(page_start + i * PG_SIZE);
	}
	sys_lseek(fd, offset, SEEK_SET);
	sys_read(fd, (void *)vaddr, filesz);
}

static ssize_t elf_parse(const char *pathname, struct addr_space *addr_space) {
	ssize_t fd = sys_open(pathname);
	if (fd < 0) {
		return fd;
	}

	ssize_t ret = 0;

	Elf64_Ehdr elf_header;
	ssize_t nbyte = sys_read(fd, &elf_header, sizeof(Elf64_Ehdr));
	if (nbyte != sizeof(Elf64_Ehdr)) {
		ret = -sizeof(Elf64_Ehdr);
		goto done;
	}

	Elf64_Phdr prog_header;
	if (memcmp(elf_header.e_ident, "\177ELF\2\1\1", 7)
	    || elf_header.e_type != ET_EXEC || elf_header.e_machine != EM_X86_64
	    || elf_header.e_version != 1
	    || elf_header.e_phentsize != sizeof(Elf64_Phdr)) {
		ret = -sizeof(Elf64_Ehdr);
		goto done;
	}
	size_t load_idx = 0;
	for (size_t prog_idx = 0; prog_idx != elf_header.e_phnum; ++prog_idx) {
		sys_lseek(fd,
			  elf_header.e_phoff
				  + prog_idx * elf_header.e_phentsize,
			  SEEK_SET);
		if (sys_read(fd, &prog_header, elf_header.e_phentsize)
		    != elf_header.e_phentsize) {
			ret = -sizeof(Elf64_Phdr);
			goto done;
		}
		if (PT_LOAD == prog_header.p_type) {
			/* memsz会大于等于filesz，
			 * 如果有一段不用的全局内存
			 * filesz就会等于零 */
			segment_load(fd, prog_header.p_offset,
				     prog_header.p_memsz, prog_header.p_vaddr);
			addr_space->filesz[load_idx] = prog_header.p_memsz;
			addr_space->vaddr[load_idx++] = prog_header.p_vaddr;

			size_t *oldbrk = &addr_space->heap_start;
			size_t newbrk =
				prog_header.p_vaddr + prog_header.p_memsz;
			*oldbrk = *oldbrk > newbrk ? *oldbrk : newbrk;
		}
	}
	addr_space->heap_end = addr_space->heap_start;
	ASSERT(load_idx <= 4);
	ret = elf_header.e_entry;

done:
	sys_close(fd);
	return ret;
}

ssize_t sys_execv(char *pathname, const char *argv[]) {
	uint64_t rbp;
	asm volatile("movq (%%rbp),%0" : "=a"(rbp));

	struct task_struct *cur_task = running_task();
	cur_task->addr_space.heap_start = 0;
	ssize_t entry_point = elf_parse(pathname, &cur_task->addr_space);
	if (entry_point < 0) {
		return entry_point;
	}
	*(uint64_t *)(rbp - 16) = entry_point;

	uint64_t argc = 0;
	while (argv[argc]) {
		++argc;
	}
	asm volatile("movq %0,%%rdi;movq %1,%%rsi" ::"g"(argc), "g"(argv)
		     : "memory");
	asm volatile("movq %0,%%rbp;jmp system_ret" ::"g"(rbp) : "memory");
	return 0;
}
