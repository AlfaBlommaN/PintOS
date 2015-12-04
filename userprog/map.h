#ifndef __MAP__
#define __MAP__
#define MAP_SIZE (128)
#include <stddef.h>
#include <stdio.h>

#define DBG(format, ...) printf("# DEBUG: " format "\n", ##__VA_ARGS__);

typedef int key_t;

struct map 
{  
  struct file * content [MAP_SIZE];
  key_t key;
  
};

void 
map_init(struct map* m);

key_t
map_insert(struct map* m, struct file * v);

struct file *
map_find(struct map* m, key_t k);

struct file * 
map_remove(struct map* m, key_t k);

bool 
map_is_full(struct map* m);

void map_for_each
(struct map* m,
 void (*exec)(key_t k, struct file * v));

#endif
