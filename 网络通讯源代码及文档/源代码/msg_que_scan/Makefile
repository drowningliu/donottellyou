CC=gcc
ARMCC=arm-none-linux-gnueabi-gcc
CPPFLAGS= -I./incl
CFLAGS=-Wall -g
LIBPATH = -L /usr/lib
LIBS= -lpthread -lm -ldl


#找到当前目录下所有的.c文件
src = $(wildcard ./src/*.c)

#将当前目录下所有的.c  转换成.o给obj
obj = $(patsubst %.c, %.o, $(src))



scan_lib = libscan_ipc.so
test_scan_ipc = test_scan_ipc

target = $(test_scan_ipc)  $(scan_lib)  
ALL:$(target)
               
#生成所有的.o文件
$(obj):%.o:%.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)    	
       

#test_scan_ipc程序
$(test_scan_ipc): ./src/IPC_shmory.o ./src/msg_que_scan.o  ./src/socklog.o ./src/scan_ipc.o ./src/test_scan_ipc.o
	$(CC) $^ -o $@ $(CPPFLAGS)  $(LIBPATH) $(LIBS) $(CFLAGS)

#scan_lib程序
$(scan_lib): ./src/IPC_shmory.c ./src/msg_que_scan.c  ./src/socklog.c ./src/scan_ipc.c 
	$(CC) $^ -fPIC -shared -o $@ $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)

arm:
	$(ARMCC) ./src/IPC_shmory.c ./src/msg_que_scan.c  ./src/socklog.c ./src/scan_ipc.c  -fPIC -shared -o $(scan_lib) $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)

#clean指令

clean:
	-rm -rf $(obj) $(target) ./src/*.o


#将clean目标 改成一个虚拟符号
.PHONY: clean ALL cp arm
