#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
	uint32_t* args = ((uint32_t*) f->esp);

	/*
	 * The following print statement, if uncommented, will print out the syscall
	 * number whenever a process enters a system call. You might find it useful
	 * when debugging. It will cause tests to fail, however, so you should not
	 * include it in your final submission.
	 */

	/* printf("System call number: %d\n", args[0]); */

	if (args[0] == SYS_EXIT)
	{
		f->eax = args[1];
		printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
		thread_exit ();
	}
	else if (args[0] == SYS_PRACTICE)
	{
		f->eax = args[1] + 1;
	}
	else if (args[0] == SYS_WRITE)
	{
		int fd = args[1];
		const char *buf = args[2];
		size_t size = args[3];

		f->eax = -1; 
		if (fd == 1)
		{
			putbuf(buf, size);
			f->eax = size;
		}
		else
		{
			struct file *file = get_file(fd);
			f->eax = file_write (file, buf, size);
		}
	}
	else if (args[0] == SYS_READ)
	{
		int fd = args[1];
		void *buf = args[2];
		off_t size = args[3];

		f->eax = -1;
		if (fd > 1)
		{
			struct file *file = get_file(fd);
			f->eax = file_read (file, buf, size);
		}
	}
	else if (args[0] == SYS_OPEN)
	{
		char *file_name = args[1];
		struct file *file = filesys_open(file_name);

		if (file != NULL)
			f->eax = add_file(file);
		else
			f->eax = -1;
	}
	else if (args[0] == SYS_CLOSE)
	{
		int fd = args[1];

		if (fd > 1)
		{
			struct file *file = get_file(fd);
			file_close (file);
			remove_file(file);
		}
	}
	else if (args[0] == SYS_CREATE)
	{
		char *file_name = args[1];
		off_t initial_size = args[2];

		f->eax = filesys_create (file_name, initial_size);
	}
	else if (args[0] == SYS_REMOVE)
	{
		char *file_name = args[1];
		f->eax = filesys_remove (file_name);
	}
}
