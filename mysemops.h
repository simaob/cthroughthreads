// SIMPLICACAO DAS OPERACOES BASICAS SOBRE SEMAFOROS DO SYSTEM V

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SEM_MODE 0600

//static struct sembuf op_arr[1] = {0,0,SEM_UNDO}; //cuidado c/SEM_UNDO em Linux (ver slides)
static struct sembuf op_arr[1] = {{0,0,0}};

int semcreate(key_t key, int initval)
{
 union semun {
  int              val;     
  struct semid_ds  *buf;    
  unsigned short   *array; 
  struct seminfo   *__buf;  // LINUX SPECIFIC 
 } semctl_arg;

 int id = semget(key, 1, IPC_CREAT|IPC_EXCL|SEM_MODE);
 if (id==-1) return -1;
 semctl_arg.val = initval;
 semctl(id, 0, SETVAL, semctl_arg);
 return(id);
}

int semwait(int id)
{
 op_arr[0].sem_op = -1;
 return (semop(id, &op_arr[0], 1));
}

int semsignal(int id)
{
 op_arr[0].sem_op = 1;
 return (semop(id, &op_arr[0], 1));
}

int semremove(int id)
{
 return (semctl(id, 0, IPC_RMID, 0));
}

