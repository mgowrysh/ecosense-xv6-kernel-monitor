// sem.c - counting semaphores for xv6-riscv
// Feature 3: new synchronization primitive.
//
// A semaphore wraps xv6's sleep/wakeup with a counter so that multiple
// producers and consumers can coordinate without busy-waiting.

#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define MAX_SEMS 32

struct sem {
  int       used;    // 1 if this slot is allocated
  int       count;   // current semaphore value
  struct spinlock lock;
};

static struct sem semtable[MAX_SEMS];

void
seminit(void)
{
  int i;
  for(i = 0; i < MAX_SEMS; i++)
    semtable[i].used = 0;
}

// Allocate a new semaphore with the given initial count.
// Returns semaphore id (>=0) or -1 if the table is full.
int
sem_create(int init)
{
  int i;
  for(i = 0; i < MAX_SEMS; i++){
    if(!semtable[i].used){
      initlock(&semtable[i].lock, "sem");
      semtable[i].count = init;
      semtable[i].used  = 1;
      return i;
    }
  }
  return -1;
}

// Block until semaphore > 0, then decrement.
int
sem_wait(int id)
{
  if(id < 0 || id >= MAX_SEMS || !semtable[id].used)
    return -1;
  acquire(&semtable[id].lock);
  while(semtable[id].count <= 0)
    sleep(&semtable[id], &semtable[id].lock);
  semtable[id].count--;
  release(&semtable[id].lock);
  return 0;
}

// Increment semaphore and wake a waiter if any.
int
sem_signal(int id)
{
  if(id < 0 || id >= MAX_SEMS || !semtable[id].used)
    return -1;
  acquire(&semtable[id].lock);
  semtable[id].count++;
  wakeup(&semtable[id]);
  release(&semtable[id].lock);
  return 0;
}

// Free a semaphore.
int
sem_destroy(int id)
{
  if(id < 0 || id >= MAX_SEMS || !semtable[id].used)
    return -1;
  acquire(&semtable[id].lock);
  semtable[id].used = 0;
  release(&semtable[id].lock);
  return 0;
}
