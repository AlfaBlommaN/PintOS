#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "userprog/myClib.h"

#include "userprog/gdt.h"      /* SEL_* constants */
#include "userprog/process.h"
#include "userprog/load.h"
#include "userprog/pagedir.h"  /* pagedir_activate etc. */
#include "userprog/tss.h"      /* tss_update */
#include "filesys/file.h"
#include "threads/flags.h"     /* FLAG_* constants */
#include "threads/thread.h"
#include "threads/vaddr.h"     /* PHYS_BASE */
#include "threads/interrupt.h" /* if_ */


/* OUR HEADERS */
#include "userprog/map.h"
#include "userprog/syscall.h"
/* Headers not yet used that you may need for various reasons. */
#include "threads/synch.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"

#include "userprog/flist.h"
#include "userprog/plist.h"
#include "userprog/pmap.h"


#define STACK_DEBUG(...) //printf(__VA_ARGS__);
#define DBG(format, ...) //printf("# DEBUG: " format "\n", ##__VA_ARGS__);
#define DBG_SYS_WAIT(format,...) //printf("# DEBUG: " format "\n", ##__VA_ARGS__)
#define DBG_PRINT_LIST process_print_list();
void* setup_main_stack(const char* command_line, void* stack_top);
struct semaphore main_sema;

struct main_args
{
  /* Variable "ret" that stores address (*ret) to a function*/
  void (*ret)(void);
  int argc;
  char** argv;
};


/* This function is called at boot time (threads/init.c) to initialize
 * the process subsystem. */
void process_init(void)
{
  pmap_constructor(&global_process_table,&main_sema);
  DBG("# PROCESS INIT %d i filen %s", __LINE__, __FILE__);
}

/* This function is currently never called. As thread_exit does not
 * have an exit status parameter, this could be used to handle that
 * instead. Note however that all cleanup after a process must be done
 * in process_cleanup, and that process_cleanup are already called
 * from thread_exit - do not call cleanup twice! */
void process_exit(int status )
{

  if(thread_current() -> pmap_key <= PMAP_SIZE)
    {
      lock_acquire(&(global_process_table.pmap_lock));
      global_process_table.content[thread_current()->pmap_key]->exit_status = status ;  
      lock_release(&(global_process_table.pmap_lock));
    }
  
  thread_exit();
}

/* Print a list of all running processes. The list shall include all
 * relevant debug information in a clean, readable format. */
void process_print_list()
{
  if(global_process_table.content[thread_current()->pmap_key] == NULL)
    {
      return;
    }
  else
    {
      print_pretty(&global_process_table,global_process_table.siz);  
    }
}


struct parameters_to_start_process
{
  char* command_line;
  struct semaphore sema;
  int pid;
  int parent_pid;
};

static void
start_process(struct parameters_to_start_process* parameters) NO_RETURN;

/* Starts a new proccess by creating a new thread to run it. The
   process is loaded from the file specified in the COMMAND_LINE and
   started with the arguments on the COMMAND_LINE. The new thread may
   be scheduled (and may even exit) before process_execute() returns.
   Returns the new process's thread id, or TID_ERROR if the thread
   cannot be created. */
int
process_execute (const char *command_line) 
{
  char debug_name[64];
  int command_line_size = strlen(command_line) + 1;
  tid_t thread_id = -1;
  int  process_id = -1;

  /* LOCAL variable will cease existence when function return! */
  struct parameters_to_start_process arguments;
  arguments.parent_pid = thread_current()-> tid; //Setting this so that the created sub process gets it.
  sema_init(&(arguments.sema),0); 
 
  debug("%s#%d: process_execute(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        command_line);

  /* COPY command line out of parent process memory */
  arguments.command_line = malloc(command_line_size);
  strlcpy(arguments.command_line, command_line, command_line_size);

  strlcpy_first_word (debug_name, command_line, 64);
  
  /* SCHEDULES function `start_process' to run (LATER) */
      thread_id = thread_create (debug_name, PRI_DEFAULT,
				 (thread_func*)start_process, &arguments);
  if( -1 == thread_id)
    {
      free(arguments.command_line);
      arguments.pid = -1; //We have failed to create the thread.
      return process_id;
    }
    
  /*Wait until the process starts in a good way */ 
  sema_down(&(arguments.sema));
  process_id = arguments.pid;
  /* WHICH thread may still be using this right now? */
  free(arguments.command_line);

  debug("%s#%d: process_execute(\"%s\") RETURNS %d\n",
        thread_current()->name,
        thread_current()->tid,
        command_line, process_id); 
  return process_id;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (struct parameters_to_start_process* parameters)
{
  /* The last argument passed to thread_create is received here... */
  /*ESP is contained in this struct, the struct represents the interrupt frame - John */
  struct intr_frame if_; 
  bool success = false;
  struct thread* current_thread = thread_current();
  char file_name[64];
  strlcpy_first_word (file_name, parameters->command_line, 64);

  parameters -> pid = -1;
  
  /* Note that each thread gets assign the parent threads id  -John 2015 26 April*/

  debug("%s#%d: start_process(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);
  
  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  
  success = load (file_name, &if_.eip, &if_.esp);

  debug("%s#%d: start_process(...): load returned %d\n",
        thread_current()->name,
        thread_current()->tid,
        success);
  
  if (success)
    {
      /* We managed to load the new program to a process, and have
	 allocated memory for a process stack. The stack top is in
	 if_.esp, now we must prepare and place the arguments to main on
	 the stack. */
    
      if_.esp = setup_main_stack(parameters -> command_line,if_.esp);
       
      /* The stack and stack pointer should be setup correct just before
	 the process start, so this is the place to dump stack content
	 for debug purposes. Disable the dump when it works. */
      
      /* Calculating which values we will get in the process_info*/
      if(!pmap_is_full(&global_process_table))
	{
	  /*Sets the entry in the table we prepair
	    the updat that is done in update_process_info*/	 
	  int proc_id = thread_current() -> tid;
	  int parent_id = parameters -> parent_pid;
	  int exit_status = 0;
	  bool alive = true;
	  bool parent_alive = true;
	  lock_acquire(&(global_process_table.pmap_lock));
	  current_thread -> pmap_key = pmap_insert(&(global_process_table));	    	 
	  insert_process_info(thread_current()->pmap_key,proc_id,parent_id,
			      exit_status,alive,parent_alive);
	  lock_release(&(global_process_table.pmap_lock));  	 
	  /*Sets the process id */
	  parameters -> pid = thread_current() -> tid;
	  /*The thread now belongs to a process */
	  thread_current() -> process = true;
	}	 
      else
      	{
	  success = false;
      	}
    }

  debug("%s#%d: start_process(\"%s\") DONE\n",
	thread_current()->name,
        thread_current()->tid,
        parameters->command_line);
   
 sema_up(&parameters->sema);  

  /* If load fail, quit. Load may fail for several reasons.
     Some simple examples:
     - File does not exist
     - File do not contain a valid program
     - Not enough memory
  */ 
 if ( ! success )
    {
      thread_exit ();
    }
 
  
  /* Start the user process by simulating a return from an interrupt,
     implemented by intr_exit (in threads/intr-stubs.S). Because
     intr_exit takes all of its arguments on the stack in the form of
     a `struct intr_frame', we just point the stack pointer (%esp) to
     our stack frame and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Wait for process `child_id' to die and then return its exit
   status. If it was terminated by the kernel (i.e. killed due to an
   exception), return -1. If `child_id' is invalid or if it was not a
   child of the calling process, or if process_wait() has already been
   successfully called for the given `child_id', return -1
   immediately, without waiting.

   This function will be implemented last, after a communication
   mechanism between parent and child is established. */
int
process_wait(int child_id) 
{
  struct thread *cur = thread_current();
  int our_id = cur -> tid;
  int status = -1;
  
  debug("%s#%d: process_wait(%d) ENTERED\n",
        cur->name, cur->tid, child_id);

  lock_acquire(&(global_process_table.pmap_lock));
  struct process_info* child = pmap_find_process(&global_process_table,child_id);
  lock_release(&(global_process_table.pmap_lock));
  /*If we got a non valid process we exit with error code -1 */
  if(child == NULL || child->parent_id != our_id)
    {
      DBG_SYS_WAIT("# process wait failed.");
      return -1;
    }
  /*If the child is dead we have nothing to wait for */
  if(!child->alive && our_id == child->parent_id)
    {
      status = child -> exit_status;
      lock_acquire(&(global_process_table.pmap_lock));
      pmap_remove_process(&(global_process_table),child_id);
      lock_release(&(global_process_table.pmap_lock));
      return status;
    }

  /*Wait for the child to finnish */
  sema_down(&child->wait_sema); 
  status = child -> exit_status;

  lock_acquire(&(global_process_table.pmap_lock));     
  pmap_remove_process(&( global_process_table),child_id);
  lock_release(&(global_process_table.pmap_lock));
  debug("%s#%d: process_wait(%d) RETURNS %d\n",
        cur->name, cur->tid, child_id, status);

  
  return status;
}

/* Free the current process's resources. This function is called
   automatically from thread_exit() to make sure cleanup of any
   process resources is always done. That is correct behaviour. But
   know that thread_exit() is called at many places inside the kernel,
   mostly in case of some unrecoverable error in a thread.

   In such case it may happen that some data is not yet available, or
   initialized. You must make sure that nay data needed IS available
   or initialized to something sane, or else that any such situation
   is detected.
*/
  
void
process_cleanup (void)
{
  struct thread  *cur = thread_current();
  uint32_t       *pd  = cur->pagedir;
  int status = -1;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    } 
  if(!cur-> process)
    {
      return;
    }

  if(cur->pmap_key <= PMAP_SIZE)
    {
      DBG("# exekverade rad %d i filen %s with the name of: %s # %d", __LINE__, __FILE__,cur->name, cur->tid); 
      DBG("# PMAP_KEY = %d, the process running is: %s # %d ", thread_current()->pmap_key,cur->name,cur->tid);
      lock_acquire(&(global_process_table.pmap_lock));
      status = global_process_table.content[thread_current()->pmap_key] ->exit_status;  
      lock_release(&(global_process_table.pmap_lock));
    }
  else if(cur->tid == 1)
    {
      //observe special case for main.
      DBG("# exekverad rad %d i filen %s with the name of: %s # %d", __LINE__, __FILE__,cur->name, cur->tid);
      status = 0; 
    }
  else
    {
      status = -1;
    }
  
  debug("%s#%d: process_cleanup() ENTERED\n", cur->name, cur->tid);
  
  /* Later tests DEPEND on this output to work correct. You will have
   * to find the actual exit status in your process list. It is
   * important to do this printf BEFORE you tell the parent process
   * that you exit.  (Since the parent may be the main() function,
   * that may sometimes poweroff as soon as process_wait() returns,
   * possibly before the printf is completed.)
   */
  printf("%s: exit(%d)\n", thread_name(), status);
  
 
  /*Closes all open files*/
  process_close_files(cur);
  DBG("# exekverade rad %d i filen %s with the name of: %s # %d", __LINE__, __FILE__,cur->name, cur->tid);

   /*Only real processes shall be able to run the calls below. */
  if(thread_current()->pmap_key < PMAP_SIZE && thread_current()->tid != 1)
    {
      DBG("# exekverade rad %d i filen %s with the name of: %s # %d", __LINE__, __FILE__,cur->name, cur->tid); 
      DBG("# PMAP_KEY = %d, the process running is: %s # %d", thread_current()->pmap_key,cur->name,cur->tid);

      lock_acquire(&(global_process_table.pmap_lock));
      remove_process_info(status);       
      if(global_process_table.content[cur->pmap_key] != NULL)
	{ 
	  sema_up(&(global_process_table.content[cur->pmap_key]->wait_sema));
	}
      lock_release(&(global_process_table.pmap_lock));
    }
  /* End of our code */
  debug("%s#%d: process_cleanup() DONE with status %d\n",
        cur->name, cur->tid, status);
}
/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}


/*The task of this function is to init the process of closing all
  open files, it will invoke map_for_each with a special process_close()
  function that will make sure that all open files are closed when a program is 
  done,  This is implemented via a function pointer*/
void process_close_files(struct thread* ptr)
{
  ptr = thread_current();
  map_for_each(&(ptr -> open_file_table),process_close);
}

void* setup_main_stack(const char* command_line, void* stack_top)
{
  /* Variable "esp" stores an address, and at the memory loaction
   * pointed out by that address a "struct main_args" is found.
   * That is: "esp" is a pointer to "struct main_args" */
  struct main_args* esp;
  int argc;
  int total_size;
  int line_size;
  /* "cmd_line_on_stack" and "ptr_save" are variables that each store
   * one address, and at that address (the first) char (of a possible
   * sequence) can be found. */
  char* cmd_line_on_stack;
  int i = 0, j = 0;

  /* calculate the bytes needed to store the command_line */
  line_size = strlen(command_line)+1; //count  the last'\0' aswell
  STACK_DEBUG("# line_size = %d\n", line_size);

  /* round up to make it even divisible by 4 */
  line_size = round_up(line_size);
  STACK_DEBUG("# line_size (aligned) = %d\n", line_size);

  /* calculate how many words the command_line contain */
  argc =  count_args(command_line," ");
  STACK_DEBUG("# argc = %d\n", argc);

  /* calculate the size needed on our simulated stack */
  total_size = line_size + (argc+1)*4 + 3*4; 
  STACK_DEBUG("# total_size = %d\n", total_size);
  /* calculate where the final stack top will be located */
  esp = (struct main_args*)(((int)stack_top) - total_size);
  /*******************************************************/
  /* setup return address and argument count */
  esp->ret = 0x00000000; //Written for protection
  esp->argc = argc; 
  /*******************************************************/
  /* calculate where in the memory the argv array starts */
  esp->argv = esp+1; //Note that the size of esp is 12 bytes
  /*******************************************************/
  /* calculate where in the memory the words is stored */
  cmd_line_on_stack = &(esp->argv[argc]) + 1;
  /*******************************************************/  
  /* copy the command_line to where it should be in the stack */
  for(i = 0; i < ((char*)stack_top - cmd_line_on_stack);++i)
    {
      cmd_line_on_stack[i] = command_line[i];
    }
  /* build argv array and insert null-characters after each word */

  char* save_ptr;
  char* token;  
  /*j = 0 */
  esp-> argv[j] = token = strtok_r(cmd_line_on_stack," ", &save_ptr);  
  /* First argv now points to the first word */
  ++j; //Incremeted to get the other to pt to the right objecct
  /* cmd_line_on_stack gets divided into tokens since it is already on the stack
   * the existing stack gets modified*/ 
  do  
    { 
      token  = strtok_r(NULL, " ",&save_ptr);
      esp -> argv[j] = token;
    }while((token != NULL) && ++j); //NULL when the division into token is done
  /* j contains the sum of the delimeters*/


  return esp; /* the new stack top */
}
