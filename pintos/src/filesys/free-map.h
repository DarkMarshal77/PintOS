#ifndef FILESYS_FREE_MAP_H
#define FILESYS_FREE_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/block.h"

void free_map_init (void);
void free_map_read (void);
void free_map_create (void);
void free_map_open (void);
void free_map_close (void);

bool free_map_allocate (size_t, block_sector_t *, bool);
void free_map_release (block_sector_t, bool);

bool free_map_check_space(size_t, bool);
void lock_free_map(void);
void unlock_free_map(void);

#endif /* filesys/free-map.h */
