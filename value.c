
#include "value.h"
#include "builtins.h"

Value* tl_val_num(long x) {
  Value* v = malloc(sizeof(Value));
  v->type = TL_INTEGER;
  v->num = x;
  return v;
}

Value* tl_val_string(char* s) {
  Value* v = malloc(sizeof(Value));
  v->type = TL_STRING;
  v->str = malloc(strlen(s) + 1);
  strcpy(v->str, s);
  return v;
}

Value* tl_val_error(char* fmt, ...) {
  Value* v = malloc(sizeof(Value));
  v->type = TL_ERROR;

  va_list va;
  va_start(va, fmt);

  v->err = malloc(512);
  vsnprintf(v->err, 511, fmt, va);
  v->err = realloc(v->err, strlen(v->err)+1);

  va_end(va);

  return v;
}

Value* tl_val_symbol(char* s) {
  Value* v = malloc(sizeof(Value));
  v->type = TL_SYMBOL;
  v->sym = malloc(strlen(s)+1);
  strcpy(v->sym, s);
  return v;
}

Value* tl_val_sexpr(void) {
  Value* v = malloc(sizeof(Value));
  v->type = TL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

Value* tl_val_qexpr(void) {
  Value* v = malloc(sizeof(Value));
  v->type = TL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

Value* tl_val_lambda(Value* formals, Value* body) {
  Value* v = malloc(sizeof(Value));
  v->type = TL_FUNCTION;

  v->builtin = NULL;
  v->env = tl_env_new();

  v->formals = formals;
  v->body = body;

  return v;
}

Value* tl_val_call(Env* e, Value* fn, Value* args) {
  if(fn->builtin) { return fn->builtin(e, args); }

  int given = args->count;
  int total = fn->formals->count;

  while (args->count) {
    if (fn->formals->count == 0) {
      tl_val_delete(args);
      return tl_val_error(
          "Function passed too many arguments. "
          "Got %i, expected %i.", given, total);
    }

    Value* sym = tl_val_pop(fn->formals, 0);

    if (strcmp(sym->sym, "&") == 0) {
      if (fn->formals->count != 1) {
        tl_val_delete(args);
        return tl_val_error("Function format invalid."
            "Symbol '&' not followed by single symbol");
      }

      Value* symbols = tl_val_pop(fn->formals, 0);
      tl_env_put(fn->env, symbols, builtin_list(e, args));
      tl_val_delete(sym);
      tl_val_delete(symbols);
      break;
    }

    Value* val = tl_val_pop(args, 0);
    tl_env_put(fn->env, sym, val);
    tl_val_delete(sym);
    tl_val_delete(val);
  }

  if (fn->formals->count > 0 && strcmp(fn->formals->cell[0]->sym, "&") == 0) {
    if (fn->formals->count != 2) {
      return tl_val_error("Function format invalid."
          "Symbol '&' not followed by single symbol");
    }

    tl_val_delete(tl_val_pop(fn->formals, 0));
    Value* symbol = tl_val_pop(fn->formals, 0);
    Value* value  = tl_val_qexpr();

    tl_env_put(e, symbol, value);
    tl_val_delete(symbol);
    tl_val_delete(value);
  }

  tl_val_delete(args);

  if (fn->formals->count == 0) {
    // Eval and return if all formals have been bound
    fn->env->parent = e;
    return builtin_eval(fn->env,
        tl_val_add(tl_val_sexpr(), tl_val_copy(fn->body)));
  } else {
    // Partially evaluated function
    return tl_val_copy(fn);
  }
}

void tl_val_print_string(Value* v) {
  char* escaped = malloc(strlen(v->str) + 1);
  strcpy(escaped, v->str);
  escaped = mpcf_escape(escaped);
  printf("\"%s\"", escaped);  
  free(escaped);
}

void tl_val_print(Value* v) {
  switch (v->type) {
    case TL_INTEGER:
      printf("%ld", v->num);
      break;

    case TL_ERROR:
      printf("Error: %s.", v->err);
      break;

    case TL_SYMBOL:
      printf("%s", v->sym);
      break;

    case TL_STRING:
      tl_val_print_string(v);
      break;

    case TL_SEXPR:
      tl_val_print_expr(v, '(', ')');
      break;

    case TL_QEXPR:
      tl_val_print_expr(v, '{', '}');
      break;

    case TL_FUNCTION:
      if (v->builtin) {
        printf("<function>");
      } else {
        printf("(\\ ");
        tl_val_print(v->formals);
        putchar(' ');
        tl_val_print(v->body);
        putchar(')');
      }
      break;
  }
}

void tl_val_delete(Value* v) {
  switch(v->type) {
    case TL_ERROR:   free(v->err); break;
    case TL_SYMBOL:  free(v->sym); break;
    case TL_STRING:  free(v->str); break;

    case TL_INTEGER:  break;

    case TL_QEXPR:
    case TL_SEXPR:
      for(int i=0; i < v->count; i++) tl_val_delete(v->cell[i]);
      free(v->cell);
      break;

    case TL_FUNCTION:
      if (!v->builtin) {
        tl_env_delete(v->env);
        tl_val_delete(v->formals);
        tl_val_delete(v->body);
      }
      break;
  }
  free(v);
}

Value* tl_val_add(Value* v, Value* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(Value*) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

Value* tl_val_read_integer(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? tl_val_num(x) : tl_val_error("Invalid number");
}

Value* tl_val_read_str(mpc_ast_t* t) {
  t->contents[strlen(t->contents)-1] = '\0';
  char* unescaped = malloc(strlen(t->contents+1)+1);
  strcpy(unescaped, t->contents+1);
  unescaped = mpcf_unescape(unescaped);
  Value* str = tl_val_string(unescaped);
  free(unescaped);
  return str;
}

Value* tl_val_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) { return tl_val_read_integer(t); }
  if (strstr(t->tag, "symbol")) { return tl_val_symbol(t->contents); }

  Value* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = tl_val_sexpr();  }
  if (strstr(t->tag, "sexpr")) { x = tl_val_sexpr(); }
  if (strstr(t->tag, "qexpr")) { x = tl_val_qexpr(); }
  if (strstr(t->tag, "string")) { return tl_val_read_str(t); }

  for(int i = 0; i<t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) continue;
    if (strcmp(t->children[i]->contents, ")") == 0) continue;
    if (strcmp(t->children[i]->contents, "{") == 0) continue;
    if (strcmp(t->children[i]->contents, "}") == 0) continue;
    if (strcmp(t->children[i]->tag,  "regex") == 0) continue;

    x = tl_val_add(x, tl_val_read(t->children[i]));
  }
  return x;
}

void tl_val_print_expr(Value* v, char open, char close) {
  putchar(open);
  putchar(' ');
  for(int i=0; i<v->count; i++) {
    tl_val_print(v->cell[i]);
    if (i != (v->count - 1)) putchar(' ');
  }
  putchar(' ');
  putchar(close);
}

Value* tl_val_eval_sexpr(Env* e, Value* v) {
  for(int i=0; i < v->count; i++)
    v->cell[i] = tl_val_eval(e, v->cell[i]);

  for(int i=0; i < v->count; i++)
    if (v->cell[i]->type == TL_ERROR)
      return tl_val_take(v, i);

  if (v->count == 0) return v;

  if (v->count == 1) return tl_val_take(v, 0);

  Value* f = tl_val_pop(v, 0);
  if (f->type != TL_FUNCTION) {
    Value* err = tl_val_error(
        "S-expression starts with incorrect type. "
        "Got %s, expected %s.",
        tl_type_name(f->type), tl_type_name(TL_FUNCTION));
    tl_val_delete(f);
    tl_val_delete(v);
    return err;
  }

  Value* result = tl_val_call(e, f, v);
  tl_val_delete(f);
  return result;
}

Value* tl_val_eval(Env* e, Value* v) {
  if (v->type == TL_SYMBOL) {
    Value* x = tl_env_get(e, v);
    tl_val_delete(v);
    return x;
  }

  if (v->type == TL_SEXPR)
    return tl_val_eval_sexpr(e, v);

  return v;
}

Value* tl_val_pop(Value* v, int i) {
  Value* x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i+1], sizeof(Value*)*(v->count-i-1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(Value*)*v->count);
  return x;
}

Value* tl_val_take(Value* v, int i) {
  Value* x = tl_val_pop(v, i);
  tl_val_delete(v);
  return x;
}

Value* tl_val_join(Value* x, Value* y) {
  while (y->count)
    x = tl_val_add(x, tl_val_pop(y, 0));
  tl_val_delete(y);
  return x;
}

Value* tl_val_fun(tl_builtin func) {
  Value* v = malloc(sizeof(Value));
  v->type = TL_FUNCTION;
  v->builtin = func;
  return v;
}

Value* tl_val_copy(Value* v) {
  Value* x = malloc(sizeof(Value));
  x->type = v->type;

  switch(v->type) {
    case TL_INTEGER:  x->num = v->num; break;

    case TL_ERROR:
      x->err = malloc(strlen(v->err)+1);
      strcpy(x->err, v->err);
      break;

    case TL_SYMBOL:
      x->sym = malloc(strlen(v->sym)+1);
      strcpy(x->sym, v->sym);
      break;

    case TL_STRING:
      x->str = malloc(strlen(v->str)+1);
      strcpy(x->str, v->str);
      break;

    case TL_SEXPR:
    case TL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(Value*) * v->count);
      for (int i=0; i < x->count; i++)
        x->cell[i] = tl_val_copy(v->cell[i]);
      break;

    case TL_FUNCTION:
      if (v->builtin) {
        x->builtin = v->builtin; break;
      } else {
        x->builtin = NULL;
        x->env = tl_env_copy(v->env);
        x->formals = tl_val_copy(v->formals);
        x->body = tl_val_copy(v->body);
      }
      break;
  }
  return x;
}

Env* tl_env_new(void) {
  Env* e = malloc(sizeof(Env));
  e->parent = NULL;
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void tl_env_delete(Env* e) {
  for(int i=0; i < e->count; i++) {
    free(e->syms[i]);
    free(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

Value* tl_env_get(Env* e, Value* v) {
  for(int i=0; i < e->count; i++) {
    if (strcmp(e->syms[i], v->sym) == 0)
      return tl_val_copy(e->vals[i]);
  }

  if (e->parent) {
    return tl_env_get(e->parent, v);
  } else {
    return tl_val_error("Unbound symbol '%s'", v->sym);
  }
}

void tl_env_put(Env* e, Value* s, Value* v) {
  for(int i=0; i < e->count; i++) {
    if (strcmp(e->syms[i], s->sym) == 0) {
      tl_val_delete(e->vals[i]);
      e->vals[i] = tl_val_copy(v);
      return;
    }
  }

  e->count++;
  e->vals = realloc(e->vals, sizeof(Value*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  e->vals[e->count - 1] = tl_val_copy(v);
  e->syms[e->count - 1] = malloc(strlen(s->sym)+1);
  strcpy(e->syms[e->count - 1], s->sym);
}

void tl_env_def(Env* e, Value* k, Value* v) {
  while (e->parent) e = e->parent;
  tl_env_put(e, k, v);
}

Env* tl_env_copy(Env* e) {
  Env* n = malloc(sizeof(Env));
  n->parent = e->parent;
  n->count = e->count;
  n->syms = malloc(sizeof(char*) * n->count);
  n->vals = malloc(sizeof(Env*) * n->count);
  for(int i=0; i < n->count; i++) {
    n->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(n->syms[i], e->syms[i]);
    n->vals[i] = tl_val_copy(e->vals[i]);
  }
  return n;
}

void tl_env_add_builtin(Env* e, char* name, tl_builtin func) {
  Value* s = tl_val_symbol(name);
  Value* f = tl_val_fun(func);
  tl_env_put(e, s, f);
  tl_val_delete(s);
  tl_val_delete(f);
}

void tl_env_add_builtins(Env* e) {
  tl_env_add_builtin(e, "list", builtin_list);
  tl_env_add_builtin(e, "head", builtin_head);
  tl_env_add_builtin(e, "tail", builtin_tail);
  tl_env_add_builtin(e, "eval", builtin_eval);
  tl_env_add_builtin(e, "join", builtin_join);

  tl_env_add_builtin(e, "+", builtin_add);
  tl_env_add_builtin(e, "-", builtin_subtract);
  tl_env_add_builtin(e, "*", builtin_multiply);
  tl_env_add_builtin(e, "/", builtin_divide);

  tl_env_add_builtin(e, "def", builtin_def);
  tl_env_add_builtin(e, "=",   builtin_put);
  tl_env_add_builtin(e, "\\",  builtin_lambda);

  // Comparison
  tl_env_add_builtin(e, "if", builtin_if);
  tl_env_add_builtin(e, "==", builtin_eq);
  tl_env_add_builtin(e, "!=", builtin_ne);
  tl_env_add_builtin(e, ">",  builtin_gt);
  tl_env_add_builtin(e, "<",  builtin_lt);
  tl_env_add_builtin(e, ">=", builtin_ge);
  tl_env_add_builtin(e, "<=", builtin_le);
}

char* tl_type_name(int t) {
  switch(t) {
    case TL_FUNCTION: return "Function";
    case TL_INTEGER:  return "Number";
    case TL_ERROR:    return "Error";
    case TL_SYMBOL:   return "Symbol";
    case TL_SEXPR:    return "S-expression";
    case TL_QEXPR:    return "Q-expression";
    default:          return "Unknown";
  }
}
