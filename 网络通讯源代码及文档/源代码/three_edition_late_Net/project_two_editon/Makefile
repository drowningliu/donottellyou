CC=gcc
ARMCC=arm-none-linux-gnueabi-gcc
CPPFLAGS= -I./incl
CFLAGS=-Wall -g
LIBPATH = -L /usr/lib
#LIBS= -lssl -lcrypto -lpthread -lm -ldl
LIBS=  -lpthread -lm -ldl


#找到当前目录下所有的.c文件
src = $(wildcard ./src/*.c)

#将当前目录下所有的.c  转换成.o给obj
obj = $(patsubst %.c, %.o, $(src))


server_simple = server_simple
test_client_ui = test_client_ui
libtest_package_time = libnet_transfer.so
test_dhcp = test_dhcp


#target = $(server_simple)  $(test_client_ui)    $(test_dhcp) $(libtest_package_time)
target =   $(server_simple)  $(test_client_ui)  $(libtest_package_time)
ALL:$(target)
    
#生成所有的.o文件
$(obj):%.o:%.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS) 

#./src/cldMemPool.o
#test_client_ui程序
$(test_client_ui):./src/test_package.o ./src/escape_char.o  ./src/socklog.o ./src/queue_sendAddr_dataLenth.o ./src/data_package_time.o ./src/memory_pool.o ./src/sockclient.o ./src/dictionary.o ./src/dnsAddr.o ./src/sockclient.o ./src/fifo.o ./src/iniparser.o ./src/link_list.o ./src/queue.o ./src/queue_int.o ./src/queue_buffer.o ./src/myarp.o ./src/autoip.o ./src/client.o ./src/config.o ./src/dhcp.o ./src/handler.o  ./src/netstatus.o ./src/setmsg.o ./src/udpipgen.o ./src/func_compont.o ./src/list_index.o
	$(CC) $^ -o $@ $(CPPFLAGS)  $(LIBPATH) $(LIBS) $(CFLAGS)

#server_simple程序
$(server_simple):./src/server_simple.o ./src/escape_char.o ./src/func_compont.o ./src/socklog.o ./src/msg_que_scan.o  ./src/memory_pool.o ./src/test_server.o  ./src/queue_sendAddr_dataLenth.o
	$(CC) $^ -o $@ $(CPPFLAGS)  $(LIBPATH) $(LIBS) $(CFLAGS)	
	
#(libtest_package_time) operating procedure
$(libtest_package_time): ./src/escape_char.c ./src/socklog.c ./src/data_package_time.c ./src/func_compont.c ./src/queue_sendAddr_dataLenth.c ./src/memory_pool.c  ./src/dictionary.c ./src/dnsAddr.c ./src/sockclient.c ./src/fifo.c ./src/iniparser.c ./src/link_list.c ./src/queue.c ./src/queue_int.c ./src/queue_buffer.c ./src/myarp.c ./src/autoip.c ./src/client.c ./src/config.c ./src/dhcp.c ./src/handler.c  ./src/netstatus.c ./src/setmsg.c ./src/udpipgen.c ./src/list_index.c
	$(CC) $^ -fPIC -shared -o $@ $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)

#test_dhcp程序
$(test_dhcp):./src/test_dhcp.o  ./src/myarp.o ./src/autoip.o ./src/client.o ./src/config.o ./src/dhcp.o ./src/handler.o  ./src/netstatus.o ./src/setmsg.o ./src/udpipgen.o 
	$(CC) $^ -o $@ $(CPPFLAGS)  $(LIBPATH) $(LIBS) $(CFLAGS)

arm:
	$(ARMCC) ./src/escape_char.c ./src/socklog.c ./src/data_package_time.c ./src/func_compont.c ./src/queue_sendAddr_dataLenth.c ./src/memory_pool.c  ./src/dictionary.c ./src/dnsAddr.c ./src/sockclient.c ./src/fifo.c ./src/iniparser.c ./src/link_list.c ./src/queue.c ./src/queue_int.c ./src/queue_buffer.c ./src/myarp.c ./src/autoip.c ./src/client.c ./src/config.c ./src/dhcp.c ./src/handler.c  ./src/netstatus.c ./src/setmsg.c ./src/udpipgen.c ./src/list_index.c -fPIC -shared -o  $(libtest_package_time) $(CPPFLAGS) $(LIBPATH) $(LIBS) $(CFLAGS)

#clean指令

clean:
	-rm -rf $(obj) $(target) ./src/*.o
cp:
	-cp ./libtest_package_time.so /home/lsh/share/NH_1200/NH_1200_X86/lib/

#将clean目标 改成一个虚拟符号
.PHONY: clean ALL cp arm
