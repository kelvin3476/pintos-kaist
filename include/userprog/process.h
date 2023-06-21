#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "vm/vm.h"

//lazy_load_segment에 넘겨줄 인자들
struct vm_entry {
	struct file *f;			/* 가상 주소와 매핑된 파일 */
	off_t offset;			/* 읽어야할 파일 offset */
	uint32_t read_bytes;	/* 가상 페이지에 쓰여져 있는 데이터 크기 */
	uint32_t zero_bytes;	/* 0으로 채울 남은 페이지 byte */
};

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);
bool setup_stack (struct intr_frame *if_);

/* Argument Passing */
void argument_stack(char **parse, int count, void **esp);

/* System Call */
struct thread *get_child_process(int pid);

#endif /* userprog/process.h */