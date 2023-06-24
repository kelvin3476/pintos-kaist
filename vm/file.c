/* file.c: Implementation of memory backed file object (mmaped object). */

#include "threads/malloc.h"
#include <round.h>
#include "vm/vm.h"
#include "threads/mmu.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

bool lazy_load_seg(struct page *page, void *aux);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
	struct vm_entry *vme = (struct vm_entry *)page->uninit.aux;
	file_page->file = vme->f;
	file_page->offset = vme->offset;
	file_page->read_bytes = vme->read_bytes;
	file_page->zero_bytes = vme->zero_bytes;
	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	if(pml4_is_dirty(thread_current()->pml4, page->va)) {
		//변경사항을 파일에 저장하기
		file_write_at(file_page->file, page->va, file_page->read_bytes, file_page->offset);
		//dirty bit = 0
		pml4_set_dirty(thread_current()->pml4, page->va, 0);
	}
	pml4_clear_page(thread_current()->pml4, page->va);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {
	struct file *f = file_reopen(file);
	void *start_addr = addr;
	int total_page_count;
	if(length <= PGSIZE) { //현재 매핑하려는 길이가 페이지 사이즈보다 작을 경우
		total_page_count = 1; //한 개의 페이지에 모두 들어간다.
	}
	else { //하나 이상의 페이지가 필요할 경우
		if(length % PGSIZE != 0) {  //나누어떨어지지 않을 경우 페이지 1개 더 필요하다.
			total_page_count = length / PGSIZE + 1;
		}
		else {
			total_page_count = length / PGSIZE;
		}
	}

	size_t read_bytes = file_length(f) < length ? file_length(f) : length;
	size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;

	ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT(pg_ofs(addr) == 0);
    ASSERT(offset % PGSIZE == 0);

	while(read_bytes > 0 || zero_bytes > 0) {
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct file_page *file_page = (struct file_page *)malloc(sizeof(struct file_page));
		file_page->file = f;
		file_page->offset = offset;
		file_page->read_bytes = page_read_bytes;
		file_page->zero_bytes = page_zero_bytes;

		if(!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_seg, file_page)) {
			file_close(f);
			return NULL;
		}

		struct page *p = spt_find_page(&thread_current()->spt, start_addr);
		p->mapped_page_cnt = total_page_count;

		read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        addr += PGSIZE;
        offset += page_read_bytes;
	}
	return start_addr; 
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct supplemental_page_table *spt = &thread_current()->spt;
	struct page *p = spt_find_page(spt, addr);

	if (p->frame == NULL) {
		vm_claim_page(addr);
	}

	struct file *file = p->file.file;
	int count = p->mapped_page_cnt;

	for (int i = 0; i < count; i++) {
		if(p) {
			destroy(p);
		}
		addr += PGSIZE;
		p = spt_find_page(spt, addr);
	}

	file_close(file);
}

bool lazy_load_seg(struct page *page, void *aux)
{
   struct file_page *file_page = (struct file_page *)aux;
    page->file = (struct file_page) {
      .file = file_page->file,
      .offset = file_page->offset,
      .read_bytes = file_page->read_bytes,
      .zero_bytes = file_page->zero_bytes};

   file_seek(file_page->file, file_page->offset);

   if (file_read(file_page->file, page->frame->kva, file_page->read_bytes) != (int)(file_page->read_bytes))
   {
      palloc_free_page(page->frame->kva);
      return false;
   }
   // 3) 다 읽은 지점부터 zero_bytes만큼 0으로 채운다.
   memset(page->frame->kva + file_page->read_bytes, 0, file_page->zero_bytes);
//    free(file_page);
   return true;
}
