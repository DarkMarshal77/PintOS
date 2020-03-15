#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);

static uint32_t get_user_byte (const uint8_t *uaddr);
static uint32_t get_user_byte_safe (const uint8_t *uaddr);
static void get_user_safe (uint8_t *udst, const uint8_t *usrc, size_t size);
static void check_user_safe (const uint8_t *usrc, size_t size);
static void check_user_str_safe(const char *uaddr);
static bool put_user_byte (uint8_t *udst, uint8_t byte); 

static struct file* get_file_safe(int fd);
static char* get_file_name_safe(int fd);

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
		check_user_safe(&args[1], 4);
		
		f->eax = args[1];
		printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
		inform_parent();
		thread_exit ();
	}
	else if (args[0] == SYS_HALT)
	{
		shutdown_power_off();
	}
	else if (args[0] == SYS_EXEC)
	{
		check_user_safe(&args[1], 4);
		char *file_name = args[1];

   		check_user_str_safe(file_name);
		tid_t cid = execute_child_process(file_name);
		f->eax = (int)cid;
	}
	else if (args[0] == SYS_WAIT)
	{
		check_user_safe(&args[1], 4);
		tid_t cid = args[1];
	}
	else if (args[0] == SYS_PRACTICE)
	{
		check_user_safe(&args[1], 4);

		f->eax = args[1] + 1;
	}
	else if (args[0] == SYS_WRITE)
	{
		check_user_safe(&args[1], 4 * 3);
		
		int fd = args[1];
		const char *buf = args[2];
		size_t size = args[3];

		check_user_safe(buf, size);

		f->eax = -1; 
		if (fd == 1)
		{
			putbuf(buf, size);
			f->eax = size;
		}
		else
		{
			struct file *file = get_file_safe(fd);
			char* file_name = get_file_name_safe(fd);
			check_open_execs(file_name, file);
			f->eax = file_write (file, buf, size);
		}
	}
	else if (args[0] == SYS_READ)
	{
		check_user_safe(&args[1], 4*3);
		int fd = args[1];
		void *buf = args[2];
		off_t size = args[3];

    check_user_safe(buf, size);

		f->eax = -1;
		if (fd > 1)
		{
			struct file *file = get_file_safe(fd);
			f->eax = file_read (file, buf, size);
		}
	}
	else if (args[0] == SYS_OPEN)
	{
		check_user_safe(&args[1], 4);
		char *file_name = args[1];

    check_user_str_safe(file_name);

		struct file *file = filesys_open(file_name);

		if (file != NULL)
		{
			f->eax = add_file(file, file_name);
			check_open_execs(file_name, file);
		}
		else
			f->eax = -1;
	}
	else if (args[0] == SYS_CLOSE)
	{
		check_user_safe(&args[1], 4);
		int fd = args[1];

		if (fd > 1)
		{
			struct file *file = get_file_safe(fd);
			file_close (file);
			remove_file(file);
		}
	}
	else if (args[0] == SYS_CREATE)
	{
		check_user_safe(&args[1], 4*2);
		char *file_name = args[1];
		off_t initial_size = args[2];

    check_user_str_safe(file_name);

		f->eax = filesys_create (file_name, initial_size);
	}
	else if (args[0] == SYS_REMOVE)
	{
		check_user_safe(&args[1], 4);
		char *file_name = args[1];

    check_user_str_safe(file_name);

		f->eax = filesys_remove (file_name);
	}
	else if (args[0] == SYS_FILESIZE)
	{
		check_user_safe(&args[1], 4);
		int fd = args[1];
		struct file *file = get_file_safe(fd);
		f->eax = file_length (file);
	}
	else if (args[0] == SYS_SEEK)
	{
		check_user_safe(&args[1], 4*2);
		int fd = args[1];
		unsigned position = args[2];
		struct file *file = get_file_safe(fd);
		file_seek (file, position);
	}
	else if (args[0] == SYS_TELL)
	{
		check_user_safe(&args[1], 4);
		int fd = args[1];
		struct file *file = get_file_safe(fd);
		f->eax = file_tell (file);
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
		sys_exit(-1);

	uint32_t result = get_user_byte(uaddr);
	if (result == 0xffffffff)
		sys_exit(-1);

	return result;
}

// Reads from user memory with size and copies to dst. It exits in case it's violating memory access
static void get_user_safe (uint8_t *udst, const uint8_t *usrc, size_t size)
{
	check_user_safe(udst, size);

	for (size_t i = 0; i < size; i++)
		*(udst + i) = get_user_byte_safe(usrc + i) & 0xff;
}

// Checks user memory with size to check violations. It exits in case it's violating memory access
static void check_user_safe (const uint8_t *usrc, size_t size)
{
	for(size_t i = 0; i < size; i++)
		get_user_byte_safe(usrc + i);
}

// Checks string to be safe in terms of memory access (size is unknown)
static void check_user_str_safe(const char *uaddr)
{
  char *ptr = uaddr;
  while( (check_user_safe(ptr, 1), *ptr != '\0') )
    ptr++;
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


// Gets fd file or fails
static struct file* get_file_safe(int fd) { 
  struct file *ret = get_file(fd);
  
  if (ret == NULL)
    sys_exit(-1);
  return ret;
}


static char* get_file_name_safe(int fd){
	char* ret = get_file_name(fd);

	if (ret == NULL)
		sys_exit(-1);
	return ret;
}
