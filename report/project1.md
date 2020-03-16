Design Document for Project 1: User Programs
============================================

## Group Members

* Reza Yarbakhsh <ryarbakhsh@gmail.com>
* Rouzbeh Meshkinnezhad <rouzyd@gmail.com>
* Alireza Tabatabaeian <tabanavid77@gmail.com>
* Ali Behjati <bahjatia@gmail.com>

## Argument Passing
at first we wanted to parse the arguments in `load` function but we did it in start_process because of the need for name of the program. we also needed the name for the thread name in `execute_process`. we also needed to copy the file_name because of the way `strtok_r` changed its first argument. 

## Syscalls
### Memory Safe Access in Syscalls
In page fault if it's from kernel we set `eip` to `eax` and eax to `0xffffffff` and we use get_byte and set_byte provided in the resources to safely get and set byte without page fault. Then exit if there is an error in getting it. 

### File Syscalls
We store open files per thread (Opposed to initial design). Also we store all executing process files and don't allow other processes to write on them.

### Threads Syscalls
It's completely same as the design. We only store a list of processes globally to find child thread in process list.

To have access to child thread after the child finishes we remove child page in the parrent if it's parrent exists.
