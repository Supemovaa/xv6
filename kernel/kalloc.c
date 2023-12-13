// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

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

/**
 * @brief struct to record refcounts
 * 
 */
typedef struct {
  struct spinlock lock;
  uint8 refCount;
} Reference;
/* record user space */
Reference refer[(PHYSTOP - KERNBASE) / PGSIZE + 10];
#define PG_IDX(pa) ((pa - KERNBASE) / PGSIZE)

/**
 * @brief add one reference to the page which pa refers to
 * 
 * @param pa 
 * @return int 
 */
int addRefCount(uint64 pa){
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return -1;
  acquire(&(refer[PG_IDX(pa)].lock));
  refer[PG_IDX(pa)].refCount++;
  release(&(refer[PG_IDX(pa)].lock));
  return 0;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // init reference locks
  for (int i = 0; i < (PHYSTOP - KERNBASE) / PGSIZE + 10; i++)
    initlock(&(refer[i].lock), "reference");
  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  // init refcounts
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    refer[PG_IDX((uint64)p)].refCount = 0;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // reduce a refcount on the page. if refcount reaches 0, free the whole page
  acquire(&(refer[PG_IDX((uint64)pa)].lock));
  if(refer[PG_IDX((uint64)pa)].refCount > 0)
    refer[PG_IDX((uint64)pa)].refCount -= 1;
  if(refer[PG_IDX((uint64)pa)].refCount != 0){
    release(&(refer[PG_IDX((uint64)pa)].lock));
    return;
  }
  release(&(refer[PG_IDX((uint64)pa)].lock));

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
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
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    // set the refcount of the new page to 1
    acquire(&(refer[PG_IDX((uint64)r)].lock));
    refer[PG_IDX((uint64)r)].refCount = 1;
    release(&(refer[PG_IDX((uint64)r)].lock));
  }

  return (void*)r;
}
