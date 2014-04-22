#include "synchconsole.h"


//The below two functions need to be C routines, because
//      C++ can't handle pointers to member functions.
//----------------------------------------------------------------------
static void SynchConsoleReadPoll(int c) { 
    SynchConsole *sconsole = (SynchConsole *)c; sconsole->SynchCheckCharAvail(); 
}
static void SynchConsoleWriteDone(int c) { 
    SynchConsole *sconsole = (SynchConsole *)c; sconsole->SynchWriteDone(); 
}

// SynchConsole Constructor
// Semaphore initialized to zero;
SynchConsole::SynchConsole(char *readFile, char *writeFile) {
    semIn = new Semaphore("console input semaphore", 0);
    semOut = new Semaphore("console output semaphore", 0);
    lockIn = new Lock("console input lock");
    lockOut = new Lock("console output lock");
    console = new Console(readFile, writeFile, (VoidFunctionPtr) SynchConsoleReadPoll,
            (VoidFunctionPtr) SynchConsoleWriteDone, (int) this);
}

// SynchConsole destructor
SynchConsole::~SynchConsole() {
    DEBUG('x',"SynchConsole Destructor\n");
    delete lockIn;
    delete lockOut;
    delete semIn;
    delete semOut;
    delete console;
}

//SynchConsole::PutChar
//We first acquire the lock . Only when we finish do we release it.
//If another process had the lock than we would be required to sleep while waiting.
//Once we have the lock we write the character to the console and then execute
// P()  on semOut.
void SynchConsole::PutChar(char c) {
    DEBUG('x',"SynchConsole PutChar  %c\n",c);
    lockOut->Acquire();
    
    console->PutChar(c);
    semOut->P();
    
    lockOut->Release();
}

//SynchConsole::GetChar
//We first acquire the lock . Only when we finish do we release it.
//If another process had the lock than we would be required to sleep while waiting.
//Once we have the lock we first execute P() on semIn (we make sure that there is 
//a character to get) and then read the character and return.

char SynchConsole::GetChar() {
    char c;
    lockIn->Acquire();
    
    semIn->P();
    c = console->GetChar();
    DEBUG('x',"SynchConsole GetChar  %c\n",c);
    lockIn->Release();
    return c;
}

//These two functions signal asynchronously a successful completion .
void SynchConsole::SynchWriteDone(){
    DEBUG('x',"SynchConsole SynchWriteDone \n");
    semOut->V();
}

void SynchConsole::SynchCheckCharAvail() {
    DEBUG('x',"SynchConsole SynchCheckCharAvail \n");
    semIn->V();
}


