#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include <list.h>
#include "threads/synch.h"
#include "filesys/off_t.h"
#include "devices/block.h"

#define LRU_CACHE_SIZE 64

struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t, bool);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

struct list LRU_list;
struct lock LRU_lock;

int cache_hit_cnt;
int cache_access_cnt;

int read_cnt;
int write_cnt;

struct cache_block {
  bool is_dirty;

  struct block *block;
  block_sector_t sector;

  struct list_elem elem;
  struct lock cb_lock;

  char block_data[BLOCK_SECTOR_SIZE];
};

struct cache_block* fetch_cache_block (struct block *block, block_sector_t sector);

void cached_block_read (struct block *block, block_sector_t sector, void *buffer);
void cached_block_write (struct block *block, block_sector_t sector, const void *buffer);

void cached_block_read_partial (struct block *block, block_sector_t sector, void *buffer,
                                size_t offset, size_t size);
void cached_block_write_partial (struct block *block, block_sector_t sector, const void *buffer,
                                 size_t offset, size_t size);

void free_all_cache (void);

bool inode_is_dir (const struct inode *inode);
bool inode_is_removed (const struct inode *inode);
block_sector_t inode_get_parent (const struct inode* inode);
void inode_set_parent (struct inode* inode, block_sector_t parent);

#endif /* filesys/inode.h */
