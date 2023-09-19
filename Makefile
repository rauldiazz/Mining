CC=gcc
EXE1 = miner
EXE2 = monitor
EXE3 = funciones
EXE4 = funciones_monitor

LIB = -lpthread -lrt

exe:
	make clean
	make all 
	./miner 6 50

all: $(EXE1) $(EXE2)

$(EXE1):
	$(CC) $(EXE1).c $(EXE3).c -o $@ $(LIB) -Wall

$(EXE2):
	$(CC) $(EXE2).c $(EXE4).c -o $@ $(LIB) -Wall

$(EXE3): 
	$(CC) -c $(EXE3).c -Wall

$(EXE4):
	$(CC) -c $(EXE4).c -Wall

clean:
	rm -f *.o $(EXE1) $(EXE2)

cleanall:
	rm -f *.o $(EXE1) $(EXE2) /dev/mqueue/mq_monitor /dev/shm/netdata /dev/shm/block log.txt