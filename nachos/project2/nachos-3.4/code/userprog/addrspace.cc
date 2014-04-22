// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------
static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

AddrSpace::AddrSpace()
{}

AddrSpace * AddrSpace::Clone()
{
    int size;
    machine->MemoryTieUpRate = bitmap->BitMapTieUpRate();
    DEBUG('t', "Memory Usage Rate = %.2f\%\n",100*machine->MemoryTieUpRate);
    //printf("bitmap->NumClear():%d\n",bitmap->NumClear());
    if(numPages > bitmap->NumClear())
        return NULL;
    
    AddrSpace * space = new AddrSpace();
    size = numPages * PageSize;
    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
                                    numPages, size);
// first, set up the translation
    space->numPages = this->numPages;
    space->pageTable = new TranslationEntry[numPages];
    int base = numPages + 1;
    //set up the translation
    for (int i = 0; i < numPages; i++) {
        space->pageTable[i].virtualPage = i;
        //space->pageTable[i].physicalPage = i + base;
        space->pageTable[i].physicalPage = bitmap->Find();
        space->pageTable[i].valid = TRUE;
        space->pageTable[i].use = FALSE;
        space->pageTable[i].dirty = FALSE;
        space->pageTable[i].readOnly = FALSE;
                        // if the code segment was entirely on
                        // a separate page, we could set its
                        // pages to be read-only
        memcpy( 
            machine->mainMemory + space->pageTable[i].physicalPage * PageSize,
            machine->mainMemory + this->pageTable[i].physicalPage * PageSize, 
            PageSize
            );
        
        if( i == 0 ) 
            space->firstPhysicalPage = space->pageTable[i].physicalPage;
//        printf("PhysicalPage:%d\n",space->pageTable[i].physicalPage);
    }
    
    space->pcb = new ProcessControlBlock(10);
    machine->MemoryTieUpRate = bitmap->BitMapTieUpRate();
    DEBUG('t', "Memory Usage Rate = %.2f\%\n",100*machine->MemoryTieUpRate);
    return space;
}
//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    machine->MemoryTieUpRate = bitmap->BitMapTieUpRate();
    DEBUG('t', "Memory Usage Rate = %.2f\%\n",100*machine->MemoryTieUpRate);
//    printf("numPages:%d  bitmap->NumClear():%d\n",
//            numPages,
//            bitmap->NumClear());
    if(numPages > bitmap->NumClear()){
        numPages = 0;
        return;
    }

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        // for now, virtual page # = phys page #
        pageTable[i].virtualPage = i;	
        pageTable[i].physicalPage = bitmap->Find();
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  
        if( i == 0 )
            this->firstPhysicalPage = pageTable[i].physicalPage;

//        printf("virtualAdd[0x%x]->physicalAdd[0x%x]\n",
//                i*PageSize, pageTable[i].physicalPage*PageSize);
    }
    
// zero out the process address space, to zero the unitialized data segment 
// and the stack segment
    for (i = 0; i < numPages; i++) {
        DEBUG('z', "Zeroing frame number %d starting at 0x%x\n", 
                pageTable[i].physicalPage,
                pageTable[i].physicalPage*PageSize);
        bzero( 
            &(machine->mainMemory[pageTable[i].physicalPage*PageSize]),
            PageSize);
    }


//// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('z', "Initializing code segment, at 0x%x, size %d\n", 
            noffH.code.virtualAddr, noffH.code.size);
        LoadSegment(executable,
                    noffH.code.virtualAddr,
                    noffH.code.size, 
                    noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('z', "Initializing data segment, at 0x%x, size %d\n", 
            noffH.initData.virtualAddr, noffH.initData.size);
        LoadSegment(executable,
                    noffH.initData.virtualAddr,
                    noffH.initData.size,
                    noffH.initData.inFileAddr);
    }

    this->pcb = new ProcessControlBlock(10);

    printf("Loaded Program: [%d] code | [%d] data | [%d] bss\n",
            noffH.code.size,
            noffH.initData.size,
            noffH.uninitData.size);
    
    machine->MemoryTieUpRate = bitmap->BitMapTieUpRate();
    DEBUG('t', "Memory Usage Rate = %.2f\%\n",100*machine->MemoryTieUpRate);

}


void AddrSpace::LoadSegment(OpenFile *executable, int addr, int size, int inFileAddr) {
    
    int physAddr, loops;
    
    //translate for block not starting at the beginning of a page
    if (addr % PageSize) {
        Translate(addr,&physAddr,size % PageSize);
        DEBUG('z',"1.Translation from Virtual Addr 0x%x to Physical Addr = 0x%x for block of size %d\n",addr, physAddr,size % PageSize);
        executable->ReadAt(& (machine->mainMemory[physAddr]), size % PageSize, inFileAddr);
        size -=  (size % PageSize);
        addr += (size % PageSize);
        inFileAddr += (size % PageSize);
    }
    loops = size/PageSize;
    //translate for blocks starting at the beginning of a page and allocating the whole page
    for (int i=0; i<loops ; i++){
        Translate(addr,&physAddr,PageSize);
        DEBUG('z',"2.Translation from Virtual Addr 0x%x to Physical Addr = 0x%x for block of size %d\n",addr, physAddr, PageSize);
        executable->ReadAt(& (machine->mainMemory[physAddr]), PageSize, inFileAddr);
        size -= PageSize;
        addr += PageSize;
        inFileAddr += PageSize;
    }
    //translate for blocks starting at the beginning of a page but allocating only a part of it.
    if (size % PageSize) {
        Translate(addr,&physAddr,size % PageSize);
        DEBUG('z',"3.Translation from Virtual Addr 0x%x to Physical Addr = 0x%x for block of size %d\n",addr, physAddr,size % PageSize);
        executable->ReadAt(& (machine->mainMemory[physAddr]), size % PageSize, inFileAddr);
    }       
}


//taken from translate.cc  (the same function from translate.cc could be used
// but a problem came up when loading initial file with "StartProcess").
// it is either this or setting the pagetable pointer for the machine Object inside AddrSpace constructor
ExceptionType AddrSpace::Translate(int virtAddr, int * physAddr, int size) {
    unsigned int vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;
    
    if(virtAddr < 0) {
        return AddressErrorException;
    }
    vpn = (unsigned) virtAddr / PageSize;
    offset = (unsigned) virtAddr % PageSize;
    if (vpn >= numPages) {
        DEBUG('z', "virtual page # %d too large for page table size %d!\n", virtAddr, numPages);
        return AddressErrorException;
    }
    else if (!pageTable[vpn].valid) {
        DEBUG('z', "virtual page # %d too large for page table size %d!\n", virtAddr, numPages);
        return PageFaultException;
    }
    entry = &pageTable[vpn];
    pageFrame = entry->physicalPage;

    // if the pageFrame is too big, there is something really wrong! 
    // An invalid translation was loaded into the page table or TLB. 
    if (pageFrame >= NumPhysPages) { 
        DEBUG('z', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
        return BusErrorException;
    }
    entry->use = TRUE;      // set the use bits
    
    *physAddr = pageFrame * PageSize + offset;
    ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
    DEBUG('z', "phys addr = 0x%x\n", *physAddr);
    return NoException;
}



//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    int i;
    DEBUG('t',"Finishing AddrSpace!\n");
   for(i=0;i<numPages;i++){
        bitmap->Clear(pageTable[i].physicalPage);
//        DEBUG('z', "\nFrame number (%d) cleared   ", pageTable[i].physicalPage);
    }
    if(numPages >0)
        delete pageTable;

    machine->MemoryTieUpRate = bitmap->BitMapTieUpRate();
    DEBUG('t', "Memory Usage Rate = %.2f\%\n",100*machine->MemoryTieUpRate);

   
}
/*
void AddrSpace::freeFrames() {
    if (pageTable != NULL) {
        for (int i = 0; i < numPages; i++) {
            frameBitmap->Clear( pageTable[i].physicalPage );
        }
    }
}
*/
//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
