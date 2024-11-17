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

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'24.MS2 - #03] [1] KERNEL HEAP - kmalloc
	// Write your code here, remove the panic and write your code
	kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

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
