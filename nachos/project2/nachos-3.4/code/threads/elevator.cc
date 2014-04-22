
#include "copyright.h"
#include "system.h"
#include "synch.h"


#define MAX_PERSON_NUM 1024
#define MAX_LOOP  10000000
typedef struct _Person{
    int atFloor;
    int toFloor;
    char name[100];
}Person;
Person persons[MAX_PERSON_NUM];

Condition * inList[1024];
Condition * outList[1024];
int WaitToGetInAtFloor[1024];
int WaitToGetOutAtFloor[1024];
int personNum = 0;
bool stop = false;
Condition *personOut;
Condition *personIn;
Condition *capcityCon;
Condition *start;
Lock *myLock;
void Elevator(int numFloors)
{
    DEBUG('t', "Entering Elevator Thread");
    int i,k;
    int capcity = 5;
    for(i=1;i<=numFloors;i++){
        inList[i] = new Condition("Wait to get in the elevator");
        outList[i] = new Condition("Wait to get out of the elevator");
    }
    capcityCon = new Condition("capcity");
    personIn = new Condition("persong in");
    personOut = new Condition("person out");    
   
    myLock->Acquire();
    start->Broadcast(myLock);
    //start->Signal(myLock);
    myLock->Release();
    while(!stop){
         //upstair
        for(i=1;i<=numFloors;i++){
           myLock->Acquire();
           start->Broadcast(myLock);
           printf("Elevator arrives on floor %d.\n",i);
           while(WaitToGetOutAtFloor[i]>0){
               outList[i]->Signal(myLock);
               personOut->Wait(myLock);
               capcity ++;
           }
           myLock->Release();
           
           for(k=0;k<=MAX_LOOP;k++);
           
           myLock->Acquire();
           while(WaitToGetInAtFloor[i]>0 && capcity>0){
               inList[i]->Signal(myLock);
               personIn->Wait(myLock);
               capcity --;
           }
           if(personNum <=0) stop = true;
           else stop = false;
           
           myLock->Release();
        }
       //downstair
        for(i=numFloors-1;i>=2;i--){
           myLock->Acquire();
           printf("Elevator arrives on floor %d.\n",i);
           while(WaitToGetOutAtFloor[i]>0){
               outList[i]->Signal(myLock);
               personOut->Wait(myLock);
               capcity ++;
           }
          
           if(personNum <=0) stop = true;
           else stop = false;

           myLock->Release();

           for(k=0;k<=MAX_LOOP;k++);
           
           myLock->Acquire();
           while(WaitToGetInAtFloor[i]>0 && capcity>0){
               inList[i]->Signal(myLock);
               personIn->Wait(myLock);
               capcity --;
           }
           myLock->Release();
        }
    }
}
void personThread(int which)
{
    //printf("\nI am %d\n",which);
    myLock->Acquire();

    start->Wait(myLock);
    myLock->Release();

    
    int atFloor,toFloor;
    atFloor = persons[which].atFloor;
    toFloor = persons[which].toFloor;
    myLock->Acquire();
    WaitToGetInAtFloor[atFloor]++;
    //while(arriveAt == atFloor)
    inList[atFloor]->Wait(myLock);
    printf("Person %d get into the elevator.\n", which);
    WaitToGetInAtFloor[atFloor]--;
    WaitToGetOutAtFloor[toFloor]++;
    personIn->Signal(myLock);
    myLock->Release();

   // printf("%d:It is my turn ! at:%d to:%d waiting:%d\n",which,atFloor,toFloor,WaitToGetInAtFloor[atFloor]);
    
    myLock->Acquire();
    //while(arriveAt == toFloor)
        outList[toFloor]->Wait(myLock);
    printf("Person %d get out of the elevator.\n", which);
    WaitToGetOutAtFloor[toFloor]--;
    personOut->Signal(myLock);
    personNum --;
    myLock->Release();
}
int personID = 0;
void ArrivingGoingFromTo(int atFloor, int toFloor)
{
    if(start == NULL)   start = new Condition("start");
    if(myLock == NULL){
        myLock = new Lock("myLockXXX");
    }
    personNum ++;
    persons[personID].atFloor = atFloor;
    persons[personID].toFloor = toFloor;
    printf("Person %d wants to go to floor %d from floor %d.\n",personID,toFloor,atFloor);
    sprintf(persons[personID].name,"persong thread:%d",personID);
    Thread *t = new Thread(persons[personID].name);
    t->Fork(personThread , personID);
    personID++;
}

