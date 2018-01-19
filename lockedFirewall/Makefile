parallelPacket: parallelPacket.c locks.c locks.h
	gcc -pthread -Wall -O3 -std=c99 -o parallelPacket parallelPacket.c -lm	
parallelMake: parallelFirewall.c serialQueueMake
	gcc -pthread -Wall -O3 -std=c99 -o parallelFirewall parallelFirewall.c -lm

serialQueueMake: serialQueue.c
	gcc -pthread -Wall -O3 -std=c99 -o serialQueue serialQueue.c -lm
firewall: firewall.c
	gcc -pthread -Wall -O3 -std=c99 -o firewall firewall.c -lm
serialCounter: serialCounter.c
	gcc -Wall -O3 -std=c99 -o serialCounter serialCounter.c -lm
parallelCounter: parallelCounter.c locks.h locks.c
	gcc -pthread -Wall -O3 -std=c99 -o parallelCounter parallelCounter.c -lm

serialPacket: serialPacket.c
	gcc -pthread -Wall -O3 -std=c99 -o serialPacket serialPacket.c -lm

newParallelCounter: newParallelCounter.c locks.h locks.c
	gcc -pthread -Wall -O3 -std=c99 -o newParallelCounter newParallelCounter.c -lm
clean:
	rm -rf *.o parallelFirewall serialQueue parallelCounter
