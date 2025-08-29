CC = gcc
CFLAGS = -Wall -O2 -Wno-format-zero-length

all: zjdcli_debug zjdcli zjdtest_debug zjdtest

zjdcli_debug:
	$(CC) -DZJD_DEBUG=1 -I zjpgd main.c  zjpgd/zjpgd.c -o zjdcli_debug

zjdcli:
	$(CC) -DZJD_DEBUG=0 -I zjpgd main.c  zjpgd/zjpgd.c -o zjdcli

zjdtest_debug:
	$(CC) -DZJD_DEBUG=1 -I zjpgd test.c  zjpgd/zjpgd.c -o zjdtest_debug

zjdtest:
	$(CC) -DZJD_DEBUG=0 -I zjpgd test.c  zjpgd/zjpgd.c -o zjdtest

clean:
	rm -f zjdcli* zjdtest*

