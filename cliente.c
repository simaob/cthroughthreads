/*
SISTEMAS OPERATIVOS - 2006/07
Trab #02 - Simulação de funcionamento de uma oficina de reparação de automóveis

Turma 3

Trabalho realizado por:
Cláudio Costa - 030509032
Rossana Fonseca - 030509092
Simão Castro - 040509100

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <pthread.h> 
#include <time.h>

//Semáforos
#include "mysemops.h"
#include <sys/shm.h>

//Para Teste
#include "teste.h"

#define FIFONAME "pedidos"
#define MAX_FIFONAME 64

#define MAX_STRING 8
#define MAX_SERVICOS 20

//posicoes no parque
#define NPOSICOES 100 

//Semaforo
#define SEMKEY 900
int semid;


typedef struct {
 int	id;
 char	matricula[MAX_STRING];
 int	servicos[MAX_SERVICOS];
 int	n_servicos;
} Cliente;

//Apontar para a regiao de memoria partilhada
Cliente *pointer;


FILE* registos;

// SIGUSR
void sig_usr(int signo);
//Registar informaçoes do cliente
int registaCliente(char veiculo[], char * accao);

Cliente cliente;





/*
  *  FUNÇÃO MAIN
  */
int main(int argc, char *argv[])
{
	int		fd_FIFO;		// file descriptors
	pid_t	pid;	// PID do cliente
	FILE *	file;
	int i;
	
	//memoria partilhada
	key_t memkey;
	int shmid; //Shared Memory ID
	
	if(argc!=2)
	{
		printf("Instanciação errada! Modo de uso: %s <ficheiro com dados da viatura>\n", argv[0]);
		return 1;
	}
 
	if (signal(SIGUSR1,sig_usr) == SIG_ERR)
	{ printf("Can't catch SIGUSR1"); }
	
	if (signal(SIGUSR2,sig_usr) == SIG_ERR)
	{ printf("Can't catch SIGUSR2"); }
	
	//Semaforos
	 // Espera um pouco para a eventualidade de ainda nao terem sido criados
	for (i=0; i<3; i++) // ciclo para a eventualidade do sem. ainda nao ter sido criado
	{
		semid=semget(SEMKEY,0,0);
		if (semid!=-1) break;
			sleep(3);
	}
	if (semid==-1) 
	{
		printf("Cliente: Semaforo c/key = %d nao existe ?!\n",SEMKEY);
		exit(1);
	}

	memkey = ftok("chefe", 126); //126 é um número aleatório entre 0 e 255
	//Encontrar a região de memória partilhada criada pelo Chefe
	for (i=0; i<3; i++)
	{
		shmid = shmget(memkey,0,0);
		if (shmid!=-1) break;
			sleep(5);
	}
	if (shmid==-1)
	{
		printf("Cliente: Regiao de memoria partilhada c/key = %d nao existe ?!\n",memkey);
		exit(3);
	}
	
	//Attach da memoria partilhada
	pointer=(Cliente *)shmat(shmid, NULL, 0);


	
	// Abre o FIFO de pedidos (criado pelo chefe), em modo WRITE
	fd_FIFO = open(FIFONAME, O_WRONLY|O_NONBLOCK);
	if (fd_FIFO == -1) {			// o FIFO está fechado
		registaCliente(NULL,"Grrr! A oficina está fechada! (resmungo)");
		registaCliente(NULL,"Vou-me mas é embora. Raios!");
		perror(FIFONAME);
		exit(1);
	}
	
	//
	// Envia os dados do cliente para o FIFO
	//
	pid = getpid();
	cliente.id = pid;
	printf("cliente.id > %d\n", cliente.id);

	file = fopen(argv[1], "r");	// abre o ficheiro .dat do veiculo	
	if(file == NULL){
		printf("Erro ao abrir o ficheiro %s!\n", argv[1]);
		exit(1);
	}
	else
	{
		// abre e le o ficheiro
		cliente.n_servicos = 0;
		fscanf(file, "%s", cliente.matricula);
		printf("cliente.matricula > %s\n", cliente.matricula);
		while(fscanf(file, "%d", &cliente.servicos[cliente.n_servicos])!=-1){
			printf("cliente.servicos[%d] > %d\n", cliente.n_servicos, cliente.servicos[cliente.n_servicos]);
			cliente.n_servicos++;
		}
		fclose(file);	
		printf("n_servicos > %d\n", cliente.n_servicos);
	}
	
	write(fd_FIFO, &cliente, sizeof(Cliente));
	registaCliente(cliente.matricula, "Bom dia, aqui esta o meu popo");
	
	close(fd_FIFO);

	// Fica a aguardar notificacao pelo recepcionista
	printf("Agora estou à espera de ser notificado...\n");
	pause();
	
	//Detattch da memoria partilhada
	shmdt(pointer);

	return 1;
}

//Registar informaçoes do cliente
int registaCliente(char veiculo[], char * accao)
{
	char veic2[MAX_STRING];
	time_t tempo;
	struct tm tempo2;
	int pid = getpid();
	
	if(veiculo == NULL){
		strcpy(veic2,"        ");
	}
	else
		strcpy(veic2, veiculo);	
		
	tempo = time(NULL);
	localtime_r(&tempo, &tempo2);

	if((registos = fopen("registos.dat", "a+"))==NULL){
		printf("Failled to create file: registos.dat .\n");
		return 1;
	}
	if(tempo2.tm_hour<10)
		fprintf(registos, "0");
	fprintf(registos, "%d:",  tempo2.tm_hour);
	if(tempo2.tm_min<10)
		fprintf(registos,"0");
	fprintf(registos, "%d:",tempo2.tm_min);
	if(tempo2.tm_sec<10)
		fprintf(registos, "0");
	fprintf(registos, "%d Cliente %d    | %s | %s\n",tempo2.tm_sec, pid, veic2, accao);
	fclose(registos);
	return 1;
}


// SIGUSR
void sig_usr(int signo) /* argument is signal number */
{
	Cliente cli_get;int i;int j;
	if (signo == SIGUSR1){
		printf("E agora vou procurar o meu carro...\n");
		registaCliente(cliente.matricula,"Vou ao parque procurar o meu carrinho");
		for(i=0;i<NPOSICOES;i++){
			if(pointer[i].id == getpid()){
				cli_get = pointer[i];
				registaCliente(cliente.matricula,"Ja ca esta o meu carrinho");
				break;
			}
		}
		for(j = i;j<NPOSICOES;j++)
			pointer[j] = pointer[j+1];
		semsignal(semid);
	}
	else if (signo == SIGUSR2){ 
		printf("Vou-me lamentar ou protestar vigorosamente.\n");
		registaCliente(cliente.matricula,"Que raio de servico e este!!");
		registaCliente(cliente.matricula,"Este tempo todo e a viatura nao esta pronta?");
	}
}
