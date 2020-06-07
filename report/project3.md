Report for Project 3: FileSystem
============================================

## Group Members

* Reza Yarbakhsh <ryarbakhsh@gmail.com>
* Rouzbeh Meshkinnezhad <rouzyd@gmail.com>
* Alireza Tabatabaeian <tabanavid77@gmail.com>
* Ali Behjati <bahjatia@gmail.com>

## Buffer Cache
Changes to design are as follows:
- Add implementation for cached block read and write without offset and size (complete sector), implementation with offset is ended with partial suffix.
- Add 3 syscalls (`fs_info`, `cache_info`, `cache_flush`) for user tests and also track stats about fs read/write and cache access/hit for them.
- Add some functions to make code more clean. Most importantly `fetch_cache_block` which fetches cache block if it exists in the cache and `create_cache_block` which creates a cache block (without reading it) and adds it to the LRU block list and removes least recently used block if size of cache exceeds the limit.

## Subdirectories
two new functions implemented in ``directory.c``:
```C
static int get_next_part (char part[NAME_MAX + 1], const char **srcp);
```
* Extracts a file name part from SRCP into PART, and updates SRCP so that the next call will return
the next file name part. Returns 1 if successful, 0 at end of string, -1 for a too-long file name part.

```C
bool split_directory_and_filename (const char *path, char *directory, char *filename);
```

* Given a full PATH, extract the DIRECTORY and FILENAME into the provided pointers.

```C
struct dir *dir_open_directory (const char *directory)
```

* Opens and returns a directory given its name

added below functions to ``inode.c`` in order to work with inodes' data from other files:

```C
bool inode_is_dir (const struct inode *inode);
bool inode_is_removed (const struct inode *inode);
block_sector_t inode_get_parent (const struct inode* inode);
void inode_set_parent (struct inode* inode, block_sector_t parent);
```

unused size and type changed in ``struct inode_disk`` because of the addition of ``bool is_dir`` and ``block_sector_t parent``

we had to give the initial size of 1000 to directories in order to test them becasue we implemented directories before extensible files.
