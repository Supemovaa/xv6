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

#define NBUCKETS  13
#define hash(blockno) (blockno % NBUCKETS)

struct bucket{
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  // bufs in each bucket are organized in linked lists
  struct bucket buckets[NBUCKETS]; // the cache has 13 buckets
} bcache;

void
binit(void)
{
  struct buf *b;
  struct buf *prev_b;

  initlock(&bcache.lock, "bcache");
  
  for(int i = 0; i < NBUCKETS; i++){
    initlock(&bcache.buckets[i].lock, "bcache.bucket");
    // bcache.buckets[i].head is a sentinal node
    bcache.buckets[i].head.next = (void *)0;
    // pass all bufs to buckets[0] firstly
    if (i == 0){
      prev_b = &bcache.buckets[i].head;
      for(b = bcache.buf; b < bcache.buf + NBUF; b++){
        b->next = (void*)0;
        prev_b->next = b;
        initsleeplock(&b->lock, "buffer");
        prev_b = b; // next linking
      }    
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = blockno % NBUCKETS;

  // check if the block already cached?
  acquire(&bcache.buckets[index].lock);  
  b = bcache.buckets[index].head.next; // the first buf in buckets[index]
  while(b){ 
    // found
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buckets[index].lock);
      acquiresleep(&b->lock);
      return b;   
    }
    b = b->next;
  }
  release(&bcache.buckets[index].lock);
  
  // if not, steal a buf (any buf whose refcount == 0) from another bucket
  // remember to lock each bucket
  struct buf *stolen = (void *)0;
  struct buf *prev = (void *)0;
  for(int i = 0; i < NBUCKETS; i++){
    prev = &bcache.buckets[i].head;
    acquire(&bcache.buckets[i].lock);
    while(prev->next){
      if(prev->next->refcnt == 0){
        stolen = prev->next;
        prev->next = prev->next->next;
        release(&bcache.buckets[i].lock);
        goto End_loop;
      }
      prev = prev->next;
    }
    release(&bcache.buckets[i].lock);
  }
  End_loop:

  // cache *stolen* to buckets[index]
  acquire(&bcache.lock);
  acquire(&bcache.buckets[index].lock);
  if(stolen){
    stolen->next = bcache.buckets[index].head.next;
    bcache.buckets[index].head.next = stolen;
  }

  // b = bcache.buckets[index].head.next;
  // while(b){ 
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.buckets[index].lock);
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;   
  //   }
  //   b = b->next;
  // }

  // no free blocks
  if (stolen == 0)
    panic("bget: no buffers");
  
  stolen->dev = dev;
  stolen->blockno = blockno;
  stolen->valid = 0;
  stolen->refcnt = 1;
  release(&bcache.buckets[index].lock);
  release(&bcache.lock);
  acquiresleep(&stolen->lock);
  return stolen;
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

  int buk_id = hash(b->blockno);
  acquire(&bcache.buckets[buk_id].lock);
  b->refcnt--;
  release(&bcache.buckets[buk_id].lock);
}

void
bpin(struct buf *b) {
  int index = hash(b->blockno);
  acquire(&bcache.buckets[index].lock);
  b->refcnt++;
  release(&bcache.buckets[index].lock);
}

void
bunpin(struct buf *b) {
  int index = hash(b->blockno);
  acquire(&bcache.buckets[index].lock);
  b->refcnt--;
  release(&bcache.buckets[index].lock);
}