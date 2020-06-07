#include "filesys/free-map.h"
#include <bitmap.h>
#include <debug.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/synch.h"

static struct file *free_map_file;  /* Free map file. */
static struct bitmap *free_map;     /* Free map, one bit per sector. */
static struct lock free_map_lock;   /* Free map lock to avoid race in alloc and free */

/* Initializes the free map. */
void
free_map_init (void)
{
  free_map = bitmap_create (block_size (fs_device));
  if (free_map == NULL)
    PANIC ("bitmap creation failed--file system device is too large");
  bitmap_mark (free_map, FREE_MAP_SECTOR);
  bitmap_mark (free_map, ROOT_DIR_SECTOR);
  lock_init (&free_map_lock);
}

/* Allocates CNT consecutive sectors from the free map and stores
   the first into *SECTORP.
   Returns true if successful, false if not enough consecutive
   sectors were available or if the free_map file could not be
   written. */
bool
free_map_allocate (size_t cnt, block_sector_t *sectors, bool check_lock)
{
  ASSERT (!check_lock || lock_held_by_current_thread (&free_map_lock));

  if (!free_map_check_space (cnt))
    return false;

  size_t i = 0;
  size_t pos = 0;
  for (; i < cnt; i++)
  {
    pos = bitmap_scan_and_flip (free_map, pos, 1, false);
    sectors[i] = pos++;

    /* Check if the last allocation was successful. */
    if (sectors[i] == BITMAP_ERROR)
    {
      for (; i >= 0; i--)
        bitmap_reset (free_map, sectors[i]);
      return false;
    }
  }

  /* Try writing to free_map_file. */
  if (free_map_file == NULL || !bitmap_write (free_map, free_map_file))
  {
    for (; i >= 0; i--)
      bitmap_reset (free_map, sectors[i]);
    return false;
  }

  return true;
}

/* Makes SECTOR available for use. */
void
free_map_release (block_sector_t sector, bool check_lock)
{
  ASSERT (!check_lock || lock_held_by_current_thread (&free_map_lock));

  ASSERT (bitmap_test (free_map, sector));
  bitmap_reset (free_map, sector);
  bitmap_write (free_map, free_map_file);
}

/* Opens the free map file and reads it from disk. */
void
free_map_open (void)
{
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_read (free_map, free_map_file))
    PANIC ("can't read free map");
}

/* Writes the free map to disk and closes the free map file. */
void
free_map_close (void)
{
  file_close (free_map_file);
}

/* Creates a new free map file on disk and writes the free map to
   it. */
void
free_map_create (void)
{
  /* Create inode. */
  if (!inode_create (FREE_MAP_SECTOR, bitmap_file_size (free_map)))
    PANIC ("free map creation failed");

  /* Write bitmap to file. */
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_write (free_map, free_map_file))
    PANIC ("can't write free map");
}

/* Check if we have enough sectors. */
bool
free_map_check_space(size_t sector_count)
{
  ASSERT (lock_held_by_current_thread (&free_map_lock));

  if (bitmap_count (free_map, 0, block_size (fs_device), false) < sector_count)
    return false;
  return true;
}

void lock_free_map()
{
  acquire_lock (&free_map_lock);
}

void unlock_free_map()
{
  release_lock (&free_map_lock);
}
