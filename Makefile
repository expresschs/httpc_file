CC=gcc

httpc_file: main.c httpc_file.c
	$(CC) -o $@ $^ 

clean:
	rm -f *.o httpc_file

