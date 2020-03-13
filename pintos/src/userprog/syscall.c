#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

static int get_user_byte (const uint8_t *uaddr);
static int get_user_byte_safe (const uint8_t *uaddr);
static void get_user_safe (uint8_t *udst, const uint8_t *usrc, size_t size);
static void check_user_safe (const uint8_t *usrc, size_t size);
static bool put_user_byte (uint8_t *udst, uint8_t byte); 

void sys_exit(int status) {
  printf ("%s: exit(%d)\n", &thread_current ()->name, status);
  thread_exit ();
  NOT_REACHED();
}


void
syscall_init (void)
{
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  f->eax = -1; 

	uint32_t* args = ((uint32_t*) f->esp);

  check_user_safe(args, 4);

	/*
	 * The following print statement, if uncommented, will print out the syscall
	 * number whenever a process enters a system call. You might find it useful
	 * when debugging. It will cause tests to fail, however, so you should not
	 * include it in your final submission.
	 */

	/* printf("System call number: %d\n", args[0]); */

	if (args[0] == SYS_EXIT)
	{
    check_user_safe(args + 1, 4);
		f->eax = args[1];
		printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
		thread_exit ();
	}
	else if (args[0] == SYS_PRACTICE)
	{
    check_user_safe(args + 1, 4);
		f->eax = args[1] + 1;
	}
	else if (args[0] == SYS_WRITE)
	{
    check_user_safe(args + 1, 4);
		int fd = args[1];

    check_user_safe(args + 3, 4);
		size_t size = args[3];

    check_user_safe(args + 2, 4);
    check_user_safe(*(args + 2), size);
		const char *buf = args[2];

		f->eax = -1; 
		if (fd == 1)
		{
			putbuf(buf, size);
			f->eax = size;
		}
		else
			printf("Not Implemented yet.\n");
	}
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault occurred. */
static int get_user_byte (const uint8_t *uaddr) {
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
      : "=&a" (result) : "m" (*uaddr));
  return result;
}

static int get_user_byte_safe (const uint8_t *uaddr) { 
  if (!is_user_vaddr(uaddr))
    sys_exit(-1); 

  int result = get_user_byte(uaddr);; 
  if (result == -1)
    sys_exit(-1);

  return result;
}

static void get_user_safe (uint8_t *udst, const uint8_t *usrc, size_t size) { 
  for(size_t i = 0; i < size; i++) 
    * (udst + i) = get_user_byte_safe(usrc + size) & 0xff;
}

static void check_user_safe (const uint8_t *usrc, size_t size) {
  for(size_t i = 0; i < size; i++) 
    get_user_byte_safe(usrc + size);
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user_byte (uint8_t *udst, uint8_t byte) {
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
      : "=&a" (error_code), "=m" (*udst) : "q" (byte)); return error_code != -1;
}
