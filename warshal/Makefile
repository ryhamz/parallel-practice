parallelmake: parallelFW.c
	gcc -pthread -o parallelFW parallelFW.c -Wall -O3

fw: fw.c
	gcc -pthread -O3 -Wall -Werror -o fw fw.c

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
