// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

struct page_ref{
  int count;
  struct spinlock lock;
};

struct page_ref ref_count[32768]; //(PHYSTOP-KERNBASE)/PGSIZE, [(KERNBASE + 128*1024*1024) - KERNBASE] / 4096

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  for(int i = 0; i < (PHYSTOP - KERNBASE)/PGSIZE; i++){
    initlock(&(ref_count[i].lock), "ref_count");
    //ref_count[((uint64)pa - KERNBASE)/PGSIZE].count = 1;  //no pa
  }
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    ref_count[((uint64)p - KERNBASE)/PGSIZE].count = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  uint64 i = ((uint64)pa - KERNBASE)/PGSIZE;
  acquire(&ref_count[i].lock);
  if(ref_count[i].count == 0){
    panic("kfree: too much times");
  } 
  
  ref_count[i].count --;
  if(ref_count[i].count != 0){
    release(&ref_count[i].lock);
    return;
  }
  else{
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  release(&ref_count[i].lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
  {
    uint64 i = ((uint64)r - KERNBASE) / PGSIZE;
    acquire(&ref_count[i].lock);
    ref_count[i].count = 1;
    release(&ref_count[i].lock);
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void add_ref_count(void* pa)
{
   acquire(&ref_count[((uint64)pa - KERNBASE)/PGSIZE].lock);
   ref_count[((uint64)pa - KERNBASE)/PGSIZE].count++;
   release(&ref_count[((uint64)pa - KERNBASE)/PGSIZE].lock);
}

void dec_ref_count(void *pa)
{
   acquire(&ref_count[((uint64)pa - KERNBASE)/PGSIZE].lock);
   ref_count[((uint64)pa - KERNBASE)/PGSIZE].count--;
   release(&ref_count[((uint64)pa - KERNBASE)/PGSIZE].lock);
}

int get_ref_count(void *pa)
{
  return ref_count[((uint64)pa - KERNBASE)/PGSIZE].count;
}