
#ifndef BUILTINS_H_INCLUDED_
#define BUILTINS_H_INCLUDED_

#include "value.h"

#define TL_ASSERT(args, cond, err) if(!(cond)) { tl_val_delete(args); return tl_val_error(err); }

Value* builtin(Value*, char*);
Value* builtin_op(Value*, char*);
Value* builtin_list(Value*);
Value* builtin_head(Value*);
Value* builtin_tail(Value*);
Value* builtin_eval(Value*);
Value* builtin_join(Value*);

#endif
