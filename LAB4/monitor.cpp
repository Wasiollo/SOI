#include "monitor.h"
#include <stdio.h>
#include <iostream>

#define CONSUMER_A 0
#define CONSUMER_B 1
#define SIZE 15

struct thread_data
{
	int number;
	char letter;
};

class PCMonitor : Monitor
{
	private:
		char buffer[SIZE];		//buffer
		int counter;				// counter - elements in buffer
		int input;					// input - where to write
		int output; 				// output - from where to read
		//aReaded has already read
		int aReaded;
		//bReaded has already read
		int bReaded;
		// Prod - buffer is full
		// ConsumerA - A has already consumed
		// ConsumerB - B has already consumed
		Condition Prod;
		Condition ConsumerA;
		Condition ConsumerB;
		int waitA;
		int waitB;
		void showbuffer();
	public:
		PCMonitor();
		void put(char value);
		char get(int c_id);
};

PCMonitor::PCMonitor()
{
	input=output=counter=0;
	aReaded=bReaded=0;
	waitA=waitB=0;
}

void PCMonitor::showbuffer()
{
	if(input==output){
		printf("%c",buffer[output]);
	for(int i = ((output+1)%SIZE) ; i!=input ; i=((i+1)%SIZE))
		printf("%c",buffer[i]);
	}
	for (int i=output ; i!=input ; i=((i+1)%SIZE))
		printf("%c",buffer[i]);
	printf("\t");
}

void PCMonitor::put(char value)
{
	enter();
	printf("Producer %c want to add\n",value);
	
	printf("COUNTER = %d\n",counter);
	if(counter>=SIZE-1 && value == 'B')
	{
		wait(Prod);
		wait(Prod);
	}
	else if(counter>=SIZE-1){
		wait(Prod);
	}
	printf("Producer %c is adding\n",value);
	printf("buffer: ");
	showbuffer();
	if(value=='B')
	{
			buffer[input] = value;
			input = (input+1)%SIZE;
			++counter ;
			buffer[input] = value;
			
	}
	else
		buffer[input]=value;
	input=(input+1)%SIZE;
	++counter;

	printf("->\t");
	showbuffer();
	printf("\n");

	if(counter>3)
	{
		if(aReaded)
			signal(ConsumerB);
		else if(bReaded)
			signal(ConsumerA);
		else
			if(waitA)
				signal(ConsumerA);
			else if(waitB)
				signal(ConsumerB);
	}
	else
		signal(Prod);
	leave();
}

char PCMonitor::get(int c_id)
{
	enter();
	int deleted=0;
	char value;
// if A has already read and he wants to read - A waits
	if((aReaded==1 || counter<=3) && c_id==CONSUMER_A)
	{
		waitA++;
		wait(ConsumerA);
	}
// if B has already read and he wants to read - B waits
	if((bReaded==1 || counter<=3) && c_id==CONSUMER_B)
	{
		waitB++;
		wait(ConsumerB);
	}

	value=buffer[output];

	if(c_id==CONSUMER_A)
	{
		printf("reading A: %c\n",value);
		aReaded=1;
		if(waitA>0)
			waitA--;
	}
	else
	{
		printf("reading B: %c\n",value);
		bReaded=1;
		if(waitB>0)
			waitB--;
	}

	if(bReaded==1 && aReaded==1)
	{
		aReaded=bReaded=0;
		printf("I deleted from bufor letter %c\n",value);
		printf("buffer: ");
		showbuffer();

		output=(output+1)%SIZE;
		counter--;
		deleted=1;

		printf("->\t");
		showbuffer();
		printf("\n");


	}

	if(deleted)
		signal(Prod);

	if(c_id==CONSUMER_A)
	{
		signal(ConsumerB);
	}
	else
	{
		signal(ConsumerA);
	}

	leave();

	return value;
}

PCMonitor mon;

void * producer(void *threadarg)
{
	struct thread_data *producer_data ;

	producer_data = (struct thread_data *) threadarg;

	int num = producer_data->number;
	for (int i = 0 ; i < num ; i++)
	{
		char x=producer_data->letter;
		mon.put(x);
		usleep(rand()%10000);
	}
}

void * consumer(void *threadarg)
{
	struct thread_data *producer_data ;

	producer_data = (struct thread_data *) threadarg;

	int num = producer_data->number;

	char n = producer_data->letter ;

	for (int i = 0 ; i < num ; i++)
	{
		char x;
		if(n=='A')
			x=mon.get(0);
		else
			x=mon.get(1);
		usleep(rand()%10000);
	}
}



int main(int argc, char* argv[])
{
	pthread_t prodA, consA, prodB,consB;
	
	struct thread_data td[4];

	int toproduce_A, toproduce_B;
	
	if(argc==3)
	{
		toproduce_A = atoi(argv[1]);
		toproduce_B = atoi(argv[2]);
	}
	else{
		printf("How many times you want to run producer A?\n");
		std::cin >> toproduce_A;
		printf("How many times you want to run producer B?\n");
		std::cin >> toproduce_B;
	}
	if(toproduce_A <=0 && toproduce_B <=0)
	{
		std::cout<<"ERROR BAD ARGUMENT"<<std::endl;
		return 1;
	}

	char a='A';
	char b='B';

	int toreceive = toproduce_A+(toproduce_B*2)-3;


	td[0].number = toproduce_A;
	td[1].number = toreceive;
	td[2].number = toproduce_B;
	td[3].number = toreceive;
	td[0].letter = a;
	td[1].letter = a;
	td[2].letter = b;
	td[3].letter = b;

	pthread_create(&prodA,NULL,producer, (void *)&td[0]);
	pthread_create(&consA,NULL,consumer, (void *)&td[1]);
	pthread_create(&prodB,NULL,producer, (void *)&td[2]);
	pthread_create(&consB,NULL,consumer, (void *)&td[3]);


	pthread_join(prodA,NULL);
	pthread_join(consA,NULL);
	pthread_join(prodB,NULL);
	pthread_join(consB,NULL);

	return 0;
}
