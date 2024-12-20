#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");
	//Your Code is Here...

	if(numOfFrames <= 0){
		return NULL;
	}

	//cprintf("numbers of frames:%d\n", numOfFrames);
	//cprintf("size of frames info%d\n", (uint32)sizeof(struct FrameInfo*));
	struct FrameInfo** FramesArr = (struct FrameInfo**)kmalloc(sizeof(char)*numOfFrames);

	// if allocation failed -> return NULL
	if(FramesArr == NULL)
			return NULL;
	// initialize to zeros
	memset(FramesArr, 0, sizeof(char)*numOfFrames);

	return FramesArr;

}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share*  create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_share is not implemented yet");
	//Your Code is Here...

	struct Share* newShare = (struct Share *)kmalloc(sizeof(struct Share));
	if(newShare == NULL){
		return NULL;
	}
	uint32 nameSize = sizeof(newShare->name);
	if (sizeof(shareName) < nameSize)
			nameSize = sizeof(shareName);
	// setup data
	newShare->ID = (int32)newShare & 0x7FFFFFFF; //address of object + masking the MSB (make it +ve)
	newShare->ownerID = ownerID;
	uint32 i;
	for(i = 0; i < nameSize; i++){
		(newShare->name)[i] = shareName[i];
	}
	// pad with null terminator
	for(; i < sizeof(newShare->name); i++)
		(newShare->name)[i] = '\0';
	//
	//strcpy((new_object->name),shareName); // not safe
	newShare->size = size;
	newShare->isWritable = isWritable;
	newShare->references = 1;
	newShare->framesStorage = create_frames_storage(ROUNDUP(size/PAGE_SIZE, PAGE_SIZE));
	// if allocating frames failed -> return null
	if(newShare->framesStorage == NULL){
		kfree(newShare);
		return NULL;
	}

	return newShare;
}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name)
{
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_share is not implemented yet");
	//Your Code is Here...
	acquire_spinlock(&AllShares.shareslock);

	struct Share *desiredObject = NULL;
	struct Share *currentOb = NULL;

	LIST_FOREACH(currentOb, &(AllShares.shares_list)){
		if(strcmp((currentOb->name), name)==0 && (currentOb->ownerID) == ownerID){
			//cprintf("found obj");
			desiredObject = currentOb;
			break;
		}
	}

	release_spinlock(&AllShares.shareslock);

	return desiredObject;
}

//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment

	struct Share *isObjectExist = get_share(ownerID,shareName);
	//cprintf("Owner with ID %d want to create obj with name: %s\n", ownerID, shareName);
	//cprintf("\n Object value: %x\n", isObjectExist);
	if(isObjectExist == NULL)
	{
		uint32 totalSpace = (uint32)virtual_address + size;
		uint32 endOfObject = ROUNDUP(totalSpace,PAGE_SIZE);
		// uint32 endOfObject = (uint32)virtual_address + size;
		struct Share *new_object = create_share(ownerID,shareName,size,isWritable);

		//check if the object created and allocated successfully
		if(new_object != NULL)
		{
			int frameIndex = 0;
			for(uint32 currentV = (uint32)virtual_address; currentV < endOfObject; currentV += PAGE_SIZE)
			{
				struct FrameInfo *objectFrame = NULL;
				int check =  allocate_frame(&(objectFrame));
				if(check == E_NO_MEM)
				{
					//panic("There is no enough memory!");
					// free allocated frames
					uint32 startVa = (uint32)virtual_address;
					while(currentV >= startVa){
						frameIndex--;
						currentV-=PAGE_SIZE;
						new_object->framesStorage[frameIndex] = NULL;
						unmap_frame((myenv->env_page_directory), currentV);
					}

					return E_NO_SHARE;
				}

				//map each VA to frame and add the frame to framesStorage array
				map_frame((myenv->env_page_directory),objectFrame,currentV,PERM_USER | PERM_WRITEABLE);
				new_object->framesStorage[frameIndex] = objectFrame;
				// cprintf("frame at index %d is %p\n",frameIndex,objectFrame);
				frameIndex++;
			}
			//cprintf("virtual_address%p , endOfObject:%p frameIndex:%d\n",virtual_address,endOfObject,frameIndex);

			//add the new object to the shared list
			acquire_spinlock(&(AllShares.shareslock));
			LIST_INSERT_TAIL(&(AllShares.shares_list), new_object);
			release_spinlock(&(AllShares.shareslock));

			return new_object->ID;
		}

		else
		{
			return E_NO_SHARE;
		}
	}

	else
	{
		return E_SHARED_MEM_EXISTS;
	}

}


//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("getSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment

	struct Share *requestedObject = get_share(ownerID,shareName);
	if(requestedObject != NULL)
	{
		uint8 isWrite = 0;
		if(requestedObject->isWritable)
			isWrite = PERM_WRITEABLE;
		int count = 0;
		struct FrameInfo *sharedFrame;
		uint32 VA = (uint32)virtual_address;
		int numOfFrames = sizeof(requestedObject->framesStorage)/sizeof(requestedObject->framesStorage[0]);
		while(count < numOfFrames)
		{
			sharedFrame = requestedObject->framesStorage[count];
			//map each page to each shared frame with the permission isWritable
			map_frame((myenv->env_page_directory), sharedFrame, VA+(count*PAGE_SIZE), PERM_USER | isWrite);
			count++;
		}

		//update the references
		requestedObject->references++;
		return requestedObject->ID;
	}

	//return if the object does not exist
	return E_SHARED_MEM_NOT_EXISTS;

}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
// void free_share(struct Share* ptrShare)
// {
// 	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
// 	//COMMENT THE FOLLOWING LINE BEFORE START CODING
// 	// panic("free_share is not implemented yet");
// 	//Your Code is Here...



void free_share(struct Share *ptrShare)
{
	// Acquire lock to ensure thread safety
	acquire_spinlock(&(AllShares.shareslock));

	// Remove the share from the global list
	struct Share *curr;
	LIST_FOREACH(curr, &(AllShares.shares_list))
	{
		if (curr == ptrShare)
		{
			LIST_REMOVE(&(AllShares.shares_list), curr);
			break;
		}
	}

	release_spinlock(&(AllShares.shareslock));


	

	// Free associated frames
	
		// uint32 numFrames = (ptrShare->size + PAGE_SIZE - 1) / PAGE_SIZE; // Round up
		// uint32 numFrames= ROUNDDOWN(ptrShare->size/PAGE_SIZE,PAGE_SIZE);

		uint32 numFrames = ptrShare->size / PAGE_SIZE;
		for (uint32 frameIndex = 0; frameIndex < numFrames; frameIndex++)
		{
			if (ptrShare->framesStorage[frameIndex] != NULL)
			{
				// cprintf("Freeing frame at index %d: %p\n", frameIndex, ptrShare->framesStorage[frameIndex]);
				free_frame(ptrShare->framesStorage[frameIndex]);
			}
		}

		

		// Free framesStorage array
		cprintf("Free share frames storage :%p\n", ptrShare->framesStorage);
		kfree(ptrShare->framesStorage);

	// Free the Share structure itself
	cprintf("Free share structure :%p\n", ptrShare);
	kfree(ptrShare);
}
//========================
// [B2] Free Share Object:
//========================
// int fre




int freeSharedObject(int32 sharedObjectID, void *startVA)
{
    // Locate the shared object from the global shares list
    struct Share *sharedObj = NULL;
    acquire_spinlock(&(AllShares.shareslock)); // Acquire lock to modify global list
    struct Share *curr;
    LIST_FOREACH(curr, &(AllShares.shares_list))
    {
        if (curr->ID == sharedObjectID)
        {
            sharedObj = curr;
            break;
        }
    }
    release_spinlock(&(AllShares.shareslock)); // Release lock after list manipulation

    if (!sharedObj)
    {
        return E_NO_SHARE; // Return error if no such shared object is found
    }
    cprintf("Freeing shared object %s\n", sharedObj->name);

    struct Env *myenv = get_cpu_proc();
    uint32 endOfObject = ROUNDUP((uint32)startVA + (sharedObj->references * PAGE_SIZE), PAGE_SIZE);

    // Unmap frames from the current process
    for (int i = 0; i < sharedObj->references; i++)
    {
        uint32 va = (uint32)startVA + (i * PAGE_SIZE);
        if ((uint32)sharedObj->framesStorage[i] == va) // Correct comparison
        {
            unmap_frame(myenv->env_page_directory, va); // Unmap frame
            sharedObj->framesStorage[i] = NULL; // Nullify the frame storage
        }
    }

    // // Check for empty page tables and remove them if necessary
    for (uint32 va = (uint32)startVA; va < endOfObject; va += PAGE_SIZE)
    {
        uint32 *ptr_page_table = NULL;
        get_page_table(myenv->env_page_directory, va, &ptr_page_table);

        if (ptr_page_table != NULL)
        {
            int isTableEmpty = 1;
            for (int page = 0; page < 1024; page++)
            {
                if (ptr_page_table[page] != 0)
                {
                    isTableEmpty = 0; // Found an entry in the page table
                    break;
                }
            }

            if (isTableEmpty)
            {
                pd_clear_page_dir_entry(myenv->env_page_directory, va); // Clear page dir entry
                kfree((void *)ptr_page_table); // Free the page table
            }
        }
    }

    // Decrement references and check if the last reference is reached
    if (--sharedObj->references == 0)
    {
        free_share(sharedObj); // Free the share object if no references remain
    }

    // Flush the Translation Lookaside Buffer (TLB) to ensure memory mappings are updated
    tlbflush();

    return 0; // Success
}