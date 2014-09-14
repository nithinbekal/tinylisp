
#ifndef BUILTINS_H_INCLUDED_
#define BUILTINS_H_INCLUDED_

#include "value.h"

#define TL_ASSERT(args, cond, fmt, ...) \
  if (!(cond)) { \
    Value* err = tl_val_error(fmt, ##__VA_ARGS__); \
    tl_val_delete(args); \
    return err; \
  }

Value* builtin_op(Env*, Value*, char*);
Value* builtin_list(Env*, Value*);
Value* builtin_head(Env*, Value*);
Value* builtin_tail(Env*, Value*);
Value* builtin_eval(Env*, Value*);
Value* builtin_join(Env*, Value*);

Value* builtin_add      (Env*, Value*);
Value* builtin_subtract (Env*, Value*);
Value* builtin_multiply (Env*, Value*);
Value* builtin_divide   (Env*, Value*);

#endif
