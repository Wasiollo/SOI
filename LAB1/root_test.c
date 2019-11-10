#include </usr/include/lib.h>
#include </usr/include/stdio.h>

int getprocnr (int id_proc)
{
	message m;
	m.m1_i1=id_proc;
	return (_syscall(MM, 78, &m)
}

int main (int argc, char* argv[])
{
	int result;
	int i;

	if (argc == 1)
		printf ("No Argument!\n");
	else
	{
		int number, top;
		int begin = atoi (argv[1]);
		if (argc == 2) 
			number = 10;
		else
			number = atoi (argv[2]);
		top = begin + number ;
		for (i=begin ; i<top; i++)
		{	
			result = getprocnr (i) ;
			if (result !=-1)
				printf ("Index of process with pid: %d is %d\n",i,result);
			else
			{
				printf("No process with pid: %d in table! ",i);
				printf("Error : %s\n", strerror(errno));
			}
		}
	}
	return 0 ;
}
