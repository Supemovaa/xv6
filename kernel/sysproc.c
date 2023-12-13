#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


// #ifdef LAB_PGTBL
/**
 * @brief reports which pages have been accessed. this syscall requires 3 parameters
 * 
 * @param firstUserPage starting virtual address of the first user page to check
 * @param nPages the number of pages to check
 * @param outputAddr a user address to a buffer to store the results into a bitmask
 * 
 * @return int 
 */
int sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  struct proc *p = myproc();
  uint64 firstUserPage;
  int nPages;
  uint64 outputAddr;
  /* read in params */
  argaddr(0, &firstUserPage);
  argint(1, &nPages);
  argaddr(2, &outputAddr);
  uint ans = 0, pgcnt = 0;
  for (uint64 page = firstUserPage; page < firstUserPage + nPages * PGSIZE; page += PGSIZE){
    pte_t *pte_ptr = walk(p->pagetable, page, 0);
    // pte_t pte = *pte_ptr;
    /* find accessed pages */
    if(*pte_ptr & PTE_A){
      /* fill in the bitmap */
      ans |= (1 << pgcnt);
      /* clear access bit */
      *pte_ptr &= (~PTE_A);
    }
    pgcnt++;
  }
  /* copy out answer to use space */
  copyout(p->pagetable, outputAddr, (void *)&ans, sizeof(uint64));
  return 0;
}
// #endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
