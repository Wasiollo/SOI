#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <lib.h>
#include <minix/type.h>
int main(int argc, char* argv[]) {
                  message mes,m,prio;
  	pid_t  pid = getpid();
  	int i, j, x, l, p;

  		  mes.m1_i1 =  4;
                  l = mes.m1_i2=  (argc >= 2) ? atoi(argv[1]) : 0;
                  mes.m1_i3=  pid;
                  _syscall(MM, 79, &mes);
		
		  m.m1_i1 =  1;
                  p = m.m1_i2 = (argc == 3) ? atoi(argv[2]) : 20;
                  m.m1_i3 =  pid;
		  _syscall(MM, 79, &m);
		
		  for(j=1; j<4 ; ++j)
		  {
                      for(i=1; i<65536*16; ++i)
			{ }
		  printf("Loop with priority %d of GROUP %d ended \n", p,l); 
		  }
                  return 0;
                 
      }
