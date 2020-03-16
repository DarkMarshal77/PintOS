Design Document for Project 1: User Programs
============================================

## Group Members

* Reza Yarbakhsh <ryarbakhsh@gmail.com>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>

Argument Passing
at first we wanted to parse the arguments in `load` function but we did it in start_process because of the need for name of the program. we also needed the name for the thread name in `execute_process`. we also needed to copy the file_name because of the way `strtok_r` changed its first argument. 
