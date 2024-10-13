// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKET_SIZE 13

extern uint ticks;

struct {
  struct spinlock lock;
  struct buf head;
} bcaches[BUCKET_SIZE];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < BUCKET_SIZE; i++){
    initlock(&bcaches[i].lock, "bcaches");
    bcaches[i].head.prev = &bcaches[i].head;
    bcaches[i].head.next = &bcaches[i].head;
  }

  for(b = bcache.buf; b < bcache.buf + NBUF; b++){
    b->tick = -1;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *lru;  //potential LRU buf

  int bkt_id = blockno % BUCKET_SIZE;
  acquire(&bcaches[bkt_id].lock);

  // Is the block already cached?
  for(b = bcaches[bkt_id].head.next; b != &bcaches[bkt_id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->tick = ticks;
      b->refcnt++;
      release(&bcaches[bkt_id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  //Search in the bucket for LRU.
  lru = 0;
  for(b = bcaches[bkt_id].head.prev; b != &bcaches[bkt_id].head; b = b->prev){
    if(b->refcnt == 0) {
      if(lru == 0){ //LRU = NULL
        lru = b;
      }
      else {
        if(lru->tick < b->tick) lru = b;  //update LRU buf
      }
    }
  }
  if(lru){
    lru->dev = dev;
    lru->blockno = blockno;
    lru->valid = 0;
    lru->refcnt = 1;
    lru->tick = ticks;
    release(&bcaches[bkt_id].lock);
    acquiresleep(&lru->lock);
    return lru;
    
  }

  //Not cached
  //Search in the cache buffer for LRU.
  int flag = 1;
  while(flag) {
    flag = 0;
    lru = 0;
    for(b = bcache.buf; b < bcache.buf + NBUF; b++){
      if(b->refcnt == 0){
        flag = 1;
        if(lru == 0){
          lru = b;
        }
        else{
          if(lru->tick > b->tick) lru = b;
        }
      }
    }
    if(lru){
      if(lru->tick == -1){
        acquire(&bcache.lock);
        if(lru->refcnt == 0) {
          lru->dev = dev;
          lru->blockno = blockno;
          lru->valid = 0;
          lru->refcnt = 1;
          lru->tick = ticks;

          lru->next = bcaches[bkt_id].head.next;
          lru->prev = &bcaches[bkt_id].head;
          bcaches[bkt_id].head.next->prev = lru;
          bcaches[bkt_id].head.next = lru;

          release(&bcache.lock);
          release(&bcaches[bkt_id].lock);
          acquiresleep(&lru->lock);
          return lru;
        }
        else{ //search for another buffer.
          release(&bcache.lock);
          break;
        }
      }
      else{
        int oth_id = (b->blockno) % BUCKET_SIZE;
        acquire(&bcaches[oth_id].lock);
        
        if(lru->refcnt == 0){
          lru->dev = dev;
          lru->blockno = blockno;
          lru->valid = 0;
          lru->refcnt = 1;
          lru->tick = ticks;

          lru->next->prev = lru->prev;
          lru->prev->next = lru->next;
          lru->next = bcaches[bkt_id].head.next;
          lru->prev = &bcaches[bkt_id].head;
          bcaches[bkt_id].head.next->prev = lru;
          bcaches[bkt_id].head.next = lru;

          release(&bcaches[oth_id].lock);
          release(&bcaches[bkt_id].lock);
          acquiresleep(&lru->lock);
          return lru;
        }
        else{
          acquire(&bcaches[oth_id].lock);
          break;
        }
      }
    }
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bkt_id = (b->blockno) % BUCKET_SIZE;
  acquire(&bcaches[bkt_id].lock);
  b->refcnt--;
  release(&bcaches[bkt_id].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}

