
virtmem: main.o page_table.o disk.o program.o
	gcc main.o page_table.o disk.o program.o -o virtmem

main.o: main.c
	gcc -std=gnu11 -Wall -Werror -g -c main.c -o main.o

page_table.o: page_table.c
	gcc -std=gnu11 -Wall -Werror -g -c page_table.c -o page_table.o

disk.o: disk.c
	gcc -std=gnu11 -Wall -Werror -g -c disk.c -o disk.o

program.o: program.c
	gcc -std=gnu11 -Wall -Werror -g -c program.c -o program.o


clean:
	rm -f *.o virtmem
