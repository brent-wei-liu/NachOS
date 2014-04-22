#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "copyright.h"
#include "utility.h"

#include "synch.h"
#include "console.h"

#ifdef USER_PROGRAM
#include "machine.h"
#include "addrspace.h"
#endif

class SynchConsole {
    public:
        SynchConsole(char *readFile, char *writeFile) ;     // Initialize a synchronous Console,
                                                    // by initializing a plain Console.
        ~SynchConsole();            // De-allocate the synch console
    
    void PutChar(char c);
    char GetChar();
   
    void SynchWriteDone();   
        void SynchCheckCharAvail();

  private:
        Console * console;  
        Semaphore *semIn;       
    Semaphore *semOut;      
    Lock *lockIn;
    Lock *lockOut;  
  
};

#endif //SYNCHCONSOLE_H

