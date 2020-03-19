NAME	:= timertest


all:
	@ echo "Building application with source files"
	arm-linux-gnueabihf-gcc -Wall c/timertest.c c/src/hyptimer.c -o $(NAME)
	@ echo "Build complete"

clean:
	rm -fr ./$(NAME)
