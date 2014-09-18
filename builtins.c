
#include "builtins.h"

Value* builtin_op(Env* e, Value* a, char* op) {
  for (int i=0; i < a->count; i++) {
    if(a->cell[i]->type != TL_INTEGER) {
      tl_val_delete(a);
      return tl_val_error("Cannot operate on non-number");
    }
  }

  Value* x = tl_val_pop(a, 0);

  if(strcmp(op, "-") && a->count == 0)
    x->num = -x->num;

  while (a->count > 0) {
    Value* y = tl_val_pop(a, 0);

    if(strcmp(op, "+") == 0) x->num += y->num;
    if(strcmp(op, "-") == 0) x->num -= y->num;
    if(strcmp(op, "*") == 0) x->num *= y->num;

    if(strcmp(op, "/") == 0) {
      if (y->num == 0) {
        tl_val_delete(x);
        tl_val_delete(y);
        x = tl_val_error("Divide by zero");
        break;
      }
      x->num /= y->num;
    }
  }

  tl_val_delete(a);
  return x;
}

Value* builtin_list(Env* e, Value* v) {
  v->type = TL_QEXPR;
  return v;
}

Value* builtin_head(Env* e, Value* v) {
  TL_ASSERT(v, (v->count == 1),
      "Function 'head' passed too many arguments. Got %i, Expected %i",
      v->count, 1);

  TL_ASSERT(v, (v->cell[0]->type == TL_QEXPR),
      "Function 'head' passed invalid types.");

  TL_ASSERT(v, (v->cell[0]->count != 0),
      "Function 'head' passed empty list");

  Value* x = tl_val_take(v, 0);
  while(x->count > 1) tl_val_delete(tl_val_pop(x, 1));
  return x;
}

Value* builtin_tail(Env* e, Value* v) {
  TL_ASSERT(v, (v->count == 1), "Function 'tail' passed too many arguments");
  TL_ASSERT(v, (v->cell[0]->type == TL_QEXPR), "Function 'tail' passed invalid type");
  TL_ASSERT(v, (v->cell[0]->count != 0), "Function 'tail' passed empty list");

  Value* x = tl_val_take(v, 0);
  tl_val_delete(tl_val_pop(x, 0));
  return x;
}

Value* builtin_eval(Env* e, Value* v) {
  TL_ASSERT(v, (v->count == 1), "Function 'eval' passed too many arguments");
  TL_ASSERT(v, (v->cell[0]->type == TL_QEXPR), "Function 'eval' passed invalid types");

  Value* x = tl_val_take(v, 0);
  x->type = TL_SEXPR;
  return tl_val_eval(e, x);
}

Value* builtin_join(Env* e, Value* v) {
  for(int i=0; i < v->count; i++) {
    TL_ASSERT(v, (v->cell[0]->type == TL_QEXPR), "Function 'join' passed invalid type");
  }

  Value* x = tl_val_pop(v, 0);
  while(v->count > 0) x = tl_val_join(x, tl_val_pop(v, 0));
  tl_val_delete(v);
  return x;
}

Value* builtin_lambda(Env* e, Value* v) {
  TL_ASSERT(v, (v->count == 2), "Expecting 2 arguments");
  TL_ASSERT(v, (v->cell[0]->type == TL_QEXPR), "Expecting Q-expr as lambda formal params");
  TL_ASSERT(v, (v->cell[1]->type == TL_QEXPR), "Expecting Q-expr as lambda body");

  for(int i=0; i < v->cell[0]->count; i++) {
    TL_ASSERT(v, (v->cell[0]->cell[i]->type == TL_SYMBOL), "Lambda params must be symbols");
  }

  Value* formals = tl_val_pop(v, 0);
  Value* body = tl_val_pop(v, 0);
  tl_val_delete(v);

  return tl_val_lambda(formals, body);
}

Value* builtin_add      (Env* e, Value* v) { return builtin_op(e, v, "+"); }
Value* builtin_subtract (Env* e, Value* v) { return builtin_op(e, v, "-"); }
Value* builtin_multiply (Env* e, Value* v) { return builtin_op(e, v, "*"); }
Value* builtin_divide   (Env* e, Value* v) { return builtin_op(e, v, "/"); }

