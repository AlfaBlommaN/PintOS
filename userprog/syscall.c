#include <stdio.h>
#include <syscall-nr.h>
#include "userprog/syscall.h"
#include "threads/interrupt.h"
//#include "threads/thread.h"

/* header files you probably need, they are not used yet */
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "devices/timer.h"

#include "threads/synch.h"
/* OUR CODE */

#define DBG(format, ...) //printf("# DEBUG: " format "\n", ##__VA_ARGS__)
#define CARRIAGE_RETURN 13

/* END OF OUR CODE*/
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* This array defined the number of arguments each syscall expects.
   For example, if you want to find out the number of arguments for
   the read system call you shall write:
   
   int sys_read_arg_count = argc[ SYS_READ ];
   
   All system calls have a name such as SYS_READ defined as an enum
   type, see `lib/syscall-nr.h'. Use them instead of numbers.
*/
const int argc[] = {
  /* basic calls */
  0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1, 
  /* not implemented */
  2, 1,    1, 1, 2, 1, 1,
  /* extended */
  0
};

static int 
read (int32_t *esp);

static int
write (int32_t *esp);

static int
open(const char *file);

static int
close(int fd);

static bool
remove(const char *file);

static bool
create(const char *file, unsigned initial_size);

static void
seek(int fd, unsigned position);

static unsigned 
tell(int fd);

static int
filesize(int fd);

static bool 
verify_variable_length(char* start);

static bool 
verify_fix_length(void* start, unsigned length);

static void 
kill_caller(void);

static void 
check_arguments(int32_t *esp);

static void
 syscall_handler (struct intr_frame *f) 
{
  int32_t* esp = (int32_t*)f->esp;
  unsigned i = 0;
  int number_of_arguments = 0;
  /*We first verify the first part of ESP */
  if(esp >=  PHYS_BASE || !verify_fix_length(esp,4))
    {
      kill_caller();
    }

  number_of_arguments = argc[0];
  /* Checks if first adress of esp is larger then phys base, if so 
   the following adresses will be larger aswell*/
  if(esp == NULL || (unsigned)esp >= (unsigned)PHYS_BASE || 
     !verify_fix_length(esp,number_of_arguments*4)) // Total length is 16
    {
      kill_caller();      
    }
    
  switch (esp[0])
    {
    case SYS_HALT:
      { 
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
	power_off(); 
	break;
      }

    case SYS_EXIT:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	DBG("# SYS_EXIT RETURNS %d",f->eax);
	process_exit((int)esp[1]);
	break;
      }

    case SYS_READ:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
	if(!verify_fix_length((void*)esp[2], (unsigned)esp[3]))
	  {
	    kill_caller();
	  }
	f-> eax = read(esp);
	break;
      }
    
    case SYS_WRITE: 
      {    
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	if(!verify_fix_length((void*)esp[2], (unsigned)esp[3]))
	  {
	    kill_caller();
	  }
	f-> eax = write(esp);
	break;
      }
    
    case SYS_OPEN:
      {
	/* Our implementation of open will return -1 on error */
	if(!verify_variable_length((char*) esp[1]))
	  {
	    kill_caller();
	  }
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	f -> eax = open((const char*) esp[1]); 
	break;
      }
    case SYS_CLOSE:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	if(close((int)esp[1]) == -1)
	  {
	    DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	  }
	break;
      }
    
    case SYS_REMOVE:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 

	if(!verify_variable_length((char *)esp[1]))
	  {
	    kill_caller();
	  }
	f -> eax = remove((const char*)esp[1]);
	break;
      }

    case SYS_CREATE:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	if(!verify_variable_length((char *)esp[1]))
	  {
	    kill_caller();
	  }
	f -> eax =  create((const char*)esp[1],(unsigned)esp[2]);
	break;
      }

    case SYS_FILESIZE:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	f -> eax = filesize((int)esp[1]);
	break;	
      }
      
    case  SYS_TELL:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	f -> eax = tell((int)esp[1]);
	break;
      }
    case SYS_SEEK:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 	
	seek((int)esp[1],(unsigned)esp[2]);
	break;
      }     
      
    case SYS_SLEEP:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	timer_msleep((int64_t)esp[1]);
	break;
      }
      
    case SYS_PLIST:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	process_print_list();
	break;
      }
      
    case SYS_EXEC:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	if(!verify_variable_length((char*)esp[1]))
	  {
	    kill_caller();
	  }
	f->eax = process_execute((const char *)esp[1]);
	break;
      }
    case SYS_WAIT:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	f->eax = process_wait((int)esp[1]);
	break;
      }
      
    default:
      {
	DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
	thread_exit ();
      }
    }
}

static int
read (int32_t *esp)
{
  char *buf = (char *) esp[2];
  int i = 0;
  size_t t = 0;
  struct thread *ptr = thread_current(); 
  struct file *f_ptr;
  
  if(esp[1] != STDIN_FILENO)
    {
      if((f_ptr = (map_find(&(ptr -> open_file_table),esp[1]))) == NULL)
	{
	  return -1;
	}
       
      else
	{
	  return file_read(f_ptr,(void*)esp[2],(unsigned)esp[3]);
	}
    }
  else
    {
      /* Read from keyboard */      
      while(i !=  esp[3])
	{
	  t = input_getc();

	  if(t != CARRIAGE_RETURN)
	    {
	      buf[i] = t;
	      putchar(t);
	    }
     
	  else
	    {
	      /* Makes carriage_return to  */
	      t-= 3;
	      buf[i] = t;
	      putchar(t);
	    }
	
	  ++i;
	}
    }

  return i;
}

static int 
write ( int32_t *esp)
{
  struct thread *ptr = thread_current(); 
  struct file *f_ptr;
  const char *buf_p = (const char *) esp[2];

  if((int)esp[1] != STDOUT_FILENO)
    {
      if((f_ptr = (map_find(&(ptr -> open_file_table),(int)esp[1]))) == NULL)
	{
	  return -1;
	}
      else
	{
	  return file_write(f_ptr,(const void *)esp[2],(unsigned)esp[3]);
	}	     
    }
  else
      {
	  /* Write to screen */
	  putbuf(buf_p, (unsigned)esp[3]);       
      }

  return (unsigned)esp[3];
}
/*If the map is full it is up to the user to make sure a file is closed */
/* We will return -1 on error */
static int
open(const char *file)
{
  int fd = 0;
  struct thread *ptr = thread_current(); 
  
  /* Look if we got space left in the map */
  if(!map_is_full(&(ptr -> open_file_table)))
    {
      if((fd = (map_insert(&(ptr -> open_file_table),filesys_open(file)))) != -1)  
	{
	  DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
	  return fd; 
	}
      else
	{
	  DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
	  return -1;
	}    
    }
  else
    {
      DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
      return -1;
    }   
}
/*Closes a file with a valid fd */
static int 
close(int fd)
{
  struct thread *ptr = thread_current();  

  /* fd < 2 reserved shall never be used they are in use by screeen and keyboard*/ 
  if(fd > 1 && map_find(&(ptr->open_file_table), fd) != NULL)
    {
      DBG("# Closing file with FD: %d ",fd);
      filesys_close(map_find(&(ptr ->  open_file_table),fd));
      map_remove(&(ptr -> open_file_table), fd);

      return 0;
    }
  return -1;
}
/*Returns true on sucess false on failure*/
static bool
remove(const char *file)
{
  if(filesys_remove(file))
    {
      DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
      return true;
    }
  else
    {
      DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
      return false;
    }
}
/* File create returns true on sucess false on failure*/
static bool
create(const char *file, unsigned initial_size)
{
  if(file == NULL)
    {
      return false;
    }

  if(filesys_create(file,initial_size))
    {
      return true;
    }
  else
    {
      DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
      return false;
    }
}

static void 
seek(int fd, unsigned position)
{
  struct thread *ptr = thread_current(); 
  file_seek(map_find(&(ptr-> open_file_table),fd),position);
}

static unsigned 
tell(int fd)
{
  struct thread *ptr = thread_current();
  struct file *fptr = map_find(&(ptr -> open_file_table),fd);
  if(fptr == NULL)
    {
      return -1;
    }
  else
    {
      return file_tell(fptr);
    }  
}

static int
filesize(int fd)
{
  struct thread *tptr = thread_current();
  struct file *fptr = map_find(&(tptr -> open_file_table),fd);
  if(fptr == NULL)
    {
      return -1;
    }
  else
    {
      return file_length(fptr); 
    }
}

/*This function is special, it will be passed to map_for_each and invoked there via a file pointer*/
void
process_close(int fd, struct file* ptr)
{
  struct thread *tptr = thread_current();
  filesys_close(ptr);
  /* Remove the entries just for the sake of order and sanity! */
  map_remove(&(tptr -> open_file_table), fd);
}

/* ********************************/
/* Code for Saftey checks         */
/* ********************************/

/*Checks lenght between start to start to start + length (NOT)*/
static bool 
verify_fix_length(void* start, unsigned length)
{
  char* beginning_of_page = NULL;
  char* end_of_adress = (char*)start + length;

  if( start == NULL || ((unsigned)(start + length) >= (unsigned)PHYS_BASE ))
    {
      return false;
    }
  
  for(beginning_of_page = pg_round_down(start)
	;beginning_of_page < end_of_adress
	;beginning_of_page += PGSIZE)
    {
      if(pagedir_get_page(thread_current()->pagedir,beginning_of_page) == NULL)
	{
	  return false;
	}
    }

  return true;
}

/* Kontrollera alla adresser från och med start till och med den
 * adress som först innehåller ett noll-tecken, `\0'. (C-strängar
 * lagras på detta sätt.) */
static bool 
verify_variable_length(char* start)
{ 
  size_t curr_page = 0,next_page = 0;
  char *curr = start;
  
  curr_page = next_page = pg_no(start);
    
  if(curr == NULL || (unsigned)curr >= (unsigned)PHYS_BASE)
    {
      return false;
    }

  while(true)
    {
      if((unsigned)curr >= (unsigned)PHYS_BASE)
	{
	  return false;
	}
   
      if(curr_page == next_page)
	{
	  if(pagedir_get_page(thread_current()->pagedir,curr) == NULL)
	    {
	      return false;
	    }
	  ++next_page;
	}
      if(*curr == '\0' && !((unsigned)curr >= (unsigned)PHYS_BASE))
	{
	  return true;
	}
      ++curr;
      curr_page = pg_no(curr);    
    }
  return false;
}

static void
 kill_caller(void) // Avslutar en process med exit-1
{
  DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
  process_exit(-1);
}
