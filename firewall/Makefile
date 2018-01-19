parallelMake: parallelFirewall.c serialQueueMake
	gcc -pthread -Wall -O3 -std=c99 -o parallelFirewall parallelFirewall.c -lm

serialQueueMake: serialQueue.c
	gcc -pthread -Wall -O3 -std=c99 -o serialQueue serialQueue.c -lm

clean:
	rm -rf *.o parallelFirewall serialQueue
