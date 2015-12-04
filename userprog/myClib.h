#ifndef USERPROG_MYCLIB_H
#define USERPROG_MYCLIB_H
#include <stdbool.h>
#include <stdio.h>
#include "pmap.h"

int round_up(int value);
int count_args(const char* buf, const char* delimeters);
bool exists_in(char c, const char* d);
bool is_equal(const char* str1,const char* str2);
void print_pretty(struct pmap* ptr,int siz);

#endif /* userprog/pagedir.h */
