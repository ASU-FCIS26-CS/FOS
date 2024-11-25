#include <inc/lib.h>

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================

static uint32 page_allocation_status[(USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE] = {0}; // 0 = free, 1 = allocated

void* malloc(uint32 size)
{
    //==============================================================
    //DON'T CHANGE THIS CODE========================================
    if (size == 0) return NULL;
    //==============================================================
    //[PROJECT'24.MS2] [2] USER HEAP - malloc() [User Side]

    if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
    {
		// cprintf("entered dynamic allocation\n");
        if (sys_isUHeapPlacementStrategyFIRSTFIT())
            return (void*) alloc_block_FF(size);
        // else
        //     return alloc_block_BF(size);
    }
    else if (size > DYN_ALLOC_MAX_BLOCK_SIZE)
    {


        uint32 num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; // Round up to nearest page
        // Page Allocator for larger allocations

        uint32 required_size = num_pages * PAGE_SIZE;         // Total required size in bytes
        // First-Fit Strategy
		for (uint32 addr = myEnv->rlimit + PAGE_SIZE; addr + required_size <= USER_HEAP_MAX - PAGE_SIZE; addr += PAGE_SIZE)
        {
            uint32 index = (addr - USER_HEAP_START) / PAGE_SIZE;
            uint8 is_free = 1;


            // // Check if all pages in the range are free
            for (uint32 i = 0; i < num_pages; i++)
            {
                if (page_allocation_status[index + i] != 0)
                {
                    is_free = 0;
                    break;
                }
            }
			// cprintf("addr: %p , pagestatus[idx] %d\n", addr, page_allocation_status[index]);
			// if(page_allocation_status[index ] == 0 ) return (void*)addr;
            if (is_free)
            {
                // Mark the pages as allocated
                for (uint32 i = 0; i < num_pages; i++)
                {
                    page_allocation_status[index + i] = index;
                }

                sys_allocate_user_mem(addr, required_size);

                // Return the starting address of the allocated space
                return (void*)addr;
            }
        }
    }

    // If allocation fails, return NULL
    return NULL;
}
// void* malloc(uint32 size)
// {
// 	//==============================================================
// 	//DON'T CHANGE THIS CODE========================================
// 	if (size == 0) return NULL ;
// 	//==============================================================
// 	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
// 	// Write your code here, remove the panic and write your code
// 	panic("malloc() is not implemented yet...!!");
// 	return NULL;
// 	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
// 	//to check the current strategy

// }

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");

	uint32 va = (uint32)virtual_address;

	// Case 1: Check if the address is within the [BLOCK ALLOCATOR] range
	if ( va >= USER_HEAP_START && va <  myEnv->rlimit )
	{
		// Use the dynamic allocator to free the block
		free_block(virtual_address);
		return;
	}

	if (va >= myEnv->rlimit + PAGE_SIZE && va < USER_HEAP_MAX)
	{
		// Get the size of the allocation (number of pages)
		// uint32 page_index = (va - myEnv->rlimit) / PAGE_SIZE;
		uint32 page_index = (va - USER_HEAP_START) / PAGE_SIZE;


        // Find how many pages were allocated for the given virtual address
        uint32 num_pages = 0;
        while (page_allocation_status[page_index + num_pages] == page_index) {
            num_pages++;
        }

        // If the pages are allocated, free the pages
        if (num_pages > 0) {
            // Mark the pages as free
            for (uint32 i = 0; i < num_pages; i++) {
                page_allocation_status[page_index + i] = 0;
            }

            // Call the system function to free the user memory and page file
            sys_free_user_mem(va, num_pages * PAGE_SIZE);
        }
    }
	else {
        panic("Invalid address: Address is not allocated.");
    }

}
//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()
	// Write your code here, remove the panic and write your code
	// panic("smalloc() is not implemented yet...!!");
      uint32 num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; // Round up to nearest page
        // Page Allocator for larger allocations

        uint32 required_size = num_pages * PAGE_SIZE;         // Total required size in bytes

        // First-Fit Strategy
		for (uint32 addr = myEnv->rlimit+PAGE_SIZE; addr + required_size <= USER_HEAP_MAX - PAGE_SIZE; addr += PAGE_SIZE)
        {
            uint32 index = (addr - USER_HEAP_START) / PAGE_SIZE;
            uint8 is_free = 1;


            // // Check if all pages in the range are free
            for (uint32 i = 0; i < num_pages; i++)
            {
                if (page_allocation_status[index + i] != 0)
                {
                    is_free = 0;
                    break;
                }
            }
			// cprintf("addr: %p , pagestatus[idx] %d\n", addr, page_allocation_status[index]);
			// if(page_allocation_status[index ] == 0 ) return (void*)addr;
            if (is_free)
            {
                // Mark the pages as allocated
                for (uint32 i = 0; i < num_pages; i++)
                {
                    page_allocation_status[index + i] = index;
                }

                uint32 sharedObjId = sys_createSharedObject(sharedVarName,required_size,isWritable,(void*)addr);
				if(sharedObjId == E_SHARED_MEM_EXISTS){
					cprintf("exists\n");
					return NULL;
				}

                return (void*)addr;
            }
        }
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
	// Write your code here, remove the panic and write your code
	// panic("sget() is not implemented yet...!!");
	//return NULL;



}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
