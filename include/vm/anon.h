#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
struct page;
enum vm_type;

struct anon_page {
<<<<<<< Updated upstream
    vm_initializer *init;
	enum vm_type type;
	void *aux;
=======
    uint32_t slot;
>>>>>>> Stashed changes
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif
