#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 30
#define buffer_filled buffer[BUFFER_SIZE]
#define toreceive buffer[BUFFER_SIZE+1]
char* buffer;
int toproduce;
//int toreceive;
int tests;

key_t key;
int shmid, semid;
struct sembuf sem = {0,0,0};

enum {USED, RECA, RECB, TOREMOVE};

/**
USED - buffor used
RECA - consument A received letter
RECB - consument B received letter
TOREMOVE - information about how many elements we can delete from bufor
**/

void start(int);
void pushLetter(char);
void popLetter();
void end();

void producerA();
void producerB();

void consumerA();
void consumerB();

int callRemove();
int callProcesses();

int up(int);
int down(int);
int mut_up(int);
int mut_down(int);

char* assign(int);
void detach(char*);

int main(int argc, char **argv)
{
	int y ;
	time_t t;
	
	srand ((unsigned) time(&t));
	if (argc==2){
		y = atoi(argv[1]);
		tests=1;
		if (y < 1) {fprintf(stderr,"ERROR bad argument\n"); return 1;}
		start(y);
	}
	else
	{
		tests = 0;
  		start(0);
	}
  	end();

  	return 0;
}

void start(int y)
{
  int i;
  int toprod;
	if(tests==0) {
  		printf("Letters to produce: ");
  		scanf("%d", &toprod);
		if (toprod <1) {fprintf(stderr,"ERROR bad argument\n"); return ;}
	}
	else
	toprod = y;

  key = ftok(".", 1);
  shmid = shmget(key, BUFFER_SIZE + 3, 0600 | IPC_CREAT);
  semid = semget(key, 4, 0600 | IPC_CREAT);

  buffer = assign(shmid);
  buffer_filled = 0;

  for(i = 0; i<BUFFER_SIZE; ++i)
    buffer[i] = 0;


  semctl(semid, USED, SETVAL, 1);
  semctl(semid, RECA, SETVAL, 1);
  semctl(semid, RECB, SETVAL, 1);
  semctl(semid, TOREMOVE, SETVAL, 0);
  toproduce = toprod;
  toreceive = ((toproduce*3)-3);
  shmdt(buffer);

  callProcesses();
  end();
}

void end()
{
  int procleft=4;
  for(;procleft>0;--procleft)
    wait(NULL);

  semctl(semid, 0, IPC_RMID);
  shmctl(shmid, IPC_RMID, NULL);

}

int callProcesses()
{
  int pid;

  //call ProducerA
  pid = fork();
  if(pid == 0)
  {
	if (tests) puts("Start ProducerA");
    producerA();
	if (tests) puts("End ProducerA");
    return 1;
  }

  //call ProducerB
  pid = fork();
  if(pid == 0)
  {
   if (tests) puts("Start ProducerB");
    producerB();
   if (tests) puts("End ProducerB");
    return 1;
  }

  //call consumerA
  pid = fork();
  if(pid == 0)
  {
    if (tests) puts("Start ConsumerA");
    consumerA();
    if (tests) puts("End ConsumerA");
    return 1;
  }

  //call consumerB
  pid = fork();
  if(pid == 0)
  {
    if (tests) puts("Start ConsumerB");
    consumerB();
    if (tests) puts("End ConsumerB");
    return 1;
  }

	//call remove
  pid = fork();
  if(pid == 0)
  {
    if(tests) puts("Start CallRemove");
    callRemove();
    if(tests) puts("End CallRemove");
    return 1;
  }

  return 0;

}

void producerA()
{
   while (1){
	buffer = assign(shmid) ;
	if(toproduce<=0) return;
	if (tests) printf("A will produce: %d\n", toproduce);
	
	int x;
	printf("PRODUCER A is generating letter\n");
	mut_down(USED);		
		pushLetter('A');
	mut_up(USED);
	
	detach(buffer) ;
	usleep(rand()%500000+500000);

	}

}
void producerB()
{
	while (1){
	buffer = assign(shmid) ;;
	if(toproduce<=0) return;
	if(tests) printf("B will produce: %d\n", toproduce);
	
	int x;
	printf("PRODUCER B is generating letter\n");
	mut_down(USED);
	
	++toproduce;
		pushLetter('B');
		pushLetter('B');
	mut_up(USED);

	detach(buffer) ;

	usleep(rand()%500000+500000);
	}
}

void consumerA()
{
while(1)
  {
    	buffer = assign(shmid) ;

	if(toreceive<=0)return;

	mut_down(RECA);
 


    	

  	mut_down(USED);


    	
      	printf("CONSUMER A READED: %c \n", buffer[0]);
     	mut_up(USED);


	
    detach(buffer) ;



	usleep(rand()%400000+100000);
  }

}

void consumerB()
{

while(1)
  {
    buffer = assign(shmid) ;

    if(toreceive<=0)return;

    mut_down(RECB);

    mut_down(USED);
	

    printf("CONSUMER B READED: %c \n", buffer[0]);
    mut_up(USED);
	
    detach(buffer) ;
	usleep(rand()%400000+100000);
  }

}

int callRemove()
{
	while (1){
	buffer = assign(shmid) ;
		if(toreceive==0)
			return 1;
if (tests) puts("Remover start");
		
	down(TOREMOVE);
	
	mut_down(USED);

        popLetter();

      	mut_up(USED);
      	mut_up(RECA);
      	mut_up(RECB);

    detach(buffer) ;

if (tests)  puts("REMOVER END");
    usleep(rand()%400000+100000);
}
    return 1;
	
}

void popLetter()
{

buffer = assign(shmid) ;
    printf("letter popped\t before: ");

    char popped = buffer[0];
    int i;
    for(i = 0; i <= buffer_filled; ++i)
      printf("%c", buffer[i]);

    --buffer_filled;

	for(i = 0; i<=buffer_filled; ++i)
 		buffer[i] =buffer[i+1];

    printf("\t after:");
    for(i = 0; i <= buffer_filled; ++i)
      printf("%c", buffer[i]);
    printf("\n");
    --toreceive;
detach(buffer) ;



}

void pushLetter(char letter)
{
  buffer = assign(shmid);
    printf("letter pushed\t before: ");
    int i;
    for(i = 0; i < buffer_filled; ++i)
      printf("%c", buffer[i]);
    buffer[buffer_filled] = letter;
    ++buffer_filled;
	if(buffer_filled>3)
		up(TOREMOVE);

    printf("\t after:");
    for(i = 0; i<buffer_filled; ++i)
      printf("%c", buffer[i]);
    printf("\n");


    		--toproduce;

  detach(buffer);

}
int down(int sem_num)
{
  	sem.sem_num = sem_num;
  	sem.sem_op = -1;
  	sem.sem_flg = 0;
  	return semop(semid, &sem, 1);
}
int up(int sem_num)
{
  	sem.sem_num = sem_num;
  	sem.sem_op = 1;
  	sem.sem_flg = 0;
  	return semop(semid, &sem, 1);
}
int mut_down(int sem)
{
  	return down(sem);
}
int mut_up(int sem)
{
  	int temp = up(sem);
  	semctl(semid, sem, SETVAL, 1);
  	return temp;
}

char* assign(int shmid)
{
	return (char*)shmat(shmid, NULL, 0);
}

void detach(char* buffer)
{
	shmdt(buffer);
}

