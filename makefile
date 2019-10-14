all:
	@echo "Compiling client..."
	gcc client.c -o client
	@echo "Compiling server..."
	gcc server.c -o server
clean:
	@echo "Cleaning compiled files..."
	rm -rf *.o
	rm server
	rm client
