#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

typedef struct tl_val {
  int type;
  long num;

  char* err;
  char* sym;

  int count;
  struct tl_val** cell;
} TL_VALUE;

enum { TL_INTEGER, TL_ERROR, TL_SYMBOL, TL_SEXPR };

TL_VALUE* eval(mpc_ast_t*);
TL_VALUE* eval_op(char*, TL_VALUE*, TL_VALUE*);

TL_VALUE* tl_val_num(long);
TL_VALUE* tl_val_error(char*);
void tl_val_print(TL_VALUE*);

int main(int argc, char** argv) {

  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Symbol   = mpc_new("symbol");
  mpc_parser_t* Sexpr    = mpc_new("sexpr");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Tinylisp = mpc_new("tinylisp");

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                           \
      number   : /-?[0-9]+/ ;                                   \
      symbol   : '+' | '-' | '*' | '/' ;                        \
      sexpr    : '(' <expr>* ')' ;                              \
      expr     : <number> | '(' <symbol> <expr>+ ')' ;          \
      tinylisp : /^/ <symbol> <expr>+ /$/ ;                     \
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
      tl_val_print(eval(r.output));
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

TL_VALUE* eval(mpc_ast_t* node) {
  if (strstr(node->tag, "number")) {
    errno = 0;
    long x = strtol(node->contents, NULL, 10);
    return errno != ERANGE ? tl_val_num(x) : tl_val_error("Invalid number");
  }
  
  char* op = node->children[1]->contents;
  TL_VALUE* x = eval(node->children[2]);

  for (int i=3; strstr(node->children[i]->tag, "expr"); i++) {
    x = eval_op(op, x, eval(node->children[i]));
  }

  return x;
}

TL_VALUE* eval_op(char* op, TL_VALUE* x, TL_VALUE* y) {
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

TL_VALUE* tl_val_num(long x) {
  TL_VALUE* v = malloc(sizeof(TL_VALUE));
  v->type = TL_INTEGER;
  v->num = x;
  return v;
}

TL_VALUE* tl_val_error(char* m) {
  TL_VALUE* v = malloc(sizeof(TL_VALUE));
  v->type = TL_ERROR;
  v->err = malloc(strlen(m)+1);
  strcpy(v->err, m);
  return v;
}

TL_VALUE* tl_val_symbol(char* s) {
  TL_VALUE* v = malloc(sizeof(TL_VALUE));
  v->type = TL_SYMBOL;
  v->sym = malloc(strlen(s)+1);
  strcpy(v->sym, s);
  return v;
}

TL_VALUE* tl_val_sexpr(void) {
  TL_VALUE* v = malloc(sizeof(TL_VALUE));
  v->type = TL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void tl_val_print(TL_VALUE* v) {
  switch (v->type) {
    case TL_INTEGER:
      printf("%ld\n", v->num);
      break;

    case TL_ERROR:
      printf("Error: %s.\n", v->err);
      break;
  }
}
