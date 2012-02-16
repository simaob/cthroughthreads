#ifndef _AUXILIARES_H_
#define _AUXILIARES_H_


#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

//Necessários para tratamento do "Parque" (Memória Partilhada)
#include <sys/ipc.h>
#include <sys/shm.h>


//Necessário para ler e escrever no FIFO + close FIFO + "destroy" FIFO
#include <unistd.h>


//Necessárias para tratar o FIFO
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/************************************
 ************   Defines  ************
 ************************************/
 //FIFO
#define FIFO "pedidos"
#define FIFOBUF 100
//Memória Partilhada
//posicoes no parque
#define NPOSICOES 100 


//Ficheiro Registos.dat
#define REGISTO "registos.dat"


//Queue's
//#define Error(Str)        FatalError(Str)
//#define FatalError(Str)   fprintf(stderr, "%s\n", Str), exit(1)
//#define MinQueueSize (5)

//Para uso na estrutura Cliente
#define MAX_STRING 8
#define MAX_SERVICOS 20

//Cargos 
#define CHEFE "Chefe da Oficina"
#define RECEPC "Recepcionista_C"
#define RECEPS "Recepcionista_S"




//Estruturas
typedef struct {
 int	id;
 char	matricula[8];
 int	servicos[MAX_SERVICOS];
 int	n_servicos;
} Cliente;


/**********************************
 ********** Variáveis Globais *****
 **********************************/
//Variável para manter o ficheiro de registo de actividades  
 FILE* registos;

//Pointer para a memória partilhada
Cliente *pointer;


int aloca_mem(char * nomeprog);
int desaloca_mem(int idblocom);
int regista(char * autor, char veiculo[], char * accao);
int le_servico(int serv[]);
void sortServico(int v[], int serv[], int size);
int getDuracaoServico(int servico, int categoria);


#endif
