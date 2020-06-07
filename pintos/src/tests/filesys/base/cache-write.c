/* -*- c -*- */
#include "tests/filesys/seq-test.h"
#include "tests/lib.h"
#include "tests/main.h"
#include <syscall.h>

static char buf[64*1024];

static size_t
return_block_size (void) 
{
  return 1;
}

void
test_main (void) 
{
  seq_test ("x",
            buf, sizeof buf, sizeof buf,
            return_block_size, NULL);
 
  cache_flush();

  int read_cnt, write_cnt;

  fs_info(&read_cnt, &write_cnt);

  if (write_cnt > 1000)
     fail("write_cnt is %d, expected less than 1000 write_cnt\n", write_cnt);
}

