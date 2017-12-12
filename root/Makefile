OBJECTS=tulu.o MyRio.o IRQConfigure.o NiFpga.o TimerIRQ.o AIO.o
OBJ=tulu.c fpga/MyRio.c fpga/IRQConfigure.c fpga/NiFpga.c TimerIRQ.c AIO.c
OUT=tuluken

sysroot=~/oecore-x86_64/sysroots/cortexa9-vfpv3-nilrt-linux-gnueabi/
isphinxbase=/include/sphinxbase
ipocketsphinx=/include/pocketsphinx
dot=../..
dotisphinxbase=$(dot)$(isphinxbase)
dotipocketsphinx=$(dot)$(ipocketsphinx)
PFLAGS=-D MyRio_1950 -D NiFpga_Linux -DDICTFILE=\"kws.dic\" -DKWSFILE=\"kws.list\" -DHMMDIR=\"en-us\" -DHI=5 -DLO=0 -DQ=0.000076295
CFLAGS=-I./fpga -I$(dotisphinxbase) -I$(dotipocketsphinx) -I$(dot)/include -I$(dot)/usr/include -c

LDFLAGS=-lsphinxbase -lpocketsphinx -lsphinxad -lvisa -ldl -lpthread

LIBPATH+=-L$(sysroot)usr/lib 

compile: $(OBJECTS)
	$(CC) $(PFLAGS) $(OBJECTS) $(LDFLAGS) $(LIBPATH) -o $(OUT)
	rm -f *~ *.o

$(OBJECTS): $(OBJ)
	$(CC) $(CFLAGS) $(PFLAGS) $(OBJ)

.PHONY: clean
clean:
	rm -f *~ *.o $(OUT)
