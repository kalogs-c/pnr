#ifndef LVAL_H
#define LVAL_H

#include "mpc.h"

typedef enum LTypes {
  LVAL_NUM,
  LVAL_ERR,
  LVAL_SYM,
  LVAL_SEXPR,
} lval_type_t;

typedef enum LErrors {
  LERR_DIV_ZERO,
  LERR_BAD_OP,
  LERR_BAD_NUM,
} lerr_t;

typedef struct lsexpr_t {
  int count;
  struct lval_t** cell;
} lsexpr_t;

typedef union LResult {
  long num;
  char* err;
  char* sym;
  lsexpr_t sexpr;
} lresult_t;

typedef struct lval_t {
  lval_type_t type;
  lresult_t value;
} lval_t;

lval_t* lval_num(long num);
lval_t* lval_err(char* err);
lval_t* lval_sym(char* sym);
lval_t* lval_sexpr();

void lval_destroy(lval_t* v);

lval_t* lval_add(lval_t* v, lval_t* x);
lval_t* lval_read_num(mpc_ast_t* t);
lval_t* lval_read(mpc_ast_t* t);

void lval_print(lval_t* v);
void lval_println(lval_t* v);
void lval_expr_print(lval_t* v, char open, char close);

lval_t* lval_pop(lval_t* v, int i);
lval_t* lval_take(lval_t* v, int i);
lval_t* lval_eval(lval_t* v);
lval_t* lval_eval_sexpr(lval_t* v);

#endif // !LVAL_H
