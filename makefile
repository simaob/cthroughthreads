CC = gcc


all: cliente chefe
	@echo Todos os ficheiros foram compilados

cliente: cliente.o
	$(CC) -o cliente cliente.o -D_REENTRANT -lpthread -Wall
	@echo Ficheiro cliente compilado

chefe: chefe.o auxiliares.o queue.o
	$(CC) -o chefe chefe.o auxiliares.o queue.o -D_REENTRANT -lpthread -Wall
	@echo Ficheiro chefe compilado
	
%.o: %.c
	$(CC) -c $< -Wall
		
clean:
	rm *.o 
	@echo Removidos todos os .exe e .o
	
delfifo:
	rm Pedidos
	@echo Removido FIFO com sucesso
