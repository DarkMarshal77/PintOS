/* -*- c -*- */
#include "tests/lib.h"
#include "tests/main.h"
#include <random.h>
#include <syscall.h>

static char buf[64*1024];

static 
void
seq_test (const char *file_name, void *buf, size_t size, size_t initial_size,
          size_t (*block_size_func) (void),
          void (*check_func) (int fd, long ofs))
{
  size_t ofs;
  int fd;

  random_bytes (buf, size);
  CHECK (create (file_name, initial_size), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);

  ofs = 0;
  msg ("writing \"%s\"", file_name);
  while (ofs < size)
    {
      size_t block_size = block_size_func ();
      if (block_size > size - ofs)
        block_size = size - ofs;

      if (write (fd, buf + ofs, block_size) != (int) block_size)
        fail ("write %zu bytes at offset %zu in \"%s\" failed",
              block_size, ofs, file_name);

      ofs += block_size;
      if (check_func != NULL)
        check_func (fd, ofs);
    }
  msg ("close \"%s\"", file_name);
  close (fd);
}

static size_t
return_block_size (void) 
{
  return 512;
}

void
test_main (void) 
{
  int init_read_cnt, init_write_cnt; 
  fs_info (&init_read_cnt, &init_write_cnt);
  seq_test ("x",
            buf, sizeof buf, sizeof buf,
            return_block_size, NULL);
 
  cache_flush();

  int read_cnt, write_cnt;

  fs_info(&read_cnt, &write_cnt);

  if (read_cnt - init_read_cnt > 100)
     fail("read_cnt is %d, expected nearly 0 read_cnt\n", read_cnt - init_read_cnt);
}

