#include "pmap.h"
/*This constructur will only run once */
int 
pmap_constructor(struct pmap *m, struct semaphore *p)
{
  memset(m->content, 0, sizeof(*(m->content)));
  lock_init(&(m->pmap_lock)); //For locking in process.c/h
  sema_init(p,0);
  m->siz = 0;      
  return 0;
}
/*
  Informs the children that we have died in the case
  that we have died but our parent is still alive. 
*/
void set_parent_status(struct pmap* m,int key)
{  
  if(m->content[key] == NULL)
    {
      return;
    }
  int i = 0; 
  int our_id = m->content[key]->proc_id; //We are the parent in this context
 
  for(;i < PMAP_SIZE;++i)
    {
      if(m->content[i] == 0)
	{
	  continue;
	}
      else if(m->content[i]->parent_id == our_id)
	{
	  PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	  m->content[i]->parent_alive = false;
  	}   
    } 
  return;       
}

/* 
   Allocate a new process_info struct and place it on the heap, on sucess it returns 
   the key that specifies its index in the table and increases the siz variable.
*/
int
pmap_insert(struct pmap *m)
{
  int i = 0;
  
  for(; i < PMAP_SIZE; ++i)
    {
      if(m-> content[i] == NULL)
	{
	  PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	  m->content[i] = malloc(sizeof(struct process_info));
	  m->content[i] -> alive = false;
	  m->content[i] -> parent_alive = false;
	  sema_init(&(m->content[i] -> wait_sema), 0); 
	  m->siz = m->siz +1;
	  return i;
	}      
    }
  return i;
}

/* 
Returns a process_info struct 
given a specific key.
 */
struct process_info *
pmap_find(struct pmap *m, int key)
{
  if(key > PMAP_SIZE -1 || (m -> content[key] == NULL))
    {
      PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
      return NULL;
    }
  PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
  return m->content[key];
}

/*Removes a struct process_info given a key
  from the list and free the memory
  if sucessfull decrements the siz parameter of the pmap struct
*/
int
pmap_remove(struct pmap *m, int key)
{
   if( key > PMAP_SIZE -1 || m->content[key] == 0)
    {
      PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__);
      return -1;
    }
  /* Free alocated memory*/
  PMAP_DEBUG("# rad exekverade %d i filen %s", __LINE__, __FILE__); 
  free(m->content[key]);
  m->content[key] = NULL;
  m->siz = m->siz - 1; 

  return 0;
}


/*Removes a process given a specific process id */
int 
pmap_remove_process(struct pmap* m,int pid)
{
  int i = 0;
  for(; i < PMAP_SIZE;++i)
    {
      if(m->content[i] != NULL)
	{
	  if(m->content[i]->proc_id == pid)
	    {
	      free(m->content[i]);
	      m->content[i] = NULL;
	      m-> siz = m->siz -1;
	    }
	}
    }

  return 0;
}



/* 
   Removes children marked as dead.
   to the calling parent process when a removal occurs 
   the function will decrement the siz variable of the pmap struct
   the function will also inform living children that we have died.
*/
int 
pmap_remove_childs(struct pmap* m, int key)
{
  if(m->content[key] == NULL)
    {
      return -1;
    }
 
  int caller_id = m->content[key]->proc_id;
  int i = 0;
  for(;i < PMAP_SIZE; ++i)
    {
      if(m->content[i] == NULL || m->content[i]->proc_id == caller_id)
	{
	  continue;
	}
      else if(m->content[i] -> parent_id == caller_id && (!(m->content[i]->alive)))
	{
	  PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	  free(m->content[i]);
	  m->content[i] = 0;
	  m->siz = m->siz - 1;
	}
      else if(m->content[i] -> parent_id == caller_id && (m->content[i]->alive))
	{
	  PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
	  m->content[i]->parent_alive = false;
  	}     
    }


  return 0;
}

/*Find process with a specific id number
  we return a pointer to that specific structure
  returns null if not found*/
struct process_info*
pmap_find_process(struct pmap *m,int id)
{
  int i = 0;

  for(; i < PMAP_SIZE;++i)
    {
      if(m->content[i] != NULL)
	{
	  if(m->content[i]->proc_id == id)
	    {
	      return m->content[i];
	    }
	}
    }
  PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__);   
  return NULL;
}

/*
  Checks if a process with a given id  is alive 
  ,the argument int  id is the id of the 
  process to check*/
bool 
pmap_process_alive(struct pmap* m, int id)
{
  int i = 0;

  for(; i < PMAP_SIZE;++i)
    {
      if(m->content[i] == NULL)
	{
	  continue;
	}
      else if(id == (m -> content[i] -> proc_id))
	{
	  if((m-> content[i] -> alive))
	    {
	      return true;
	    }
	}
    }

  PMAP_DEBUG("# exekverade rad %d i filen %s", __LINE__, __FILE__);   
  return false;
}

bool
pmap_is_full(struct pmap* m)
{
  lock_acquire(&(m->pmap_lock));
  if(m->siz == PMAP_SIZE)
    {
      lock_release(&(m->pmap_lock));
      return true;
    }
  lock_release(&(m->pmap_lock));
  return false;
}
