/*Includes */
#include "userprog/myClib.h"


int
round_up(int value)
{
  bool done = false;
  while(!done)
    {
      if(value % 4 == 0)
	{
	  done = true;
	}
      else
	{
	  ++value;
	}
    }
  return value;

}

int
count_args(const char* buf, const char* delimeters)
{
  int i = 0;
  bool prev_was_delim;
  bool cur_is_delim = true;
  int argc = 0;

  while (buf[i] != '\0')
  {
    prev_was_delim = cur_is_delim;
    cur_is_delim = exists_in(buf[i], delimeters);
    argc += (prev_was_delim && !cur_is_delim);
    ++i;
  }
  return argc;
}

bool
exists_in(char c, const char* d)
{
  int i = 0;
  while (d[i] != '\0' && d[i] != c)
    ++i;
  return (d[i] == c);
}

void print_pretty(struct pmap *ptr,int siz)
{
  int i = 0;
  printf("######################\n");
  printf("GLOBAL PROCESS TABLE\n");
  printf("######################\n");
  printf("Total stored entries: %d \n", siz);
  for(; i < siz; ++i)
    {
      printf("-------------------------------------------------------------------------------------------------------------------------\n");
      printf("ENTRY: %d \t | ",i);
      if(ptr->content[i] != NULL)
	{
	  printf("proc_id %3d \t| parent_id: %3d \t| exit_status: %3d \t| alive %3d \t| parent_alive %3d \n",  
		 ptr->content[i]->proc_id,
		 ptr->content[i]->parent_id,
		 ptr->content[i]->exit_status,
		 ptr->content[i]->alive,
		 ptr->content[i]->parent_alive);
	  printf("-------------------------------------------------------------------------------------------------------------------------\n");
	}
      else
	{
	  printf("-------------------------------------------------------------------------------------------------------------------------\n");
	  printf("\t\t FREE \t\t");
	  printf("-------------------------------------------------------------------------------------------------------------------------\n");
	}
    }
}
