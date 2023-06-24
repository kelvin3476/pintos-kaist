#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

// * USERPROG 추가
#include "threads/palloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

// * VM 추가
#include "vm/vm.h"
#include "threads/mmu.h"
#include "vm/file.h"

<<<<<<< Updated upstream
=======
#include "userprog/process.h"

>>>>>>> Stashed changes
void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {

	lock_init(&filesys_lock);

	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {

	thread_current()->rsp = f->rsp;

	// TODO: Your implementation goes here.
	switch (f->R.rax) {
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			memcpy(&thread_current()->ptf, f, sizeof(struct intr_frame));
			f->R.rax = fork(f->R.rdi);
			break;
		case SYS_CREATE:
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			f->R.rax = remove(f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize(f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_EXEC:
			exec(f->R.rdi);
			break;
		case SYS_WAIT:
			f->R.rax = wait(f->R.rdi);
			break;
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell(f->R.rdi);
			break;
		case SYS_CLOSE:
			close(f->R.rdi);
			break;
<<<<<<< Updated upstream
=======

		#ifdef VM

>>>>>>> Stashed changes
		case SYS_MMAP:
			f->R.rax = mmap(f->R.rdi, f->R.rsi, f->R.rdx, f->R.r10, f->R.r8);
			break;
		case SYS_MUNMAP:
			munmap(f->R.rdi);
			break;
<<<<<<< Updated upstream
=======

		#endif

>>>>>>> Stashed changes
		default:
			exit(-1);
			break;
	}
}

void 
halt(void) {
	power_off();
}

void 
exit(int status) {
	struct thread *cur = thread_current();
	cur->exit_status = status;
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();	
}

int
fork (const char *thread_name){
	check_address(thread_name);
	return process_fork(thread_name, &thread_current()->ptf);
}

int
exec (const char *file_name) {
	check_address(file_name);

	int file_size = strlen(file_name) + 1;
	char *fn_copy = palloc_get_page(PAL_ZERO);
	if (!fn_copy) {
		exit(-1);
		return -1;
	}
	strlcpy(fn_copy, file_name, file_size);
	if (process_exec(fn_copy) == -1) {
		exit(-1);
		return -1;
	}
}

int
wait (tid_t pid) {
	return process_wait(pid);
}

bool
create (const char *file, unsigned initial_size) {
	check_address(file);

	lock_acquire(&filesys_lock);
	bool result = filesys_create(file, initial_size);
	lock_release(&filesys_lock);

	return result;
}

bool
remove (const char *file) {
	check_address(file);

	lock_acquire(&filesys_lock);
	bool result = filesys_remove(file);
	lock_release(&filesys_lock);

	return result;
}

int
open (const char *file) {
	check_address(file);
	struct thread *cur = thread_current();
	lock_acquire(&filesys_lock);
	struct file *fd = filesys_open(file);
	if (fd) {
		for (int i = 2; i < 128; i++) {
			if (!cur->fdt[i]) {
				cur->fdt[i] = fd;
				cur->next_fd = i + 1;
				lock_release(&filesys_lock);
				return i;
			}
		}
		file_close(fd);
	}

	lock_release(&filesys_lock);
	return -1;
}

int
filesize (int fd) {
	struct file *file = thread_current()->fdt[fd];

	lock_acquire(&filesys_lock);

	if (file) {

		lock_release(&filesys_lock);
		return file_length(file);
	}

	lock_release(&filesys_lock);
	return -1;
<<<<<<< Updated upstream
}

int
read (int fd, void *buffer, unsigned size) {
	check_address(buffer);

	if (fd == 1) {
		return -1;
	}

	if (fd == 0) {
		lock_acquire(&filesys_lock);
		int byte = input_getc();
		lock_release(&filesys_lock);
		return byte;
	}
	struct file *file = thread_current()->fdt[fd];

	#ifdef VM
		struct page* page = spt_find_page(&thread_current()->spt, buffer);
		if (page->writable == 0) {
			exit(-1);
		}
	#endif

	if (file) {
		lock_acquire(&filesys_lock);
		int read_byte = file_read(file, buffer, size);
		lock_release(&filesys_lock);
		return read_byte;
	}
	return -1;
}

int
write (int fd UNUSED, const void *buffer, unsigned size) {
	check_address(buffer);

	if (fd == 0) { // STDIN 일때 -1
		return -1;
	}

	if (fd == 1) {
		lock_acquire(&filesys_lock);
		putbuf(buffer, size);
		lock_release(&filesys_lock);
		return size;
	}
	struct file *file = thread_current()->fdt[fd];
	if (file) {
		lock_acquire(&filesys_lock);
		int write_byte = file_write(file, buffer, size);
		lock_release(&filesys_lock);
		return write_byte;
	}
}

void
seek (int fd, unsigned position) {
	struct file *curfile = thread_current()->fdt[fd];
	if (curfile) {
		file_seek(curfile, position);
	}
}

unsigned
tell (int fd) {
	struct file *curfile = thread_current()->fdt[fd];
	if (curfile) {

		lock_acquire(&filesys_lock);
		return file_tell(curfile);
		lock_release(&filesys_lock);

	}
}

void
close (int fd) {
	struct file *file = thread_current()->fdt[fd];
	if (file) {
		lock_acquire(&filesys_lock);
		thread_current()->fdt[fd] = NULL;
		file_close(file);
		lock_release(&filesys_lock);
	}
}

void
check_address(void *addr) {
	if (addr == NULL || !is_user_vaddr(addr)) {
        exit(-1);
    } 

	// return spt_find_page(&thread_current()->spt, addr);
	// if (spt_find_page(&thread_current()->spt, (uint64_t)addr) == NULL) {
	// 	exit(-1);
	// }
	// struct thread *cur = thread_current();
	// if (addr == NULL || is_kernel_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL)
	// 	exit(-1);
	// if(!is_user_vaddr(addr)) {
    //     return false;
    // }
    // if (spt_find_page(&thread_current()->spt, addr)== NULL) {
    //     return false;
    // }
	// return true;
}
=======
}

int
read (int fd, void *buffer, unsigned size) {
	check_address(buffer);

	if (fd == 1) {
		return -1;
	}

	if (fd == 0) {
		lock_acquire(&filesys_lock);
		int byte = input_getc();
		lock_release(&filesys_lock);
		return byte;
	}
	struct file *file = thread_current()->fdt[fd];

	#ifdef VM
		struct page *page = spt_find_page(&thread_current()->spt, buffer);
		if (page && page->writable == 0) {
			exit(-1);
		}
	#endif

	if (file) {
		lock_acquire(&filesys_lock);
		int read_byte = file_read(file, buffer, size);
		lock_release(&filesys_lock);
		return read_byte;
	}
	return -1;
}

int
write (int fd UNUSED, const void *buffer, unsigned size) {
	check_address(buffer);

	if (fd == 0) { // STDIN 일때 -1
		return -1;
	}

	if (fd == 1) {
		lock_acquire(&filesys_lock);
		putbuf(buffer, size);
		lock_release(&filesys_lock);
		return size;
	}
	struct file *file = thread_current()->fdt[fd];
	if (file) {
		lock_acquire(&filesys_lock);
		int write_byte = file_write(file, buffer, size);
		lock_release(&filesys_lock);
		return write_byte;
	}
}

void
seek (int fd, unsigned position) {
	struct file *curfile = thread_current()->fdt[fd];
	if (curfile) {

		lock_acquire(&filesys_lock);
		file_seek(curfile, position);
		lock_release(&filesys_lock);

	}
}

unsigned
tell (int fd) {
	struct file *curfile = thread_current()->fdt[fd];
	if (curfile) {

		lock_acquire(&filesys_lock);
		return file_tell(curfile);
		lock_release(&filesys_lock);

	}
}

void
close (int fd) {
	struct file *file = thread_current()->fdt[fd];
	if (file) {
		lock_acquire(&filesys_lock);
		thread_current()->fdt[fd] = NULL;
		file_close(file);
		lock_release(&filesys_lock);
	}
}

void
check_address(void *addr) {
	if (addr == NULL || !is_user_vaddr(addr)) {
        exit(-1);
    } 

	// return spt_find_page(&thread_current()->spt, addr);
	// if (spt_find_page(&thread_current()->spt, (uint64_t)addr) == NULL) {
	// 	exit(-1);
	// }
	// struct thread *cur = thread_current();
	// if (addr == NULL || is_kernel_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL)
	// 	exit(-1);
	// if(!is_user_vaddr(addr)) {
    //     return false;
    // }
    // if (spt_find_page(&thread_current()->spt, addr)== NULL) {
    //     return false;
    // }
	// return true;
}

#ifdef VM
>>>>>>> Stashed changes

void *mmap (void *addr, size_t length, int writable, int fd, off_t offset) {

	if(!addr || addr != pg_round_down(addr)) { //addr이 존재하지 않거나 정렬되어 있지 않은 경우
		return NULL;
	}
	if (offset != pg_round_down(offset)) { //offset이 정렬되어 있지 않은 경우
		return NULL;
	}
    if (!is_user_vaddr(addr) || !is_user_vaddr(addr + length)) { //사용자 영역에 존재하지 않을 경우
		return NULL;
	}
    if (spt_find_page(&thread_current()->spt, addr)) { //addr에 할당된 페이지가 존재할 경우
		return NULL;
	}
    struct file *f = process_get_file(fd); //fd에 파일이 없을 경우
    if (f == NULL) {
		return NULL;
	}
    if (file_length(f) == 0 || (int)length <= 0) { //길이가 0이하일 경우
		return NULL;
	}

    return do_mmap(addr, length, writable, f, offset);
}

void munmap (void *addr) {
	do_munmap(addr);
}

/* fd가 가리키는 파일 반환하는 함수 */
struct file *process_get_file(int fd)
{
	struct file **fdt = thread_current()->fdt;

	if (fd < 2 || fd >= FDT_COUNT_LIMIT)
	{
		return NULL;
	}
	return fdt[fd];
<<<<<<< Updated upstream
}
=======
}

#endif
>>>>>>> Stashed changes
