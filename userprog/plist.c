#include <stddef.h>
#include "plist.h"
#define DBG(format, ...) //printf("# DEBUG: " format "\n", ##__VA_ARGS__);
struct pmap global_process_table;

/* 
   This function will update the system wide process table by settings appropriate statuses
   the routines to handle insertions can be found in src/userprog/pmap.c */
int 
insert_process_info(size_t pmap_key,int proc_id, int parent_id,
		    int exit_status, bool alive, bool parent_alive)
{
  DBG("# Insert process info entered %d in %s with process id %d & with name %s ", __LINE__, __FILE__ 
      , thread_current()->tid, thread_current()->name); 

  if(pmap_key > PMAP_SIZE -1)
    {
      DBG("# Left Insert_process_info:  %d in %s", __LINE__, __FILE__); 
      DBG("# with process id %d & with name %s ", thread_current()->tid, thread_current()->name);  
      return -1;
    }
  if(global_process_table.siz == PMAP_SIZE) //If the map is full
    {    
      DBG("# Left insert_process_info:  %d in %s", __LINE__, __FILE__); 
      DBG("# with process id %d & with name %s ", thread_current()->tid, thread_current()->name); 
      return -1;
    }
  global_process_table.content[pmap_key] -> proc_id = proc_id;
  global_process_table.content[pmap_key] -> parent_id = parent_id;
  global_process_table.content[pmap_key] -> exit_status = exit_status; 
  global_process_table.content[pmap_key] -> alive = alive;
  global_process_table.content[pmap_key] -> parent_alive = parent_alive; 
  DBG("# Left insert_process_info:  %d in %s", __LINE__, __FILE__); 
  DBG("# with process id %d & with name %s ", thread_current()->tid, thread_current()->name); 
  return 0;
}

/*
  Function to remove a process info struct
  if that is possible to do returns true if it removed
  sucessfully, false otherwise.
*/
bool 
remove_process_info()
{

  DBG("# Remove process info entered %d in %s", __LINE__, __FILE__); 
  DBG("# with process id %d & with name %s ", thread_current()->tid, thread_current()->name); 

  int pmap_key = thread_current()->pmap_key;
  int parent_id = 0;
  /* Checks that the pmap_key is valid
     if not we do not continue from this point*/
  DBG("# %d i filen %s", __LINE__, __FILE__);      
  if(pmap_key > PMAP_SIZE && !pmap_process_alive(&global_process_table,thread_current()->tid))
    {
      DBG("# %d i filen %s", __LINE__, __FILE__);      
      DBG("# Left remove_process_info  %d in %s", __LINE__, __FILE__); 
      DBG("# with process id %d & with name %s ", thread_current()->tid, thread_current()->name); 
      return false;
    }
  parent_id = global_process_table.content[pmap_key]->parent_id;
  DBG("# %d i filen %s", __LINE__, __FILE__);

  /*if the parent id is one it is ofcourse alive since main is alive */
  if(pmap_process_alive(&global_process_table,parent_id) || parent_id == 1)
    {
      /*If parent is still alive and well we do not remove any entry 
	however we are dead as far as we know..*/
      DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
      DBG("# Process marked as not alive! ");
      global_process_table.content[pmap_key] -> alive = false;
      pmap_remove_childs(&global_process_table,pmap_key); 
      // process_print_list();
      DBG("# Left process_execute: %d in %s", __LINE__, __FILE__); 
      DBG("# with process id %d & with name %s ", thread_current()->tid, thread_current()->name); 
      return false;
    }
  else if(!pmap_process_alive(&global_process_table,parent_id))
    {
      /* in this case our parent is dead as such we can die 
	 however we must inform our children about our demise*/
      DBG("# exekverade rad %d i filen %s", __LINE__, __FILE__); 
      DBG("# Removed an entry from the global list table ");
      global_process_table.content[pmap_key] -> alive = false;
      pmap_remove_childs(&global_process_table,pmap_key);
      pmap_remove(&global_process_table, pmap_key);   
      DBG("# Left remove_process_info:  %d in %s", __LINE__, __FILE__); 
      DBG("# with process id %d & with name %s ", thread_current()->tid, thread_current()->name); 
      return true;
    }
  DBG("# Left remove_process_info:  %d in %s", __LINE__, __FILE__); 
  DBG("# with process id %d & with name %s ", thread_current()->tid, thread_current()->name); 
  return false;
}

/*Calls pmap_remove_process in a safe way */
int plist_remove_process(struct pmap* m,int pid)
{
  int ret = 0;
  lock_acquire(&(global_process_table.pmap_lock));
  ret = pmap_remove_process(m,pid);
  lock_release(&(global_process_table.pmap_lock));
  return ret;
}

bool plist_insert(struct pmap *m)
{
  pmap_insert(m);
}
