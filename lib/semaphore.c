// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{

	// TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	// COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("create_semaphore is not implemented yet");
	// Your Code is Here...
	struct semaphore sem;
	sem.semdata = NULL;
	
	sem.semdata = smalloc(semaphoreName, sizeof(struct __semdata), 1);

	sem.semdata->count = value;
	sys_init_queue(&sem.semdata->queue);
	strcpy(sem.semdata->name, semaphoreName); // Set the semaphore name
	cprintf("create_semaphore called -> %s\n", sem.semdata->name);
	sem.semdata->lock = 0;

	return sem;
}

struct semaphore get_semaphore(int32 ownerEnvID, char *semaphoreName)
{
	//pushcli();
	//cli();
	struct semaphore sem;
	sem.semdata = NULL;

	//sys_acquire_spin_lock();
	sem.semdata = sget(ownerEnvID, semaphoreName);
	//sys_release_spin_lock();

	if (sem.semdata == NULL)
	{
		cprintf("Error: Semaphore not found for ownerEnvID %d, semaphoreName %s\n", ownerEnvID, semaphoreName);
	
		return sem;
	}

	cprintf("Semaphore found: %s \n", sem.semdata->name);

	// Release lock
	sem.semdata->lock = 0;
	//popcli();
	//sti();
	return sem;
	//return sys_get_semaphore(ownerEnvID, semaphoreName);
}

void wait_semaphore(struct semaphore sem)
{

	//cprintf("wait_semaphore called \n");
	// Use a spinlock to ensure atomic access to the semaphore data
	//     uint32 keyw = 1;
	// do { xchg((volatile uint32*)&sem.semdata->lock, keyw); } while (keyw != 0);
	//   struct spinlock semaphore_lock;

	//sys_acquire_spin_lock();
	// sys_acquire_spin_lock(&semaphore_lock);
	//pushcli();
	//cli();
	while (xchg(&(sem.semdata->lock), 1) != 0);
	//     ;

	// Decrement the semaphore count
	sem.semdata->count--;

	if (sem.semdata->count < 0)
	{
		// Add the current process to the semaphore queue
		struct Env *cur_env = (struct Env *)myEnv;

		// LIST_INSERT_HEAD(&(sem.semdata->queue), cur_env); // Add to the queue
		sys_enqeue(&(sem.semdata->queue), cur_env); // Pass a pointer to the queue
		// Block the current process
		cur_env->env_status = ENV_BLOCKED; // Mark the process as blocked
	}
	sem.semdata->lock = 0; // Release the lock
	//popcli();
	//sti();
	//xchg(&(sem.semdata->lock), 0);
}

void signal_semaphore(struct semaphore sem)
{
	//pushcli();
	//cli();
	while (xchg(&(sem.semdata->lock), 1) != 0);
	// sys_acquire_spin_lock();

	// Increment the semaphore count
	sem.semdata->count++;

	
	if (sem.semdata->count <= 0)
	{
		// Remove a process from the semaphore's queue
		struct Env *waiting_env = sys_deqeue(&(sem.semdata->queue));

		// Check if a process was retrieved from the queue
		if (waiting_env != NULL)
		{
			// Change the process status to ready
			waiting_env->env_status = ENV_READY;
			sys_sched_insert_ready(waiting_env);
		}
	}

	// Release the spinlock
	sem.semdata->lock = 0;
	//popcli();
	//sti();
	//sys_release_spin_lock();
	
}



int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}