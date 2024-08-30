#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
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


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

#ifdef LAB_PGTBL
extern pte_t * walk(pagetable_t, uint64, int);
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 bit_mask = 0;
  
  uint64 start_va;
  int pages_num;
  uint64 abits;
  
  if(argaddr(0, &start_va) < 0) return -1;
  if(argint(1, &pages_num) < 0) return -1;
  if(argaddr(2, &abits) < 0) return -1;

  //check pages num
  if(pages_num > 32) return -1;

  pte_t *pte;
  for(int i = 0; i < pages_num; start_va += PGSIZE, i++){
    if((pte = walk(myproc()->pagetable, start_va, 0)) == 0){  //alloc=0
      panic("sys_pgaccess");
    }
    if(*pte & PTE_A){
      bit_mask |= 1 << i;   //bitmask update
      *pte &= ~PTE_A;       //pte remove PTE_A
    }
  }

  copyout(myproc()->pagetable, abits, 
            (char*)&bit_mask, sizeof(bit_mask));

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
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
