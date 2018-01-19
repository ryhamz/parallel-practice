serial: firewall.c Utils
	gcc -pthread -Wall -O3 -std=gnu99 -o firewall firewall.c -lm

parallel: parallel.c Utils/
	gcc -pthread -Wall -O3 -std=gnu99 -o parallel parallel.c -lm

noLoad: noLoad.c Utils
	gcc -pthread -Wall -O3 -std=gnu99 -o noLoad noLoad.c -lm
