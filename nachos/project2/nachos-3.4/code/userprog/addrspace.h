// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
//#include "synch.h"
#define UserStackSize		1024 	// increase this as necessary!
class Thread;
class Semaphore;

typedef int SpaceId;
struct FileEntry {
   OpenFile * of;
};


class ProcessControlBlock {
    public:
        ProcessControlBlock(int fileEntries) {
            //initialize open files table
            openFilesTable = new FileEntry[fileEntries];
            for (int i=0; i<fileEntries; i++) {
                openFilesTable[i].of=NULL;
            }
            size = fileEntries;
        }
    
        void setParent(Thread * myParent) { parent = myParent ; }
        Thread * getParent() { return parent; }
        
        void setPID(SpaceId myPID) { pid =  myPID ; }
        SpaceId getPID() { return pid; }
        
        //inserts a file entry to our files table
        //returns file descriptor
        int insertFile(OpenFile * of) {
            int i=0;
            while (i<size) {
                if (openFilesTable[i].of == NULL) {
                    openFilesTable[i].of = of;
                    return i+2;  //remember that file descriptors 0,1 are reserved for console
                }
                i++;
            }
            return -1;
        }
        
        OpenFile * returnFile(int fd) {  //checks if given file descriptor exists
            DEBUG('x', "Searching file table for file with fd %d\n", fd);
            if (openFilesTable[fd-2].of!=NULL){  //not-empty
                //DEBUG('x', "Entry (%p)\n", openFilesTable[fd-2].of);
                return openFilesTable[fd-2].of;
            }
            else 
                return NULL;
        }
        
        //removes given file descriptor;
        //returns true if action is successful. otherwise returns false
        bool removeFile(int fd) {
            if (openFilesTable[fd-2].of!=NULL){
                openFilesTable[fd-2].of= NULL;
                return true;                //successful removal
            }
            else 
                return false;               //failure 
        }
        
        void cleanupFileEntries() {
            for (int fd=2;fd<size+2;fd++){
                if (returnFile(fd) != NULL) {
                    removeFile(fd);
                }
            }
        }
        
        Semaphore * parentBlockSem;
        int returnExit;
        
        void setThreadPointer(Thread * t) { myThreadPointer = t; }
        Thread * getThreadPointer() { return myThreadPointer; }
        
        void setJoinedID(Thread * myJoinedID) { joinedID = myJoinedID ; }
        Thread * getJoinedID() { return joinedID; }
        
        void setParentPID(SpaceId myParentPID) { parentPid =  myParentPID ; }
        SpaceId getParentPID() { return parentPid; }
        
    private:
        FileEntry *  openFilesTable ;  //table with file entries
        int size;
        SpaceId pid;
        SpaceId parentPid;
        Thread * myThreadPointer;  
        Thread * parent;  //needed to
        Thread * joinedID; 
};



class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    AddrSpace();	// Create an address space,
    AddrSpace *Clone();
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 
    int firstPhysicalPage; 
    ProcessControlBlock * getPCB() { return pcb; }
    //void freeFrames();
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
    void LoadSegment(OpenFile *executable,int addr,int size, int inFileAddr);
    ExceptionType Translate(int virtAddr, int * phyAddr, int size);
            
  private:
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
        ProcessControlBlock * pcb;
};

#endif // ADDRSPACE_H
