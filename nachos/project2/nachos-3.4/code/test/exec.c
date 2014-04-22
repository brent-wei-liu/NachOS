#include "syscall.h"

void usememory(){
    Exec("./test/memory",sizeof("./test/memory"));
    Exit(0);
}

int main()
{
    int i=0;

    for (i = 0; i < 5; i++) {
         Fork(usememory);
        //Exec("./test/memory",sizeof("./test/memory"));
        Yield();
    }
    Exit(0);
}

