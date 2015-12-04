#include "map.h"
/* MAP_SIZE IS 128  */

/* Inits map and all positions to NULL*/
void 
map_init(struct map* m)
{
  int i = 0;
  m -> key = 0;

  for(; i < MAP_SIZE; ++i)
    {
      m -> content[i] = NULL;      
      
    }  
}
/* Inserts element return  returns -1 on error*/
key_t
map_insert(struct map* m, struct file * v)
{  
  int i = 0;
  for(; i < MAP_SIZE; ++i)
    {  
      if(v == NULL)
	{
	  /*If invalid file pointer */
	  return -1;
	}
      else if (m->content[i] == NULL)
	{
	  m->content[i] = v;
	  return (m -> key = ( i + 2)); 
	}
      else
	{
	  continue;
	}
    }  
  /*  If the map is full we return -1 */
  return -1;
}
/* Remember that the caller needs to specify a correct fd */
struct file *
map_find(struct map* m, key_t k)
{
  if(k < 2 || 128 < k || (m -> content[k-2] == NULL))
    {
      return NULL; 
    }
  
  return m->content[k-2];
}

/* removes element at requested key */
struct file *
map_remove(struct map* m, key_t k)
{
  if(k < 2 || 128 < k || (m -> content[k-2] == NULL))
    {
      //printf("\n Could not remove object with fd %d \n", k);
      return NULL;
    }

  return m->content[k-2] = NULL;
}
/*Apply given function on each element in the map */
void 
map_for_each
(struct map* m, void (*exec)(key_t k,struct file* v))
 {
   int i = 0;
   for(; i < MAP_SIZE; ++i)
     {
       /* This will invoke the function passed to each above. */
       /* In the remove file case the function called will be process_close */
        exec(i + 2,  m -> content[i]);
     }
 }

bool 
map_is_full(struct map *m)
{
  int i = 0;
  for(; i < MAP_SIZE; ++i)
    {
      if(m->content[i] == NULL)
	{
	  return false;
	}
    }
  return true;   
}
