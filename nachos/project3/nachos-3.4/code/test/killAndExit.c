
#include "syscall.h"

void func(){
    int i = 0;
    for(;;){
        Print("I'm child\n");
       i = i + 1;
    }
    Exit(0);    
}

int
main()
{
   int i = 0;
   int p1,r1;
    for(i = 0; i < 3 ; i++){
       Print("I'm parent\n");
    }
    p1 = Fork(func);
    r1 = Kill(p1);
    
    for(i = 0; i < 3 ; i++){
       Print("I'm parent second iteration\n");
    }
    
    if(r1 == 1){
       Print("r1 = 1 \n");
    }
    
    Exit(1);
    /* not reached */
}
