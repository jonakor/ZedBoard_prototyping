# ZedBoard Prototyping
Hardware .tcl code and software .c code for ZedBoard prototyping.
Using General Purpose Input/Output (GPIO) with interrupt to start and stop timer.

To set up the system:
 - Start up Vivado
 - Press "tools"->"Run TCL script"
 - Select the .tcl file in this gitrepo.
 - PS: If you're using windows, remember to change the path in #projectDir by editing the .tcl file

When SDK has launched:
 - Click "new" -> "Application project"
 - Make an .c project from template "Hello World"
 - Copy .c code from this gitrepo and paste into vivado main/helloworld.c
