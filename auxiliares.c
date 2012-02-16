#include "auxiliares.h"

/**********************************************
  *********************************************
  ********* Fun��es de Mem�ria Partilhada *****
  *********************************************
  *********************************************/
int aloca_mem(char * nomeprog){
	
	key_t memkey; //Key utilizada na cria��o da mem�ria partilhada
	int auxiliar; //Guardar o Id do Bloco de Mem�ria que ser� returnado..
	
	//Pedir uma memkey n�o utilizada
	memkey = ftok("chefe", 126); //126 � um n�mero aleat�rio entre 0 e 255
	
	//Cria��o de espa�o de mem�ria partilhada com defini��o de permiss�es de escrita e leitura
	if((auxiliar = shmget(memkey, NPOSICOES*sizeof(Cliente) , IPC_CREAT|IPC_EXCL|0600))==-1){/*IPC_CREAT|IPC_EXCL|SHM_R|SHM_W|SHM_R>>3|SHM_W>>3|SHM_R>>6|SHM_W>>6*/
		printf("shmget: Failled. Falhou a aloca��o de mem�ria\n");
		return 1;
	}
	else
		regista(CHEFE, NULL, "Parque aberto");
	
	//attach da mem�ria
	if((pointer = (Cliente *) shmat(auxiliar, 0, 0)) == NULL){//0 para deixar o sistema escolher o endere�o l�gico de mapeamento | e 0 pois nao se pretende apenas ler do bloco partilhado
		//perror((char *) pointer);
		printf("Shmat: Falhou!\n");
		return 1;
	}
	
	return auxiliar;
}

int desaloca_mem(int idblocom){

	//Desassocia��o da mem�ria
	shmdt(&pointer);
	
	
	printf("%d - IdBlocom\n", idblocom);
	//Libertar completamente o bloco de mem�ria
	if(shmctl(idblocom, IPC_RMID, NULL)==-1){
		printf("shmctl: Failled. Falhou a liberta��o de mem�ria partilhada.\n");
		return 1;
	}
	return 0;
}


/**********************************
 ********* Fun��es Auxiliares *****
 **********************************/
 

//Fun��o respons�vel por escrever uniformemente  no ficheiro registo.dat
//Retorna 1 se ok, 0 se der erro
int regista(char * autor, char veiculo[], char * accao)
{
	int i;
	char veic2[MAX_STRING];
	char * aux;
	
	time_t tempo;
	struct tm tempo2;
	
	if(veiculo == NULL){
		strcpy(veic2,"        ");
	}
	else
		strcpy(veic2, veiculo);
	
	if(autor == NULL){
		printf("Necess�rio especificar o autor.\n");
		return 0;
	}

	
	aux = (char *) malloc(16*sizeof(char));
	for(i=0;i<16;i++){
		if(i>=strlen(autor))
			aux[i] = ' ';
		else
			aux[i] = autor[i];
	}
	
	tempo = time(NULL);
	localtime_r(&tempo, &tempo2);

	if((registos = fopen(REGISTO, "a+"))==NULL){
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
	fprintf(registos, "%d %s | %s | %s\n",tempo2.tm_sec, aux, veic2, accao);
	free(aux);
	fclose(registos);
	return 1;
	
}

// fun��o que ordena o vector de servi�os consoante a sua categoria (independentemente da ordenacao do ficheiro servicos.dat)
void sortServico(int v[], int serv[], int size)
{
	int i = 0;
	int de = -1; 	//origem da troca
	int temp = 0;
	int cat = 1;		// categoria de servi�o
	int flag = 0;
	//int j;
	
	do
	{
			
		if(serv[v[i]-1] == cat){	// se o servico seleccionado for da categoria cat
			if(de == -1){			// se nao estiver � procura de um servico da categoria cat para troca, quer dizer que "est� no sitio"
				i++;				// e passa ao servio seguinte
			}
			else					// encontrou um servico da categoria cat para troca
			{
				v[de] = v[i];		// faz a troca
				v[i] = temp;
				
				if(serv[v[i]-1] != cat){	// se o servico pelo qual trocou nao pertencer a categoria cat
					de++;					// come�a a procurar pelo seguinte
					i = de;
					temp = v[i];			// guardando o seu valor em  temp
				}
				else{	//desnecessario?
					de = -1;}
			}
		}
		else						// se o servico seleccionado NAO for da categoria cat
		{
			if(de == -1){			// se nao estiver � procura de um servico da categoria cat para troca, quer dizer que "est� no sitio"
				de = i;				// esse servico passa a estar elegivel para troca
				temp = v[i];		// guardando o seu valor em temp
			}
			i++;
		}
		
		if(i == size)				// se atingir o final do vector
		{
			i = de;					// ir� iniciar a partir do �ltimo servi�o ordenado
			de = -1;				// nao est� mais � procura de um servico para troca
			cat++;					// passa a proxima categoria
			//printf("Prox cat: %d\n", cat);
		}
		if(cat>4) flag = 1;			// se nao houver mais categorias activa a flag e sai
		
	}while(flag == 0);
	//printf("Acabei\n\n");
	
}


// fun��o que l� os servi�os do ficheiro servicos.dat e regista a sua categoria, na posicao respectiva do array (cat[servico]: cat(1)|cat(2)|cat(3)|...)
int le_servico(int serv[])
{
	FILE *file;
	char *line = (char*)malloc(64*sizeof(char));
	int codigo;
	char designacao[64];
	int duracao;
	int i=0;
	
	if((file = fopen("servicos.dat","r")) ==NULL){// abre o ficheiro servicos.dat
		printf("Falhou a abertura do ficheiro: servicos.dat\n");
		return 1;
	}
	
	// abre e le o ficheiro
	while( fgets(line, 64, file)!=NULL )		// le uma linha
	{
		codigo = atoi(strtok(line, "-"));		// guarda o codigo
		strcpy(designacao, strtok(NULL, "-"));	// guarda a designacao
		serv[i] = atoi(strtok(NULL, "-"));		// guarda a categoria no array
		duracao = atoi(strtok(NULL, "-"));		// guarda a duracao
		i++;
	}
	serv[i] = -1;								// sinalizacao de fim do array

	free(line);
	fclose(file);	

	return 1;
}

int getDuracaoServico(int servico, int categoria){
	FILE *file;
	int duracao = -1;
	int categoria2; //Auxiliar para saber se o servi�o � de alguem antes ou depois (Se for antes, avan�ar�, se for depois passar�)
	
	char *line = (char*)malloc(64*sizeof(char));
	
	if((file = fopen("servicos.dat","r")) ==NULL){
		printf("Falhou a abertura do ficheiro: servicos.dat\n");
		return 1;
	}	
	while(fgets(line, 64, file)!=NULL )		// le uma linha
	{
		//Considerando que n�o h� dois servicos com o mesmo C�digo
		if(atoi(strtok(line, "-"))==servico)		// codigo
		{
			strtok(NULL, "-");	// designacao
			if(categoria == (categoria2 = atoi(strtok(NULL, "-")))){		// Verifica se est� na categoria correcta
				duracao = atoi(strtok(NULL, "-"));		// guarda a duracao
				free(line);
				fclose(file);
				return duracao;
			}
			//Se existir o servi�o e for de outra categoria
			else return (categoria > categoria2) ? 0 : -1;
		}
	}
	free(line);
	fclose(file);
	return duracao;
}
