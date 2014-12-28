
#ifndef BUILTINS_H_INCLUDED_
#define BUILTINS_H_INCLUDED_

#include "value.h"

#define TL_ASSERT(args, cond, fmt, ...) \
  if (!(cond)) { \
    Value* err = tl_val_error(fmt, ##__VA_ARGS__); \
    tl_val_delete(args); \
    return err; \
  }

#define TL_ASSERT_NUM(fn, args, num) \
  TL_ASSERT(args, args->count == num, \
      "Function '%s' passed incorrect number of arguments. Got: %s, expected: %s", \
      fn, args->count, num)

#define TL_ASSERT_TYPE(func, args, index, expect) \
  TL_ASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, tl_type_name(args->cell[index]->type), tl_type_name(expect))

Value* builtin_op(Env*, Value*, char*);
Value* builtin_list(Env*, Value*);
Value* builtin_head(Env*, Value*);
Value* builtin_tail(Env*, Value*);
Value* builtin_eval(Env*, Value*);
Value* builtin_join(Env*, Value*);
Value* builtin_lambda(Env*, Value*);
Value* builtin_var(Env*, Value*, char*);
Value* builtin_def(Env*, Value*);
Value* builtin_put(Env*, Value*);

Value* builtin_add      (Env*, Value*);
Value* builtin_subtract (Env*, Value*);
Value* builtin_multiply (Env*, Value*);
Value* builtin_divide   (Env*, Value*);

Value* builtin_eq  (Env*, Value*);
Value* builtin_ne  (Env*, Value*);
Value* builtin_gt  (Env*, Value*);
Value* builtin_ge  (Env*, Value*);
Value* builtin_lt  (Env*, Value*);
Value* builtin_le  (Env*, Value*);
Value* builtin_ord (Env*, Value*, char*);

#endif
