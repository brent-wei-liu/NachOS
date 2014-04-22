#include "syscall.h"

void func(){
    int i = 0;
    for(i = 0; i < 10; i++){
//       Print("I'm child\n");
       i = i + 1;
    }
    Exit(i);    
}

void func2(){
    int i = 0;
    for(i = 0; i < 30; i++){
//       Print("I'm child 2\n");
       i = i + 1;
    }
    Exit(i);    
}

int
main()
{
   int i = 0;
   int p1, p2;
   int r1, r2;
    for(i = 0; i < 3 ; i++){
       Print("I'm parent\n");
    }
    p1 = Fork(func);
    p2 = Fork(func2);
    if(p1 == 1){
       Print("p1 = 1 \n");
    }
    if(p2 == 2){
       Print("p2 = 2 \n");
    }
   
    r1 = Join(p1);
    r2 = Join(p2);
   
    for(i = 0; i < 3 ; i++){
       Print("I'm parent second iteration\n");
    }
   
    if(r1 == 1){
       Print("r1 = 1 \n");
    }
    if(r2 == 2){
       Print("r2 = 2 \n");
    }
   
    Exit(1);
    /* not reached */
}
