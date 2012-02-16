/*
SISTEMAS OPERATIVOS - 2006/07
Trab #02 - Simulação de funcionamento de uma oficina de reparação de automóveis

Turma 3

Trabalho realizado por:
Cláudio Costa - 030509032
Rossana Fonseca - 030509092
Simão Castro - 040509100

*/



//Ficheiro que contém funçoes auxiliares
#include "auxiliares.h"
#include "teste.h"
#include "queue.h"
 
 
//Semáforos
#include "mysemops.h"
#include <sys/shm.h>

#define SEMKEY 900
int semid; //Semaforo Id

//Mutex Related
pthread_mutex_t mutexMec = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexEle = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCha = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPin = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexDone = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexRegis = PTHREAD_MUTEX_INITIALIZER;

//Posicao no parque
int posicao=0;
	
/**********************************************************
 ** Protótipos de Funções a serem utilizadas pelo Programa*
 **********************************************************/
void * recepcionista_C(void *args); 	//Thread Recepcionista_C
void * recepcionista_S(void *args); 	//Thread Recepcionista_S
void * mecanico(void *args); 			//Thread Mecanico
void * electricista(void *args); 		//Thread Electricista
void * chapeiro(void *args); 			//Thread Chapeiro
void * pintor(void *args); 				//Thread Pintor


//Queue's
Queue Qmec;		// queue mecanico
Queue Qele;		// queue electricista
Queue Qcha;		// queue chapeiro
Queue Qpin;		// queue pintor
Queue Qdone;    // queue final

//Variável para manter toda a gente a trabalhar
int work; 		//1 = Working , 2 = Out
int feito = 0;	// flag para sinalizar se todos os serviços estao acabados


/***********************************
  ************ MAIN ****************
  ***********Thread Principal*******
  **********************************/

 /* Chefe de Oficina
    * - Simular actividade do chefe de oficina
    * - Argumentos: - Duracao de funcionamento (uma unidade de tempo == 1 segundo)
    *			- Numero de trabalhadores de cada um dos tipos (mecanico, electricista, chapeiro, pintor)
    *
    * - Cria um logfie, registo.dat, aonde serão registadas todas as acções
    * - Cria um FIFO, chamado PEDIDOS, e todas as estruturas IPC...
    * - Abre "o parque" (região de memória partilhada, acessivel aos clientes) onde sao colocados os veiculos prontos
    * - Assinala o fecho da oficina via variavel global
    * - Aguarda no máximo 60 segundos até todos os clientes levantarem os veiculos prontos e fecha o parque (destroi zona de memoria partilhada)
    * - Antes de terminar deve destruir o FIFO PEDIDOS e todas as estruturas IPC criadas
    */ 

int main(int argc, char * argv[])
{
	int idblocom; //Identificador do Bloco de memória
	int fifod; //fifo file descriptor
	pthread_t tidSecr, tidReS;
	pthread_t *tidMec, *tidEle, *tidCha, *tidPin;
	int duracao,nMec, nEle, nCha, nPin;
	int i;
	
	Qmec = CreateQueue(15);		// queue mecanico
	Qele = CreateQueue(15);		// queue electricista
	Qcha = CreateQueue(15);		// queue chapeiro
	Qpin = CreateQueue(15);		// queue pintor	
	Qdone = CreateQueue(15);	//queue final
	
	if(argc!=6){		// verificação do input
		printf("\nUso: %s [TempodeFuncionamento] [nrMecanicos] [nrElectricistas] [nrChapeiros] [nrPintores]\n", argv[0]);
		return -1;
	}
	
	// guarda os argumentos do programa
	duracao = atoi(argv[1]);
	nMec = atoi(argv[2]);
	nEle = atoi(argv[3]);
	nCha = atoi(argv[4]);
	nPin = atoi(argv[5]);
	
	//abertura/Criação do ficheiro registos.dat
	if((registos = fopen(REGISTO, "wt"))==NULL){
		printf("Failled to create file: registos.dat .\n");
		return 1;
	}
	pthread_mutex_lock(&mutexRegis);
	regista(CHEFE, NULL, "Abriu a Oficina");
	pthread_mutex_unlock(&mutexRegis);

	//Criação do FIFO
	if(mkfifo(FIFO, 0660)!=0){
		perror(FIFO);
	}
	else
		fprintf(registos, "FIFO Criado\n");
	
	//Alocar memoria
	if((idblocom = aloca_mem(argv[0]))==1){
		return 1;
	}
	
	//Criar o Semaforo com key SEMKEY
	semid=semcreate(SEMKEY,0);
	if (semid==-1)
	{
		printf("Chefe: Erro na criacao do semaforo c/key = %d\n",SEMKEY);
		exit(1);
	}

	
	// Inicia o funcionamento da oficina!!
	work = 1;
	if(pthread_create(&tidSecr,NULL, recepcionista_C, (void *) &fifod)!=0){
		perror("Recepcionista C");
	}
	
	tidMec = (pthread_t *)calloc(nMec, sizeof(pthread_t));
	for(i=0; i<nMec; i++){		//Multi-thread
		if(pthread_create(&tidMec[i],NULL, mecanico, (void*)(i+1))!=0){
			perror("Mecanico");
		}
	}
	
	tidEle = (pthread_t *)calloc(nEle, sizeof(pthread_t));
	for(i=0; i<nEle; i++){		//Multi-thread
		if(pthread_create(&tidEle[i],NULL, electricista, (void*)(i+1))!=0){
			perror("Electricista");
		}
	}
	
	tidCha = (pthread_t *)calloc(nCha, sizeof(pthread_t));
	for(i=0; i<nCha; i++){		//Multi-thread
		if(pthread_create(&tidCha[i],NULL, chapeiro, (void*)(i+1))!=0){
			perror("Chapeiro");
		}
	}
	
	tidPin = (pthread_t *)calloc(nPin, sizeof(pthread_t));
	for(i=0; i<nPin; i++){		//Multi-thread
		if(pthread_create(&tidPin[i],NULL, pintor, (void*)(i+1))!=0){
			perror("Pintor");
		}
	}

	if(pthread_create(&tidReS,NULL, recepcionista_S,NULL)!=0){
		perror("Recepcionista S");
	}
	
	//O chefe pos a malta a trabalhar e agora vai tirar uma sesta...
	pthread_mutex_lock(&mutexRegis);
	regista(CHEFE, NULL, "Agora vou trabalhar... bem, dormir...");
	pthread_mutex_unlock(&mutexRegis);
	sleep(duracao);
	pthread_mutex_lock(&mutexRegis);
	regista(CHEFE,NULL, "Dei sinal para fechar a oficina! Bam'bora!");	
	pthread_mutex_unlock(&mutexRegis);
	work = 2;
	//printf("CHEFE: Dei sinal de terminar\n");
	
	sleep(60);	// aguarda 60 unidades de tempo
	feito = 1;	// encerra o funcionamento
	
	// aguarda que os threads terminem
	pthread_join(tidSecr,NULL);
	pthread_join(tidReS,NULL);
	
	for(i=0; i<nMec; i++) {
		pthread_join(tidMec[i],NULL); }

	for(i=0; i<nEle; i++) {
		pthread_join(tidEle[i],NULL); }
	
	for(i=0; i<nCha; i++) {
		pthread_join(tidCha[i],NULL); }
	
	for(i=0; i<nPin; i++) {
		pthread_join(tidPin[i],NULL); }	
		
		
	//desalocar a memoria
	if(desaloca_mem(idblocom) ==1){
		return 1;
	}
	else{
		pthread_mutex_lock(&mutexRegis);
		regista(CHEFE, NULL, "Parque fechado");
		pthread_mutex_unlock(&mutexRegis);
	}
	
	//elimina as filas
	//printf("Dispose of the queue...\n");
	DisposeQueue(Qmec);
	DisposeQueue(Qele);
	DisposeQueue(Qcha);
	DisposeQueue(Qpin);
	DisposeQueue(Qdone);
	
	
	//destroi os mutexes
	//printf("Destruir os mutexes\n");
	
	pthread_mutex_destroy(&mutexRegis);
	pthread_mutex_destroy(&mutexMec);
	pthread_mutex_destroy(&mutexEle);
	pthread_mutex_destroy(&mutexCha);
	pthread_mutex_destroy(&mutexPin);
	pthread_mutex_destroy(&mutexDone);

	//Destroi o semaforo
	//printf("Destruir o Semaforo\n");
	semremove(semid);
	
	unlink(FIFO);
	return 1;
	
	//adeus e boa viagem!
}


/***************************************************************************
  ***************************************************************************
  ********************** Zona de THREADS - Do Not Cross ***********************
  ***************************************************************************
  ***************************************************************************/
  
/* Thread Recepcionista C
  *
  * - Anota os pedidos dos clientes e faz o encaminhamento inicial dos veiculos, dentro da oficina
  * - (Lê do FIFO os pedidos dos clientes)
  * - Ordena a lista de serviços pedidos por um cliente // Consultar ficheiro servicos.dat
  *	- 1º Mecânico
  *	- 2º Electricista
  *	- 3º Chapeiro
  *	- 4º Pintor
  * - Coloca o "veículo" na fila de atendimento do primeiro serviço a ser realizado
  * - Sempre que não estiver a executar outras tarefas, deve estar à escuta no FIFO pedidos, que "chegue um novo veículo para ser atendido".
  */

void * recepcionista_C(void *args)
{
	Cliente cli_fifo;
	int j;
	int fifod = (int) args;
	int categoria[MAX_SERVICOS];
	pthread_mutex_lock(&mutexRegis);
	regista(RECEPC, NULL, "Bom dia chefe!");
	pthread_mutex_unlock(&mutexRegis);

	//printf("Estou no thread recepcionista\n");

	fifod = open(FIFO, O_RDONLY|O_NONBLOCK);
	//lê do FIFO
	while(work ==1){
		if(read(fifod, &cli_fifo, sizeof(Cliente))>0)
		{
			//printf("Estou a ler o pedido do cliente %d. com matricula %s\n", cli_fifo.id, cli_fifo.matricula);
			//printf("n_servicos > %d\n", cli_fifo.n_servicos);
			//printf("matricula > %s\n", cli_fifo.matricula);	
			le_servico(categoria);
			sortServico(cli_fifo.servicos, categoria, cli_fifo.n_servicos);
			

			for(j=0; j<cli_fifo.n_servicos; j++){
				printf("%d ", cli_fifo.servicos[j]);
			}
			printf("\n");
			/*
			Vamos lá a encher umas queues...
			Bloqueando o acesso às queues pelos outros threads através do mutex;
			*/
			//printf("CATEGORIA: %d \n", categoria[cli_fifo.servicos[0]-1]);
			switch(categoria[cli_fifo.servicos[0]-1]){
				case 1: 
					pthread_mutex_lock(&mutexMec);
					Enqueue(cli_fifo, Qmec);
					pthread_mutex_unlock(&mutexMec);			
					//printf("Queue 1\n");
					break;
				case 2:
					pthread_mutex_lock(&mutexEle);
					Enqueue(cli_fifo, Qele);
					pthread_mutex_unlock(&mutexEle);
					//printf("Queue 2\n");
					break;
				case 3:
					pthread_mutex_lock(&mutexCha);
					Enqueue(cli_fifo, Qcha);
					pthread_mutex_unlock(&mutexCha);
					//printf("Queue 3\n");
					break;
				case 4:
					pthread_mutex_lock(&mutexPin);
					Enqueue(cli_fifo, Qpin);
					pthread_mutex_unlock(&mutexPin);
					//printf("Queue 4\n");
					break;
			}
		}
	}
	close(fifod);
	//printf("RECEPS: A fechar...\n");	
	return NULL;
}

void * recepcionista_S(void *args)
{
	Cliente cli_ready;
	
	while(feito == 0)		// enquanto estiver em funcionamento
	{
		
		if(work == 2){		// quando receber ordem de fecho avisa os clientes que o carro nao esta pronto
			if(!IsEmpty(Qmec)){
				pthread_mutex_lock(&mutexDone);
				cli_ready = FrontAndDequeue(Qmec);
				pthread_mutex_unlock(&mutexDone);
				
				//informar o cliente que o carro não está terminado
				pthread_mutex_lock(&mutexRegis);
				regista(RECEPS, cli_ready.matricula, "Vou despachar o cliente com um SIGUSR2");
				pthread_mutex_unlock(&mutexRegis);
				kill(cli_ready.id, SIGUSR2);//enviado o sinal que o carro nao esta pronto
			}

			if(!IsEmpty(Qele)){
				pthread_mutex_lock(&mutexDone);
				cli_ready = FrontAndDequeue(Qele);
				pthread_mutex_unlock(&mutexDone);
				
				//informar o cliente que o carro não está terminado
				pthread_mutex_lock(&mutexRegis);
				regista(RECEPS, cli_ready.matricula, "Vou despachar o cliente com um SIGUSR2");
				pthread_mutex_unlock(&mutexRegis);
				kill(cli_ready.id, SIGUSR2);//enviado o sinal que o carro nao esta pronto
			}

			if(!IsEmpty(Qcha)){
				pthread_mutex_lock(&mutexDone);
				cli_ready = FrontAndDequeue(Qcha);
				pthread_mutex_unlock(&mutexDone);
				
				//informar o cliente que o carro não está terminado
				pthread_mutex_lock(&mutexRegis);
				regista(RECEPS, cli_ready.matricula, "Vou despachar o cliente com um SIGUSR2");
				pthread_mutex_unlock(&mutexRegis);
				kill(cli_ready.id, SIGUSR2);//enviado o sinal que o carro nao esta pronto
			}

			if(!IsEmpty(Qpin)){
				pthread_mutex_lock(&mutexDone);
				cli_ready = FrontAndDequeue(Qpin);
				pthread_mutex_unlock(&mutexDone);
				
				//informar o cliente que o carro não está terminado
				pthread_mutex_lock(&mutexRegis);
				regista(RECEPS, cli_ready.matricula, "Vou despachar o cliente com um SIGUSR2");
				pthread_mutex_unlock(&mutexRegis);
				kill(cli_ready.id, SIGUSR2);//enviado o sinal que o carro nao esta pronto
			}		
		}
		
		if(!IsEmpty(Qdone)){	// se tiver veiculos prontos
			pthread_mutex_lock(&mutexDone);
			cli_ready = FrontAndDequeue(Qdone);
			pthread_mutex_unlock(&mutexDone);
			
			//meter no parque
			pointer[posicao] = cli_ready;
			posicao++;
			
			//informar o cliente que o carro está no parque (pointer)
			pthread_mutex_lock(&mutexRegis);
			regista(RECEPS, cli_ready.matricula, "Vou despachar o cliente com um SIGUSR1");
			pthread_mutex_unlock(&mutexRegis);
			kill(cli_ready.id, SIGUSR1);		//enviado o sinal para tirar o carro do parque
			
			//Vou aguardar que o cliente retire o carro do parque
			semwait(semid);
		}
		else
		{
			if (work == 2) feito = 1;	// se passar da hora de fecho e todos os veiculos prontos tiverem sido levantados, nao espera mais!
		}
		
	}
	
	pthread_mutex_lock(&mutexRegis);
	regista(RECEPS,NULL,"Boa tarde chefe e ate amanha!");
	pthread_mutex_unlock(&mutexRegis);
	return NULL;
}

void * mecanico(void *args){

	Cliente clitratar; int i;
	int duracao;
	
	int id_thread = (int) args;
	char idMec[11];			// string para o registo do id do mecanico - p.e. "Mecanico 1"
	char servico[27];		// string para o registo do numero do servico - p.e. "Tou a tratar o servico 1!"
	sprintf(idMec, "Mecanico %d", id_thread);
	
	pthread_mutex_lock(&mutexRegis);
	regista(idMec, NULL, "Bom dia chefe, cheguei!");
	pthread_mutex_unlock(&mutexRegis);
	while(work == 1)
	{	
		if(!IsEmpty(Qmec)){	
			//Bloquer o acesso às Queues
			pthread_mutex_lock(&mutexMec);
			clitratar = FrontAndDequeue(Qmec);
			pthread_mutex_unlock(&mutexMec);
			
			pthread_mutex_lock(&mutexRegis);
			regista(idMec, clitratar.matricula, "Vou comecar a trabalhar");
			pthread_mutex_unlock(&mutexRegis);
			
			for(i=0;i<clitratar.n_servicos;i++){
				if(work != 1) break;
				pthread_mutex_lock(&mutexRegis);
				duracao = getDuracaoServico(clitratar.servicos[i], 1);
				pthread_mutex_unlock(&mutexRegis);
				if(duracao !=-1){ //Se for Diferente de -1 é porque existe e pode ser deste "Trabalhador"
					if(duracao !=0){ //Se for diferente de zero é dele
						//printf("Servico: %d - I: %d - Duracao: %d - Matricula: %s\n",clitratar.servicos[i], i, duracao, clitratar.matricula);

						sprintf(servico, "Tou a tratar o servico %d!", clitratar.servicos[i]);
						
						pthread_mutex_lock(&mutexRegis);
						regista(idMec, clitratar.matricula, servico);
						pthread_mutex_unlock(&mutexRegis);
						sleep(duracao);
					}
					//Se for igual a zero é porque é de alguém inferior a ele, por isso vai saltar
				}
				else{ //Deve ser do próximo trabalhador
					pthread_mutex_lock(&mutexEle);
					Enqueue(clitratar, Qele);		// envia o veiculo para a proxima fila
					pthread_mutex_unlock(&mutexEle);
					break;
				}

			}
		}
		else//Se não houver nenhum cliente na Fila, faz um pequeno sleep
			sleep(2);
	}
	pthread_mutex_lock(&mutexRegis);
	regista(idMec, NULL, "Ate amanha camaradas!");
	pthread_mutex_unlock(&mutexRegis);
	return NULL;
}


void * electricista(void *args){

	Cliente clitratar; int i;
	int duracao;
	
	int id_thread = (int) args;
	char idEle[11];			// string para o registo do id do electricista - p.e. "Electricista 1"
	char servico[27];		// string para o registo do numero do servico - p.e. "Tou a tratar o servico 1!"
	sprintf(idEle, "Electricista %d", id_thread);
	
	
	pthread_mutex_lock(&mutexRegis);
	regista(idEle, NULL, "Bom dia chefe, cheguei!");
	pthread_mutex_unlock(&mutexRegis);
	while(work == 1)
	{	
		if(!IsEmpty(Qele)){
			pthread_mutex_lock(&mutexEle);
			clitratar = FrontAndDequeue(Qele);
			pthread_mutex_unlock(&mutexEle);
			
			pthread_mutex_lock(&mutexRegis);
			regista(idEle, clitratar.matricula, "Vou comecar a trabalhar");
			pthread_mutex_unlock(&mutexRegis);
			
			for(i=0;i<clitratar.n_servicos;i++){
				if(work != 1) break;
				pthread_mutex_lock(&mutexRegis);
				duracao = getDuracaoServico(clitratar.servicos[i], 2);
				pthread_mutex_unlock(&mutexRegis);
				if(duracao !=-1){
					if(duracao!=0){
						sprintf(servico, "Tou a tratar o servico %d!", clitratar.servicos[i]);
						
						pthread_mutex_lock(&mutexRegis);
						regista(idEle, clitratar.matricula, servico);
						pthread_mutex_unlock(&mutexRegis);
						
						sleep(duracao);
					}
				}
				else{
					pthread_mutex_lock(&mutexCha);
					Enqueue(clitratar, Qcha);		// envia o veiculo para a proxima fila
					pthread_mutex_unlock(&mutexCha);
					break;
				}
			}
		}
		else//Se não houver clientes para tratar faz um sleep de 2 segundos
			sleep(2);
	}
	pthread_mutex_lock(&mutexRegis);
	regista(idEle, NULL, "Ate amanha camaradas!");
	pthread_mutex_unlock(&mutexRegis);
	
	return NULL;
}


void * chapeiro(void *args){

	Cliente clitratar; int i;
	int duracao;
	
	int id_thread = (int) args;
	char idCha[11];			// string para o registo do id do chapeiro - p.e. "Chapeiro 1"
	char servico[27];		// string para o registo do numero do servico - p.e. "Tou a tratar o servico 1!"
	sprintf(idCha, "Chapeiro %d", id_thread);
	
	
	pthread_mutex_lock(&mutexRegis);
	regista(idCha, NULL, "Bom dia chefe, cheguei!");
	pthread_mutex_unlock(&mutexRegis);
	while(work == 1)
	{	
		if(!IsEmpty(Qcha)){
			pthread_mutex_lock(&mutexCha);
			clitratar = FrontAndDequeue(Qcha);
			pthread_mutex_unlock(&mutexCha);
			
			pthread_mutex_lock(&mutexRegis);
			regista(idCha, clitratar.matricula, "Vou comecar a trabalhar");
			pthread_mutex_unlock(&mutexRegis);
			
			for(i=0;i<clitratar.n_servicos;i++){
				if(work != 1) break;
				pthread_mutex_lock(&mutexRegis);
				duracao = getDuracaoServico(clitratar.servicos[i], 3);
				pthread_mutex_unlock(&mutexRegis);
				if(duracao!=-1){
					if(duracao!=0){
						sprintf(servico, "Tou a tratar o servico %d!", clitratar.servicos[i]);
						
						pthread_mutex_lock(&mutexRegis);
						regista(idCha, clitratar.matricula, servico);
						pthread_mutex_unlock(&mutexRegis);
						
						sleep(duracao);
					}
				}
				else{
					pthread_mutex_lock(&mutexPin);
					Enqueue(clitratar, Qpin);		// envia o veiculo para a proxima fila
					pthread_mutex_unlock(&mutexPin);
					break;
				}
			}
		}
		else//Se nao houver carros para tratar, aguarda 2 segundos..
			sleep(2);
	}
	pthread_mutex_lock(&mutexRegis);
	regista(idCha, NULL, "Ate amanha camaradas!");
	pthread_mutex_unlock(&mutexRegis);
	
	return NULL;
}


void * pintor(void *args){

	Cliente clitratar;
	int i;
	int duracao;

	int id_thread = (int) args;
	char idPin[11];			// string para o registo do id do pintor - p.e. "Pintor 1"
	char servico[27];		// string para o registo do numero do servico - p.e. "Tou a tratar o servico 1!"
	sprintf(idPin, "Pintor %d", id_thread);	

	//Controla se o Cliente ja foi, ou nao, enviado para a fila da RecepcionistaS
	int enviado = 0;
	
	pthread_mutex_lock(&mutexRegis);
	regista(idPin, NULL, "Bom dia chefe, cheguei!");
	pthread_mutex_unlock(&mutexRegis);
	while(work ==1)
	{	
		if(!IsEmpty(Qpin)){
			pthread_mutex_lock(&mutexPin);
			clitratar = FrontAndDequeue(Qpin);
			pthread_mutex_unlock(&mutexPin);
			
			pthread_mutex_lock(&mutexRegis);
			regista(idPin, clitratar.matricula, "Vou comecar a trabalhar");
			pthread_mutex_unlock(&mutexRegis);
			
			for(i=0;i<clitratar.n_servicos;i++){
				if(work != 1) break;
				pthread_mutex_lock(&mutexRegis);
				duracao = getDuracaoServico(clitratar.servicos[i], 4);
				pthread_mutex_unlock(&mutexRegis);
				if(duracao!=-1){
					if(duracao!=0){
						sprintf(servico, "Tou a tratar o servico %d!", clitratar.servicos[i]);
						
						pthread_mutex_lock(&mutexRegis);
						regista(idPin, clitratar.matricula, servico);
						pthread_mutex_unlock(&mutexRegis);
						sleep(duracao);
					}
				}
				else{
					pthread_mutex_lock(&mutexRegis);
					regista(idPin, clitratar.matricula, "Ta pronto, vou mandar para a Doris");
					pthread_mutex_unlock(&mutexRegis);
					
					pthread_mutex_lock(&mutexDone);
					Enqueue(clitratar, Qdone);		// envia o veiculo para a proxima fila
					pthread_mutex_unlock(&mutexDone);
					enviado = 1;
					break;
				}
			}
			if(enviado!= 1){
				//Acabaram os serviços e vou mandar para a Recepcionista S
				pthread_mutex_lock(&mutexRegis);
				regista(idPin, clitratar.matricula, "Ta pronto, vou mandar para a Doris");
				pthread_mutex_unlock(&mutexRegis);
				pthread_mutex_lock(&mutexDone);
				Enqueue(clitratar, Qdone);
				pthread_mutex_unlock(&mutexDone);
			}
		}
		else{//Se nao houver nada para tratar, aguarda 2 segundos
			sleep(2);
			enviado = 0;
		}
	}
	pthread_mutex_lock(&mutexRegis);
	regista(idPin, NULL, "Ate amanha camaradas!");
	pthread_mutex_unlock(&mutexRegis);
	return NULL;
}
