#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	start = daStart;

	brk = daStart + initSizeToAllocate;
	brk = ROUNDUP(brk, PAGE_SIZE);

	rlimit = daLimit;

	if (brk > rlimit)
	{
		panic("NO MEM IN KHEAP INITIALIZER!!\n");
		return E_NO_MEM;
	}

	uint32 moving_address = start;
	while (moving_address < brk)
	{
		struct FrameInfo *frame;
		allocate_frame(&frame);
		map_frame(ptr_page_directory, frame, moving_address, PERM_WRITEABLE);
		moving_address += PAGE_SIZE;
	}

	initialize_dynamic_allocator(daStart, initSizeToAllocate);

	return 0;
}

void *sbrk(int numOfPages)
{

	// if inc equal 0 then return the curr brk
	if (numOfPages == 0)
	{
		return (void *)brk;
	}

	else if (numOfPages > 0)
	{

		/*
			b7wl al nwbrk le char 34an am4y byte byte 34an ama kont m5liha int mknt4 btzbot
		*/
		uint32 oldBrk = brk;
		char* newBrk = (char*)brk ;

		// bzwd 4096 fe 3dd al pages da al bt7rko wa ba round le 22rb page
		newBrk+=(numOfPages*PAGE_SIZE) ;
		newBrk = ROUNDUP(newBrk, PAGE_SIZE);
		if ((int32)newBrk > rlimit)
		{
			return (void *)-1;
		}

		uint32 currAdress = brk;
		while (currAdress < (int32)newBrk )
		{
			// bgib frame mn al memory a3mlo allocate wa a mapo ll app wa azwd al itterator
			struct FrameInfo *frame;
			allocate_frame(&frame);
			map_frame(ptr_page_directory, frame, currAdress, PERM_WRITEABLE);
			currAdress += PAGE_SIZE;
		}

		// al brk dlw2ty b2a hna wa brg3 al old
		brk = (int32)newBrk;
		return (void *)oldBrk;
	}

	else if (numOfPages < 0)
	{
		//! -----------------warrning mlo4 test-------------------
		// b3ml nfs al fo2 bs bl3ks
		char* new_brk = (char*)brk ;
		new_brk+=(numOfPages*PAGE_SIZE) ;

		if ((int)new_brk < start)
		{
			new_brk = (char*)start;
		}

		uint32 currAdress =(int) new_brk;
		while (currAdress < brk)
		{
			// todo at2kd ml 7eta dy
			uint32 *page_table;
        	struct FrameInfo *frame = get_frame_info(ptr_page_directory, currAdress, &page_table);
			free_frame(frame);
			unmap_frame(ptr_page_directory, currAdress);
			currAdress += PAGE_SIZE;
		}

		brk = (int)new_brk;
		return (void *)brk;
	}

	return NULL;
}
//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void *kmalloc(unsigned int size)
{
    if (size == 0)
    {
        return NULL;
    }
    if (size <= DYN_ALLOC_MAX_BLOCK_SIZE )
    {
		if(isKHeapPlacementStrategyFIRSTFIT())
			return alloc_block_FF(size);

		return alloc_block_BF(size);
    }

    // bounds setup
    uint32 needed_pages = size / PAGE_SIZE;
    needed_pages += (size % PAGE_SIZE != 0) ? 1 : 0;

    uint32 bottom_bound = rlimit + PAGE_SIZE;
    uint32 top_bound = KERNEL_HEAP_MAX;

    uint32 free_pages_found = 0;
    uint32 loop_address = bottom_bound;
    // loop allocating frames
    while (loop_address < top_bound)
    {
        uint32 *page_table;
        struct FrameInfo *frame = get_frame_info(ptr_page_directory, loop_address, &page_table);
        if (frame == NULL)
        {
            free_pages_found++;
            if (free_pages_found == needed_pages)
            {
                uint32 ret_address = loop_address - PAGE_SIZE * (needed_pages - 1);
                for (uint32 i = 0; i < needed_pages; i++)
                {
                    struct FrameInfo *new_frame;
                    allocate_frame(&new_frame);
                    map_frame(ptr_page_directory, new_frame, ret_address + (i * PAGE_SIZE), PERM_WRITEABLE);
                }
                return (void *)ret_address;
            }
        }
        else
        {
            free_pages_found = 0;
        }
        loop_address += PAGE_SIZE;
    }
    return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details
	// Convert the virtual address to uint32 for easier range checks
	uint32 va = (uint32)virtual_address;

	// Case 1: Check if the address is within the [BLOCK ALLOCATOR] range
	if (va >= KERNEL_HEAP_START && va < rlimit)
	{
		// Use the dynamic allocator to free the block
		free_block(virtual_address);
		return;
	}

	// Case 2: Check if the address is within the [PAGE ALLOCATOR] range
	if (va >= rlimit+PAGE_SIZE && va < KERNEL_HEAP_MAX)
	{
		// Get the size of the allocation (number of pages) using dynamic allocator metadata
		uint32 size = *((uint32 *)virtual_address); // This function should return the allocated size in bytes
		if (size == 0)
		{
			panic("kfree() called on unallocated or invalid memory in PAGE ALLOCATOR range!");
			return;
		}
		// Calculate the number of pages
		uint32 num_pages = size / PAGE_SIZE + (size % PAGE_SIZE != 0 ? 1 : 0);
		// Free each page in the range
		uint32 curr_address = va;
		for (uint32 i = 0; i < num_pages; i++)
		{
			struct FrameInfo* frame = get_frame_info(ptr_page_directory, curr_address, NULL);
			if (frame != NULL)
			{
				free_frame(frame);              // Free the frame
				unmap_frame(ptr_page_directory, curr_address); // Unmap the frame
			}
			curr_address += PAGE_SIZE;          // Move to the next page
		}

		return;
	}
	// Case 3: Invalid virtual address
	panic("kfree() called with an invalid virtual address!");

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}
//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}
