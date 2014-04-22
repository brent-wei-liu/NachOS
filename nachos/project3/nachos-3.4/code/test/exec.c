#include "syscall.h"

int
main()
{
    int i,pid;
    pid = Exec("./test/execchild",sizeof("./test/execchild"));
    for(;;){
        Print("I am the Parent!\n");
    }
}
