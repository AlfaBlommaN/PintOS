#ifndef __PMAP__
#define __PMAP__
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "threads/synch.h"
#include "threads/malloc.h"


#define PMAP_SIZE 100
#define PMAP_DEBUG(...) //printf(__VA_ARGS__)
/*The interface for the process table it stores an array with pointers to structs 
 containing process info*/
 struct pmap 
{
  struct process_info *content[PMAP_SIZE];
  struct lock pmap_lock; 
  int siz; //Stores the current ammount of entries in the table
};

 struct process_info
{
  int proc_id;
  int parent_id;
  int exit_status;
  bool alive;
  bool parent_alive;
  bool freed;
  struct semaphore wait_sema; 
};


int 
pmap_constructor(struct pmap *m,struct semaphore *p);

int
pmap_insert(struct pmap *m);

struct process_info *
pmap_find(struct pmap *m, int key);

int
pmap_remove(struct pmap *m, int key);

int 
pmap_remove_process(struct pmap* m,int pid);

bool 
shall_be_removed(int key, struct pmap *m);

struct process_info*
pmap_find_process(struct pmap *m,int id);

bool 
pmap_process_alive(struct pmap* m, int id);

int
pmap_remove_childs(struct pmap* m, int key);

void 
set_parent_status(struct pmap* m,int key);

bool
pmap_is_full(struct pmap* m);

#endif


