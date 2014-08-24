#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

typedef struct {
  int type;
  long num;
  int err;
} TL_VALUE;

enum { TL_INTEGER, TL_ERROR };
enum { TLERR_ZERO_DIV, TLERR_BAD_OP, TLERR_BAD_NUM };

TL_VALUE eval(mpc_ast_t*);
TL_VALUE eval_op(char*, TL_VALUE, TL_VALUE);

TL_VALUE tl_val_num(long);
TL_VALUE tl_val_error(int);
void tl_val_print(TL_VALUE);

int main(int argc, char** argv) {

  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Tinylisp = mpc_new("tinylisp");

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                           \
      number   : /-?[0-9]+/ ;                                   \
      operator : '+' | '-' | '*' | '/' ;                        \
      expr     : <number> | '(' <operator> <expr>+ ')' ;        \
      tinylisp : /^/ <operator> <expr>+ /$/ ;                   \
    ",
    Number, Operator, Expr, Tinylisp);

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

  mpc_cleanup(4, Number, Operator, Expr, Tinylisp);

  return 0;
}

TL_VALUE eval(mpc_ast_t* node) {
  if (strstr(node->tag, "number")) {
    errno = 0;
    long x = strtol(node->contents, NULL, 10);
    return errno != ERANGE ? tl_val_num(x) : tl_val_error(TLERR_BAD_NUM);
  }
  
  char* op = node->children[1]->contents;
  TL_VALUE x = eval(node->children[2]);

  for (int i=3; strstr(node->children[i]->tag, "expr"); i++) {
    x = eval_op(op, x, eval(node->children[i]));
  }

  return x;
}

TL_VALUE eval_op(char* op, TL_VALUE x, TL_VALUE y) {
  if (x.type == TL_ERROR) { return x; }
  if (y.type == TL_ERROR) { return y; }

  if (strcmp(op, "+") == 0) { return tl_val_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return tl_val_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return tl_val_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    return y.num == 0 ? tl_val_error(TLERR_ZERO_DIV) : tl_val_num(x.num / y.num);
  }

  return tl_val_error(TLERR_BAD_OP);
}

TL_VALUE tl_val_num(long x) {
  TL_VALUE v;
  v.type = TL_INTEGER;
  v.num = x;
  return v;
}

TL_VALUE tl_val_error(int x) {
  TL_VALUE v;
  v.type = TL_ERROR;
  v.err = x;
  return v;
}

void tl_val_print(TL_VALUE v) {
  switch (v.type) {
    case TL_INTEGER:
      printf("%ld\n", v.num);
      break;

    case TL_ERROR:
      if (v.err == TLERR_ZERO_DIV)  { printf("Error: Divide by 0.\n"); }
      if (v.err == TLERR_BAD_OP)    { printf("Error: Invalid operator.\n"); }
      if (v.err == TLERR_BAD_NUM)   { printf("Error: Invalid number.\n"); }
      break;
  }
}
