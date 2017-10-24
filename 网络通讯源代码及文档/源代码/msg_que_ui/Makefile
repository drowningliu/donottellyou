CC=gcc
ARMCC=arm-none-linux-gnueabi-gcc
CPPFLAGS= -I./incl
CFLAGS=-Wall -g
LIBPATH = -L /usr/lib 
LIBS=  -lpthread -lm -ldl


#ergodic specified directory *.c file
src = $(wildcard ./shmory/*.c)

#transform *.c  source program to .o filename
obj = $(patsubst %.c, %.o, $(src))

#program objctive  
msg_que_ui = libmsg_que_ui.so
test_msg_ipc_ui = test_msg_ipc_ui

#The target makefile 
target =  $(msg_que_ui) $(test_msg_ipc_ui)



#begin
ALL:$(target)    

#transform  all .c file  to .o file
$(obj):%.o:%.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)

#(test_msg_ipc_ui) operating procedure
$(test_msg_ipc_ui):./src/socklog.o  ./src/msg_que_ui.o  ./src/test_msg_ipc_ui.o ./src/ui_ipc.o ./src/msg_shm_ui.o
	$(CC) $^ -o $@ $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)

#(msg_que_ui) operating procedure
$(msg_que_ui):./src/socklog.c  ./src/msg_que_ui.c ./src/ui_ipc.c ./src/msg_shm_ui.c
	$(CC) $^ -fPIC -shared -o $@ $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)


arm:
	$(ARMCC) ./src/socklog.c  ./src/msg_que_ui.c ./src/ui_ipc.c ./src/msg_shm_ui.c -fPIC -shared -o $(msg_que_ui) $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)

#clean cmd
clean:
	-rm -rf $(obj) $(target) ./src/*.o 

cp:
	-cp $(msg_que_ui) /home/lsh/share/NH_1200/lib

# makefile cmd statement
.PHONY: clean ALL cp arm
