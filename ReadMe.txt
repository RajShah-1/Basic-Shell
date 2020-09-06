=== Note ===
Hello.c, chck.c, test.txt, and tmp.txt are not required to run the shell.
These files are used to test various functionalities of the shell.

=== Setup ===
BasicSh.c contains the code for this basic shell.
The readline-8 library is used for keeping track of command history and showing the prompt.
It can be installed using the command sudo apt-get install libreadline-dev.
Run gcc -o BasicSh BasicSh.c to compile the source file and run the shell using ./BasicSh.

=== Prompt ===
Prompt is in the following format: 

{machine name}: {absolute path of the current directory} >>
ex: RajLaptopHP: /home/rajshah-1/Desktop/Assignment3>> 

=== Builtin Commands ===
1. ls
Shows all the contents of the current file

2. cd {path}
Changes current directory to 'path.'
Path can either be relative or absolute.
ex: absolute path: >cd /home/rajshah-1/Desktop
ex: relative path: >cd Assignment3

3. jobs
Lists all the processes running in the background.
A process can be run in the background by adding & commands.
There must be a white-space between & and the command.
ex: >./chck &
    >jobs
o/p:
Background jobs: 
NUM(pid): 6761 - ./chck &

4. kill %num
It kills the process corresponding to num in the output of jobs.
ex: >kill %6761 
It will kill the ./chck process running in the background.

5. history
Shows 10 previously used commands.
The previous commands can be run using !{ID} command.
!-1 will execute the last entered program.
NOTE: There is no space between ! and <ID>
ex: >history
o/p:
6 Previous Commands: 
ID: 0 - cd /home/rajshah-1/Desktop
ID: 1 - cd Assignment3
ID: 2 - ./chck &
ID: 3 - jobs
ID: 4 - kill %7150
ID: 5 - history
 >!2
o/p:
Executing: ./chck &

6. exit
Exits the shell if no processes are running in the background.
If there is at least one process running in the background, you must wait until it finishes
or you must kill it using the kill %num command.

=== User Commands ===
This shell also supports user commands. 
User commands can either use or absolute path or use a relative path.
User command can be pushed to the background by adding '&' at the command's end.
The stdin and stdout can also be redirected by using > {filename} and < {filename}
(There must be a single white space between > or <  and the file-name).
ex: ./Hello > test.txt < tmp.txt
This command will cause the program to take input from tmp.txt and
print output to test.txt.