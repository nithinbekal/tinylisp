
#ifndef __VALUE_H
#define __VALUE_H

#include <stdlib.h>

enum { TL_INTEGER, TL_ERROR, TL_SYMBOL, TL_SEXPR, TL_QEXPR };

typedef struct val {
  int type;
  long num;

  char* err;
  char* sym;

  int count;
  struct val** cell;
} TLVAL, *VAL;

VAL tl_val_num(long);

#endif
