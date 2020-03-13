#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

static uint32_t get_user_byte (const uint8_t *uaddr);
static uint32_t get_user_byte_safe (const uint8_t *uaddr);
static bool get_user_safe (uint8_t *udst, const uint8_t *usrc, size_t size);
static bool check_user_safe (const uint8_t *usrc, size_t size);
static bool put_user_byte (uint8_t *udst, uint8_t byte); 

void sys_exit(int status)
{
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

	if (check_user_safe(args, 4) == 0xffffffff)
		sys_exit(-1);

	/*
	 * The following print statement, if uncommented, will print out the syscall
	 * number whenever a process enters a system call. You might find it useful
	 * when debugging. It will cause tests to fail, however, so you should not
	 * include it in your final submission.
	 */

	/* printf("System call number: %d\n", args[0]); */

	if (args[0] == SYS_EXIT)
	{
		if (!check_user_safe(&args[1], 4))
			sys_exit(-1);
		
		f->eax = args[1];
		printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
		thread_exit ();
	}
	else if (args[0] == SYS_PRACTICE)
	{
		if (!check_user_safe(&args[1], 4))
			sys_exit(-1);

		f->eax = args[1] + 1;
	}
	else if (args[0] == SYS_WRITE)
	{
		if (!check_user_safe(&args[1], 4 * 3))
			sys_exit(-1);
		
		int fd = args[1];
		const char *buf = args[2];
		size_t size = args[3];

		if (!check_user_safe(buf, size))
			sys_exit(-1);

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

/* 
 * Reads a byte at user virtual address UADDR.
 * UADDR must be below PHYS_BASE.
 * Returns the byte value if successful, -1 if a segfault occurred.
 */
static uint32_t get_user_byte (const uint8_t *uaddr)
{
	uint32_t result;
	asm volatile ("movl $1f, %0; movzbl %1, %0; 1:" : "=&a" (result) : "m" (*uaddr));
	return result;
}

// Reads a user byte and exits in case it's violating memory access
static uint32_t get_user_byte_safe (const uint8_t *uaddr)
{
	if (!is_user_vaddr(uaddr))
		return 0xffffffff;

	return get_user_byte(uaddr);
}

// Reads from user memory with size and copies to dst. It exits in case it's violating memory access
static bool get_user_safe (uint8_t *udst, const uint8_t *usrc, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		uint32_t status = get_user_byte_safe(usrc + size);
		if (status == 0xffffffff)
			return false; 
		* (udst + i) = status & 0xff;
	}
	return true;
}

// Checks user memory with size to check violations. It exits in case it's violating memory access
static bool check_user_safe (const uint8_t *usrc, size_t size)
{
	for(size_t i = 0; i < size; i++) 
		if (get_user_byte_safe(usrc + size) == 0xffffffff)
			return false;

	return true;
}

/* 
 * Writes BYTE to user address UDST.
 * UDST must be below PHYS_BASE.
 * Returns true if successful, false if a segfault occurred.
 */
static bool
put_user_byte (uint8_t *udst, uint8_t byte)
{
	int error_code;
	asm volatile ("movl $1f, %0; movb %b2, %1; 1:" : "=&a" (error_code), "=m" (*udst) : "q" (byte));
	return error_code != -1;
}
