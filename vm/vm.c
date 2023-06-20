/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
void clear_func (struct hash_elem *elem, void *aux);

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		struct page * page = (struct page*)malloc(sizeof(struct page)) ;
		bool (*initializer)(struct page *, enum vm_type, void *) ; 

		if (VM_TYPE(type) == VM_ANON) {
			initializer = anon_initializer;
		}
		else if (VM_TYPE(type) == VM_FILE) {
			initializer = file_backed_initializer;
		}
		else {
			goto err ;
		}
		uninit_new(page, upage, init, type, aux, initializer);
		page->writable = writable; 
		page->full_type = type;

		/* TODO: Insert the page into the spt. */
		bool res = spt_insert_page(spt, page);
		struct page *result = spt_find_page(spt, upage);
		if (result == NULL){
			goto err ;
		}
		return true ; 
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* TODO: Fill this function. */
	struct page p;
	struct hash_elem *e;

	p.va = pg_round_down(va);
	e = hash_find (&spt->hash, &p.hash_elem); 
	return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {

	struct hash_elem *result = hash_insert (&spt->hash, &page->hash_elem); // null이면 성공인 것 
	return result == NULL ? true : false ; 
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	struct hash_elem * result =	hash_delete (&spt->hash, &page->hash_elem);
	if (result == NULL) {
		return false; 
	}

	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	/* TODO: Fill this function. */
	uint64_t *kva = palloc_get_page(PAL_USER);

	if (kva == NULL) {
		 PANIC ("no memory. evict & swap out 필요 ");
	}

	struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
	frame->kva = kva ;
	frame->page = NULL ; 

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	while (!spt_find_page (spt, addr)) {
		vm_alloc_page (VM_ANON | VM_MARKER_0, addr, true);
		vm_claim_page (addr);
		addr += PGSIZE;
  }
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	void *page_addr = pg_round_down(addr); // 페이지 사이즈로 내려서 spt_find 해야 하기 때문 
	uint64_t addr_v = (uint64_t)addr;
	struct page *page = spt_find_page(spt, page_addr);
	uint64_t MAX_STACK = USER_STACK - (1<<20);

	uint64_t rsp = NULL;
	rsp = user ? f->rsp : thread_current()->rsp; 

	if (is_kernel_vaddr(addr)) 
		return false;

	if (!not_present && write)
    	return false;

	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	if (page == NULL) {
		if (addr_v > MAX_STACK && addr_v < USER_STACK && addr_v >= rsp -8) {
			vm_stack_growth(page_addr);
			page = spt_find_page(spt, page_addr);
		} else { 
			return false ; 
		}
	}

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	/* TODO: Fill this function */
	struct page *page = spt_find_page (&thread_current()->spt, va);
	if (page == NULL) {
		 return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	bool result = pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable); 
	if (result == false) {
		return false ; 
	}

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->hash, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst, struct supplemental_page_table *src) {
	struct hash *parent_hash = &src->hash ;
	struct hash *curr_hash = &dst->hash ; 

	struct hash_iterator i;
	hash_first (&i, parent_hash);
	while (hash_next (&i)) {
    	struct page *p = hash_entry (hash_cur (&i), struct page, hash_elem);
		enum vm_type type = page_get_type(p);	
		enum vm_type full_type = p->full_type;
		void *va = p-> va; 
		bool writable = p-> writable;

		if (p->operations->type == VM_UNINIT) {
		// 초기화 안 된 페이지
			vm_initializer *init = p->uninit.init; 
			struct aux_data *aux = malloc(sizeof(struct aux_data));
			aux = p->uninit.aux; 
			if(!vm_alloc_page_with_initializer(full_type, va, writable, init, aux))
				return false;
		} else {
			// 초기화된 페이지 (이미 load는 끝남)
			if (!vm_alloc_page(full_type, va, writable)) {
				return false; 
			}
			if (!vm_claim_page(va)) {
				return false;
			}

			memcpy(va, p->frame->kva, PGSIZE); // 실제 메모리 내용 복사     
		}
    }
	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear(&spt->hash, clear_func);
}

/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->va, sizeof p->va);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->va < b->va;
}

void clear_func (struct hash_elem *elem, void *aux) {
	struct page *page = hash_entry(elem, struct page, hash_elem);
	vm_dealloc_page(page);
}
