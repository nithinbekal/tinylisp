
#ifndef VALUE_H_INCLUDED_
#define VALUE_H_INCLUDED_

#include <stdlib.h>
#include "mpc.h"

typedef struct value Value;
typedef struct tl_env Env;

typedef Value*(*tl_builtin)(Env*, Value*);

struct value {
  int type;
  long num;

  char* err;
  char* sym;

  tl_builtin builtin;
  Env* env;
  Value* formals;
  Value* body;

  int count;
  struct value** cell;
};

struct tl_env {
  Env* parent;
  int count;
  char** syms;
  Value** vals;
};

enum { TL_INTEGER, TL_ERROR, TL_SYMBOL, TL_SEXPR, TL_QEXPR, TL_FUNCTION };

Value* tl_val_num(long);
Value* tl_val_error(char*, ...);
Value* tl_val_lambda(Value*, Value*);
Value* tl_val_sexpr();

Value* tl_val_add(Value*, Value*);
Value* tl_val_read(mpc_ast_t*);
Value* tl_val_pop(Value*, int);
Value* tl_val_take(Value*, int);
Value* tl_val_eval(Env*, Value*);
Value* tl_val_join(Value*, Value*);
Value* tl_val_fun(tl_builtin);
Value* tl_val_copy(Value*);
Value* tl_val_call(Env*, Value*, Value*);

void tl_val_print(Value*);
void tl_val_print_expr(Value*, char, char);
void tl_val_delete(Value*);

Env*   tl_env_new(void);
void   tl_env_delete(Env*);
Value* tl_env_get(Env*, Value*);
void   tl_env_put(Env*, Value*, Value*);
void   tl_env_def(Env*, Value*, Value*);
Env*   tl_env_copy(Env*);

void   tl_env_add_builtin(Env*, char*, tl_builtin);
void   tl_env_add_builtins(Env*);

char* tl_type_name(int);

#endif
