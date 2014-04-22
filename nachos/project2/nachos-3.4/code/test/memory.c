#include "syscall.h"
#define MAX 128
int array[MAX];

int main()
{
    int i;
    for (i = 0; i < MAX; i++) {
        array[i] = 42;
    }
    //Print("I am child\n");
    Exit(0);
}

