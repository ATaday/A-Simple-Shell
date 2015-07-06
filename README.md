# A-Simple-Shell
You will write a C program to serve as a shell interface that accepts
user commands and then executes each command in a separate process. A shell interface gives
the user a prompt, after which the next command is entered. The example below illustrates the
prompt mysh> and the users next command: cat file1.

-mysh> cat file1

To implement a shell interface, rst let the parent process read what the user enters on the command line (in this case, cat prog.c), and then create a separate child process that performs the command. Unless otherwise specied, the parent process waits for the child to exit before continuing. However, UNIX shells typically also allow the child process to run in the background, or concurrently. To accomplish this, we add an ampersand (&) at the end of the command. Thus, if we rewrite the above command as mysh> cat file1 & the parent and child processes will run concurrently. Below given an outline for a simple shell. The separate child process is created using the fork() system call, and the user command is executed using one of the system calls in the exec() family, such as execvp(). This will require parsing what the user has entered into separate tokens and storing the tokens in an array of character strings, and pass them appropriately to execvp(). Be sure to check whether the user included an & to determine whether or not the parent process is to wait for the child to exit. In UNIX shell, there need not be a space before the &, but for your shell, you may assume the & is separated by space. The main() function presents the prompt mysh> and outlines the steps to be taken after input from the user has been read. The main() function continually loops as long as should run equals 1; when the user enters exit at the prompt, your program will set should run to 0 and terminate.

#include <stdio.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
int should run = 1; /* flag to determine when to exit program */
while (should run) {
printf("mysh>");
fflush(stdout);
/**
* After reading and parsing user input, the steps are:
* (1) fork a child process using fork()
* (2) the child process will invoke execvp()
* (3) if command included &, parent will invoke wait()
*/
}
return 0;
}

Your shell should provide the following features.
- When the user enters exit, your shell should terminate.
- If the user entered & following the command, the child process runs in the background, the shell should not block and wait but rather continue accepting commands. However, a shell with forked child processes executing in the background must be able to tell and acknowledge when the child processes exit. Here is how you can do this: The shell would receive a signal from the OS as soon as the child exits. Consult the signal() or sigaction() man pages if needed. The signal the OS sends to notify a parent process that a child has exited is SIGCHLD. Thus, you must register a handler for SIGCHLD. You have seen signal handler registration with the system call signal() or sigaction() before.
- Your shell needs to support input and output redirection (using < and > symbols). See slides on pipes on how to do this. You may assume that the redirection symbols (<, >) are space separated from the other arguments. 
- Your shell should provide a history feature that allows the user to access the most recently entered commands. The user will be able to access up to 10 commands by using the feature. The commands will be consecutively numbered starting at 1, and the numbering will continue past 10. For example, if the user has entered 35 commands, the 10 most recent commands will be numbered 26 to 35. The user will be able to list the command history by entering the command history at the mysh> prompt. As an example, assume that the history consists of the commands (from most to least recent): ps, ls -l, top, cal, who, date.

The command history will output:

6 ps
5 ls -l
4 top
3 cal
2 who
1 date

Your program should support two techniques for retrieving commands from the command history:
1. When the user enters !!, the most recent command in the history is executed. In the example below, ps will be performed.
2. When the user enters a single ! followed by an integer N, the Nth command in the history is executed. If the user enters !3 in the above example, the cal command will be executed.

Any command executed in this fashion should be echoed on the users screen. The command should also be placed in the history buer as the next command. The program should also manage basic error handling. If there are no commands in the history, entering !! should result in a message No commands in history. If there is no command corresponding to the number entered with the single !, the program should output "No such command in history."

You may assume that:
- Your shell does NOT need to support wildcard characters such as ls *.dat in parameters (or
commands).
- Your shell does NOT need to support pipes.
- Parameters are always separated by at least one whitespace character (including &, <, and > ).
- The maximum length of an individual command or parameter is at most N characters long (where N is a constant that you define in your code).
