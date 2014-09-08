
#ifndef VALUE_H_INCLUDED_
#define VALUE_H_INCLUDED_

#include <stdlib.h>
#include "mpc.h"

typedef struct value {
  int type;
  long num;

  char* err;
  char* sym;

  int count;
  struct value** cell;
} Value;

enum { TL_INTEGER, TL_ERROR, TL_SYMBOL, TL_SEXPR, TL_QEXPR };

Value* tl_val_num(long);
Value* tl_val_error(char*);
Value* tl_val_read(mpc_ast_t*);
Value* tl_val_pop(Value*, int);
Value* tl_val_take(Value*, int);
Value* tl_val_eval(Value*);
Value* tl_val_join(Value*, Value*);

void tl_val_print(Value*);
void tl_val_print_expr(Value*, char, char);
void tl_val_delete(Value*);

#endif
