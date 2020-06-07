#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

#define DIRECT_INDEX_COUNT  16

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
  block_sector_t direct[DIRECT_INDEX_COUNT];  /* First level sectors. */
  block_sector_t indirect;                    /* Second level data sectors. */
  block_sector_t double_indirect;             /* Third level data sectors. */
  off_t length;                               /* File size in bytes. */
  bool is_dir;                                /* indicates whether an inode is a directory */
  block_sector_t parent;                      /* inode's parent */
  unsigned magic;                             /* Magic number. */
  uint8_t unused[423];                        /* Not used. */
};

/* Returns sector indexes according to SECTOR number.
   For direct and indirect sectores, index will place in
   DIRECT_INDEX and INDIRECT_INDEX will be -1. For double indirect
   sectores, first level index will place in INDIRECT_INDEX and
   second level index will place in DIRECT_INDEX. */
void
sector_to_index(size_t sector, int *direct_index, int *indirect_index)
{
  if (sector < DIRECT_INDEX_COUNT + ADDRESS_COUNT_PER_BLOCK)
  {
    *direct_index = sector;
    *indirect_index = -1;
  }
  else
  {
    sector -= (DIRECT_INDEX_COUNT + ADDRESS_COUNT_PER_BLOCK);
    *direct_index = sector % ADDRESS_COUNT_PER_BLOCK;
    *indirect_index = sector / ADDRESS_COUNT_PER_BLOCK;
  }
}

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct lock inode_lock;             /* Inode lock. */
    struct inode_disk data;             /* Inode content. */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);

  cached_block_read (fs_device, inode->sector, &inode->data);

  if (pos < inode->data.length)
  {
    uint32_t sector_number = pos / BLOCK_SECTOR_SIZE;

    int direct_index, indirect_index;
    sector_to_index (sector_number, &direct_index, &indirect_index);

    if (indirect_index == -1)
    {
      if (direct_index < DIRECT_INDEX_COUNT)
      {
        return inode->data.direct[direct_index];
      }
      else
      {
        direct_index -= DIRECT_INDEX_COUNT;
        block_sector_t indirect_sector[ADDRESS_COUNT_PER_BLOCK];
        cached_block_read (fs_device, inode->data.indirect, indirect_sector);

        return indirect_sector[direct_index];
      }
    }
    else
    {
      block_sector_t double_indirect_sector[ADDRESS_COUNT_PER_BLOCK];
      cached_block_read (fs_device, inode->data.double_indirect, double_indirect_sector);

      block_sector_t indirect_sector[ADDRESS_COUNT_PER_BLOCK];
      cached_block_read (fs_device, double_indirect_sector[indirect_index], indirect_sector);

      return indirect_sector[direct_index];
    }
  }
  else
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
  list_init (&LRU_list);
  lock_init (&LRU_lock);
  cache_hit_cnt = cache_access_cnt = 0;
  read_cnt = write_cnt = 0;
}

/* Extendes DISK_INODE length to LENGTH  */
bool
inode_extend(struct inode_disk *disk_inode, off_t length)
{
  if (length == disk_inode->length)
    return true;

  ASSERT (disk_inode->length < length);
  static char zeros[BLOCK_SECTOR_SIZE];

  size_t cur_sector_cnt = bytes_to_sectors(disk_inode->length);
  size_t new_sector_cnt = bytes_to_sectors(length);

  int cur_last_index[2], new_last_index[2];
  sector_to_index (cur_sector_cnt, &cur_last_index[1], &cur_last_index[0]);
  sector_to_index (new_sector_cnt, &new_last_index[1], &new_last_index[0]);

  lock_free_map();

  /* Check if we have enough space */
  size_t needed_sectors = 0;
  if (cur_last_index[0] == -1)
  {
    if (new_last_index[0] == -1)
    {
      needed_sectors = (new_last_index[1] >= DIRECT_INDEX_COUNT && cur_last_index[1] < DIRECT_INDEX_COUNT) ? 1 : 0;
      needed_sectors += new_last_index[1] - cur_last_index[1];
    }
    else
    {
      needed_sectors = (cur_last_index[1] < DIRECT_INDEX_COUNT) ? 1 : 0;
      needed_sectors += DIRECT_INDEX_COUNT + ADDRESS_COUNT_PER_BLOCK - cur_last_index[1] - 1;
      needed_sectors += 3 + new_last_index[0]*(1+ADDRESS_COUNT_PER_BLOCK) + new_last_index[1];
    }
  }
  else
  {
    needed_sectors = (new_last_index[0] - cur_last_index[0])*(1+ADDRESS_COUNT_PER_BLOCK) +
                      new_last_index[1] - cur_last_index[1];
  }
  if (!free_map_check_space (needed_sectors, true))
  {
    unlock_free_map();
    return false;
  }

  /* Direct and indirect allocations. */
  if (cur_last_index[0] == -1)
  {
    if (cur_last_index[1] < DIRECT_INDEX_COUNT)
    {
      int tmp1 = (disk_inode->length > 0) ? cur_last_index[1] : -1;
      int tmp2 = 0;
      if (new_last_index[0] >= 0 || new_last_index[1] >= DIRECT_INDEX_COUNT)
        tmp2 = DIRECT_INDEX_COUNT - 1;
      else
        tmp2 = new_last_index[1];

      /* Direct sector allocation of direct addressing. */
      for (int j = tmp1+1; j <= tmp2; j++)
      {
        free_map_allocate (1, &disk_inode->direct[j], true);
        cached_block_write (fs_device, disk_inode->direct[j], zeros);
      }

      /* Allocate indirect sector. */
      if (new_last_index[0] >= 0 || new_last_index[1] >= DIRECT_INDEX_COUNT)
      {
        free_map_allocate (1, &disk_inode->indirect, true);
        cached_block_write (fs_device, disk_inode->indirect, zeros);
      }
    }

    if (new_last_index[0] >= 0 || new_last_index[1] >= DIRECT_INDEX_COUNT)
    {
      int tmp1 = (cur_last_index[1] >= DIRECT_INDEX_COUNT) ? cur_last_index[1] - DIRECT_INDEX_COUNT : -1;
      int tmp2 = (new_last_index[0] == -1) ? new_last_index[1] - DIRECT_INDEX_COUNT : ADDRESS_COUNT_PER_BLOCK - 1;

      /* Loading indirect sector. */
      block_sector_t indirect_sector[ADDRESS_COUNT_PER_BLOCK];
      cached_block_read (fs_device, disk_inode->indirect, indirect_sector);

      /* Direct sector allocation of indirect addressing. */
      for (int j = tmp1+1; j <= tmp2; j++)
      {
        free_map_allocate (1, &indirect_sector[j], true);
        cached_block_write (fs_device, indirect_sector[j], zeros);
      }

      cached_block_write (fs_device, disk_inode->indirect, indirect_sector);
    }

    /* Allocate double indirect sector. */
    if (new_last_index[0] >= 0)
    {
      free_map_allocate (1, &disk_inode->double_indirect, true);
      cached_block_write (fs_device, disk_inode->double_indirect, zeros);
    }
  }

  /* Double indirect allocations */
  if (new_last_index[0] >= 0)
  {
    /* Loading double indirect sector. */
    block_sector_t double_indirect_sector[ADDRESS_COUNT_PER_BLOCK];
    cached_block_read (fs_device, disk_inode->double_indirect, double_indirect_sector);

    if (cur_last_index[0] == new_last_index[0])
    {
      /* Loading indirect sector of double indirect addressing. */
      block_sector_t indirect_sector[ADDRESS_COUNT_PER_BLOCK];
      cached_block_read (fs_device, double_indirect_sector[cur_last_index[0]], indirect_sector);

      /* Direct sector allocation. */
      free_map_allocate (new_last_index[1]-cur_last_index[1], &indirect_sector[cur_last_index[1]+1], true);
      for (int j = cur_last_index[1]+1; j <= new_last_index[1]; j++)
        cached_block_write (fs_device, indirect_sector[j], zeros);

      cached_block_write (fs_device, double_indirect_sector[cur_last_index[0]], indirect_sector);
    }
    else if (cur_last_index[0] >= 0)
    {
      /* Loading indirect sector of double indirect addressing. */
      block_sector_t indirect_sector[ADDRESS_COUNT_PER_BLOCK];
      cached_block_read (fs_device, double_indirect_sector[cur_last_index[0]], indirect_sector);

      /* Direct sector allocation. */
      for (int j = cur_last_index[1]+1; j < ADDRESS_COUNT_PER_BLOCK; j++)
      {
        free_map_allocate (1, &indirect_sector[j], true);
        cached_block_write (fs_device, indirect_sector[j], zeros);
      }

      cached_block_write (fs_device, double_indirect_sector[cur_last_index[0]], indirect_sector);
    }
    for (int i = cur_last_index[0]+1; i < new_last_index[0]; i++)
    {
      /* Indirect sector allocation of double indirect addressing. */
      free_map_allocate (1, &double_indirect_sector[i], true);

      /* Direct sector allocation. */
      block_sector_t indirect_sector[ADDRESS_COUNT_PER_BLOCK];
      free_map_allocate (ADDRESS_COUNT_PER_BLOCK, indirect_sector, true);
      for (int j = 0; j < ADDRESS_COUNT_PER_BLOCK; j++)
        cached_block_write (fs_device, indirect_sector[j], zeros);

      cached_block_write (fs_device, double_indirect_sector[i], indirect_sector);
    }
    if (cur_last_index[0] != new_last_index[0])
    {
      /* Indirect sector allocation of double indirect addressing. */
      free_map_allocate (1, &double_indirect_sector[new_last_index[0]], true);

      /* Direct sector allocation. */
      block_sector_t indirect_sector[ADDRESS_COUNT_PER_BLOCK];
      free_map_allocate (new_last_index[1]+1, indirect_sector, true);
      for (int j = 0; j <= new_last_index[1]; j++)
        cached_block_write (fs_device, indirect_sector[j], zeros);

      cached_block_write (fs_device, double_indirect_sector[new_last_index[0]], indirect_sector);
    }

    cached_block_write (fs_device, disk_inode->double_indirect, double_indirect_sector);
  }

  unlock_free_map();
  return true;
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
  {
    if (inode_extend (disk_inode, length))
    {
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->is_dir = is_dir;

      cached_block_write (fs_device, sector, disk_inode);
      success = true;
    }

    free (disk_inode);
  }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  cached_block_read (fs_device, inode->sector, &inode->data);
  lock_init (&inode->inode_lock);
  // printf("%s: sector:%08x, len:%u, magic:%u\n", __FUNCTION__, inode->sector, inode->data.length, inode->data.magic);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
  {
    /* Remove from inode list and release lock. */
    list_remove (&inode->elem);

    /* Deallocate blocks if removed. */
    if (inode->removed)
    {
      lock_free_map();

      off_t pos = 0;
      while (pos <= inode->data.length)
      {
        free_map_release (byte_to_sector(inode, pos), true);
        pos += BLOCK_SECTOR_SIZE;
      }

      if (inode->data.indirect)
      {
        free_map_release (inode->data.indirect, true);
        inode->data.indirect = 0;
      }

      if (inode->data.double_indirect)
      {
        /* Loading double indirect sector. */
        block_sector_t double_indirect_sector[ADDRESS_COUNT_PER_BLOCK];
        cached_block_read (fs_device, inode->data.double_indirect, double_indirect_sector);

        for (uint32_t i = 0; i < ADDRESS_COUNT_PER_BLOCK && double_indirect_sector[i]; i++)
          free_map_release (double_indirect_sector[i], true);
        free_map_release (inode->data.double_indirect, true);
        inode->data.double_indirect = 0;
      }

      free_map_release (inode->sector, true);

      unlock_free_map();
    }

    free (inode);
  }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  // printf("\n%s Called. size:%u, off:%u\n", __FUNCTION__, size, offset);
  inode_deny_write (inode);

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      // printf("\n%s: sector_idx:%08x, len:%u\n", __FUNCTION__, sector_idx, inode->data.length);

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          cached_block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else
        {
          cached_block_read_partial (fs_device, sector_idx, buffer + bytes_read,
                                     sector_ofs, chunk_size);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }

  inode_allow_write (inode);
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  // printf("\n%s Called. sector:%08x, size:%u, off:%u\n", __FUNCTION__, inode->sector, size, offset);
  lock_acquire (&inode->inode_lock);

  if (inode->deny_write_cnt)
  {
    lock_release (&inode->inode_lock);
    return 0;
  }

  /* Check if our file has enough space. */
  if (byte_to_sector (inode, offset + size - 1) == (size_t)-1)
  {
    /* Allocate more sectors. */
    if (!inode_extend (&inode->data, offset + size))
    {
      lock_release (&inode->inode_lock);
      return 0;
    }

    /* Update inode_disk. */
    inode->data.length = offset + size;
    cached_block_write (fs_device, inode->sector, &inode->data);
  }

  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          cached_block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else
        {
          cached_block_write_partial (fs_device, sector_idx, buffer + bytes_written,
                                      sector_ofs, chunk_size);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  lock_release (&inode->inode_lock);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  lock_acquire (&inode->inode_lock);

  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);

  lock_release (&inode->inode_lock);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  lock_acquire (&inode->inode_lock);

  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;

  lock_release (&inode->inode_lock);
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}


static
void
free_LRU_block (void)
{
  struct list_elem *e = list_prev (list_end (&LRU_list));
  struct cache_block *cb = list_entry (e, struct cache_block, elem);

  list_remove (e);

  lock_acquire (&cb->cb_lock);

  if (cb->is_dirty)
    {
      block_write (cb->block, cb->sector, (void *) cb->block_data);
      write_cnt ++;
    }

  lock_release (&cb->cb_lock);

  free (cb);
}

static
struct cache_block*
create_cache_block (struct block *block, block_sector_t sector)
{
  struct cache_block *cb;
  cb = malloc (sizeof *cb);

  cb->is_dirty = false;
  cb->block = block;
  cb->sector = sector;
  lock_init (&cb->cb_lock);

  if (list_size (&LRU_list) == LRU_CACHE_SIZE)
    free_LRU_block ();

  list_insert (list_begin (&LRU_list), &cb->elem);

  return cb;
}

struct cache_block*
fetch_cache_block (struct block *block, block_sector_t sector)
{
  cache_access_cnt ++;

  struct list_elem *e;

  for (e = list_begin (&LRU_list); e != list_end (&LRU_list); e = list_next (e))
    {
      struct cache_block *cb = list_entry (e, struct cache_block, elem);
      if (block == cb->block && sector == cb->sector)
        {
          list_remove (e);
          list_insert (list_begin (&LRU_list), e);

          cache_hit_cnt ++;
          return cb;
        }
    }


  return NULL;
}


void
cached_block_read (struct block *block, block_sector_t sector,
                   void *buffer)
{
  lock_acquire (&LRU_lock);
  struct cache_block *cb = fetch_cache_block (block, sector);

  bool new_block = false;
  if (cb == NULL)
    {
      cb = create_cache_block (block, sector);
      new_block = true;
    }

  lock_acquire (&cb->cb_lock);
  lock_release (&LRU_lock);

  if (new_block)
    {
      block_read (block, sector, (void *) cb->block_data);
      read_cnt ++;
    }

  memcpy (buffer, cb->block_data, BLOCK_SECTOR_SIZE);
  lock_release (&cb->cb_lock);

  return;
}


void
cached_block_read_partial (struct block *block, block_sector_t sector,
                                void *buffer, size_t offset, size_t size)
{
  lock_acquire (&LRU_lock);
  struct cache_block *cb = fetch_cache_block (block, sector);

  bool new_block = false;
  if (cb == NULL)
    {
      cb = create_cache_block (block, sector);
      new_block = true;
    }

  lock_acquire (&cb->cb_lock);
  lock_release (&LRU_lock);

  if (new_block)
    {
      block_read (block, sector, (void *) cb->block_data);
      read_cnt ++;
    }

  memcpy (buffer, (void *) cb->block_data + offset, size);
  lock_release (&cb->cb_lock);
}


void
cached_block_write (struct block *block, block_sector_t sector,
                    const void *buffer)
{
  lock_acquire (&LRU_lock);
  struct cache_block *cb = fetch_cache_block (block, sector);

  if (cb == NULL)
    cb = create_cache_block (block, sector);

  lock_acquire (&cb->cb_lock);
  lock_release (&LRU_lock);

  cb->is_dirty = true;
  memcpy (cb->block_data, buffer, BLOCK_SECTOR_SIZE);
  lock_release (&cb->cb_lock);

  return;
}

void cached_block_write_partial (struct block *block, block_sector_t sector, const void *buffer,
                                 size_t offset, size_t size)
{
  struct cache_block *cb;

  lock_acquire (&LRU_lock);

  cb = fetch_cache_block (block, sector);

  bool new_block = false;
  if (cb == NULL)
    {
      new_block = true;
      cb = create_cache_block(block, sector);
    }

  lock_acquire (&cb->cb_lock);
  lock_release (&LRU_lock);

  if (new_block)
    {
      block_read (block, sector, (void *) cb->block_data);
      read_cnt ++;
    }

  memcpy (cb->block_data + offset, buffer, size);

  cb->is_dirty = true;
  lock_release (&cb->cb_lock);
}

void
free_all_cache (void)
{
  lock_acquire (&LRU_lock);
  while (list_size (&LRU_list))
    free_LRU_block();
  lock_release (&LRU_lock);
}

/* Returns is_dir of INODE's data. */
bool
inode_is_dir (const struct inode *inode)
{
  ASSERT (inode != NULL);
  return inode->data.is_dir;
}

bool
inode_is_removed (const struct inode *inode)
{
  ASSERT (inode != NULL);
  return inode->removed;
}

block_sector_t
inode_get_parent (const struct inode* inode)
{
  return inode->data.parent;
}

void
inode_set_parent (struct inode* inode, block_sector_t parent)
{
  inode->data.parent = parent;
}
