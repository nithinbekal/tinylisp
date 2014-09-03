#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

typedef struct tl_value {
  int type;
  long num;

  char* err;
  char* sym;

  int count;
  struct tl_value** cell;
} TLVAL, *TL_VALUE;

enum { TL_INTEGER, TL_ERROR, TL_SYMBOL, TL_SEXPR };

TL_VALUE eval_op(char*, TL_VALUE, TL_VALUE);
TL_VALUE eval_sexp(TL_VALUE);

TL_VALUE tl_val_num(long);
TL_VALUE tl_val_error(char*);
TL_VALUE tl_val_read(mpc_ast_t*);
TL_VALUE tl_val_pop(TL_VALUE, int);
TL_VALUE tl_val_take(TL_VALUE, int);
TL_VALUE tl_val_eval(TL_VALUE);

void tl_val_print(TL_VALUE);
void tl_val_print_expr(TL_VALUE, char, char);
void tl_val_delete(TL_VALUE);

TL_VALUE builtin_op(TL_VALUE, char*);

int main(int argc, char** argv) {

  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Symbol   = mpc_new("symbol");
  mpc_parser_t* Sexpr    = mpc_new("sexpr");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Tinylisp = mpc_new("tinylisp");

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                             \
      number   : /-?[0-9]+/ ;                     \
      symbol   : '+' | '-' | '*' | '/' ;          \
      sexpr    : '(' <expr>* ')' ;                \
      expr     : <number> | <symbol> | <sexpr> ;  \
      tinylisp : /^/ <expr>* /$/ ;                \
    ",
    Number, Symbol, Sexpr, Expr, Tinylisp);

  mpc_result_t r;

  puts("Tinylisp Version 0.0.1");
  puts("Press Ctrl+c to Exit\n");

  while(1) {
    char* input = readline("tinylisp> ");
    add_history(input);

    if (strcmp(input, "exit") == 0) return 0;

    if (mpc_parse("<stdin>", input, Tinylisp, &r)) {
      TL_VALUE x = tl_val_eval(tl_val_read(r.output));
      tl_val_print(x);
      tl_val_delete(x);

      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Tinylisp);

  return 0;
}

TL_VALUE eval_op(char* op, TL_VALUE x, TL_VALUE y) {
  if (x->type == TL_ERROR) { return x; }
  if (y->type == TL_ERROR) { return y; }

  if (strcmp(op, "+") == 0) { return tl_val_num(x->num + y->num); }
  if (strcmp(op, "-") == 0) { return tl_val_num(x->num - y->num); }
  if (strcmp(op, "*") == 0) { return tl_val_num(x->num * y->num); }
  if (strcmp(op, "/") == 0) {
    return y->num == 0 ? tl_val_error("Divide by zero") : tl_val_num(x->num / y->num);
  }

  return tl_val_error("Invalid operator");
}

TL_VALUE tl_val_num(long x) {
  TL_VALUE v = malloc(sizeof(TLVAL));
  v->type = TL_INTEGER;
  v->num = x;
  return v;
}

TL_VALUE tl_val_error(char* m) {
  TL_VALUE v = malloc(sizeof(TLVAL));
  v->type = TL_ERROR;
  v->err = malloc(strlen(m)+1);
  strcpy(v->err, m);
  return v;
}

TL_VALUE tl_val_symbol(char* s) {
  TL_VALUE v = malloc(sizeof(TLVAL));
  v->type = TL_SYMBOL;
  v->sym = malloc(strlen(s)+1);
  strcpy(v->sym, s);
  return v;
}

TL_VALUE tl_val_sexpr(void) {
  TL_VALUE v = malloc(sizeof(TLVAL));
  v->type = TL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void tl_val_print(TL_VALUE v) {
  switch (v->type) {
    case TL_INTEGER:
      printf("%ld\n", v->num);
      break;

    case TL_ERROR:
      printf("Error: %s.\n", v->err);
      break;

    case TL_SYMBOL:
      printf("%s", v->sym);
      break;

    case TL_SEXPR:
      tl_val_print_expr(v, '(', ')');
      break;
  }
}

void tl_val_delete(TL_VALUE v) {
  switch(v->type) {
    case TL_INTEGER: break;
    case TL_ERROR:   free(v->err); break;
    case TL_SYMBOL:  free(v->sym); break;

    case TL_SEXPR:
      for(int i=0; i < v->count; i++) tl_val_delete(v->cell[i]);
      free(v->cell);
      break;
  }
  free(v);
}

TL_VALUE tl_val_add(TL_VALUE v, TL_VALUE x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(TL_VALUE) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

TL_VALUE tl_val_read_integer(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? tl_val_num(x) : tl_val_error("Invalid number");
}

TL_VALUE tl_val_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) { return tl_val_read_integer(t); }
  if (strstr(t->tag, "symbol")) { return tl_val_symbol(t->contents); }

  TL_VALUE x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = tl_val_sexpr();  }
  if (strstr(t->tag, "sexpr")) { x = tl_val_sexpr(); }

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

void tl_val_print_expr(TL_VALUE v, char open, char close) {
  putchar(open);
  for(int i=0; i<v->count; i++) {
    tl_val_print(v->cell[i]);
    if (i != (v->count - 1)) putchar(' ');
  }
  putchar(close);
}

TL_VALUE tl_val_eval_sexpr(TL_VALUE v) {
  for(int i=0; i < v->count; i++)
    v->cell[i] = tl_val_eval(v->cell[i]);

  for(int i=0; i < v->count; i++)
    if (v->cell[i]->type == TL_ERROR)
      return tl_val_take(v, i);

  if (v->count == 0) return v;

  if (v->count == 1) return tl_val_take(v, 0);

  TL_VALUE f = tl_val_pop(v, 0);
  if (f->type != TL_SYMBOL) {
    tl_val_delete(f);
    tl_val_delete(v);
    return tl_val_error("S-expression does not start with a symbol");
  }

  TL_VALUE result = builtin_op(v, f->sym);
  tl_val_delete(f);
  return result;
}

TL_VALUE tl_val_eval(TL_VALUE v) {
  if (v->type == TL_SEXPR) return tl_val_eval_sexpr(v);
  return v;
}

TL_VALUE tl_val_pop(TL_VALUE v, int i) {
  TL_VALUE x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i+1], sizeof(TL_VALUE)*(v->count-i-1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(TL_VALUE)*v->count);
  return x;
}

TL_VALUE tl_val_take(TL_VALUE v, int i) {
  TL_VALUE x = tl_val_pop(v, i);
  tl_val_delete(v);
  return x;
}

TL_VALUE builtin_op(TL_VALUE a, char* op) {
  for (int i=0; i < a->count; i++) {
    if(a->cell[i]->type != TL_INTEGER) {
      tl_val_delete(a);
      return tl_val_error("Cannot operate on non-number");
    }
  }

  TL_VALUE x = tl_val_pop(a, 0);

  if(strcmp(op, "-") && a->count == 0)
    x->num = -x->num;

  while (a->count > 0) {
    TL_VALUE y = tl_val_pop(a, 0);

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
