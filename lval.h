#ifndef LVAL_H
#define LVAL_H

#include "mpc.h"

// Forward declaration
typedef struct lval_t lval_t;
typedef struct lenv_t lenv_t;

typedef enum {
  LVAL_NUM,
  LVAL_ERR,
  LVAL_SYM,
  LVAL_SEXPR,
  LVAL_QEXPR,
  LVAL_FUN,
} type_t;

typedef struct {
  int count;
  lval_t** values;
} lcell_t;

typedef lval_t* (*lbuiltin_t)(lenv_t*, lval_t*);

typedef struct lval_t {
  type_t type;
  union {
    long number;
    char* error;
    char* symbol;
    lcell_t cell;
    lbuiltin_t function;
  } value;
} lval_t;

lval_t* lval_num(long num);
lval_t* lval_err(char* err);
lval_t* lval_sym(char* sym);
lval_t* lval_sexpr();
lval_t* lval_qexpr();

void lval_destroy(lval_t* v);

lval_t* lval_add(lval_t* v, lval_t* x);
lval_t* lval_read_num(mpc_ast_t* t);
lval_t* lval_read(mpc_ast_t* t);

void lval_print(lval_t* v);
void lval_println(lval_t* v);
void lval_expr_print(lval_t* v, char open, char close);

lval_t* builtin(lval_t* v, char* fun);
lval_t* lval_pop(lval_t* v, int i);
lval_t* lval_take(lval_t* v, int i);
lval_t* lval_eval(lval_t* v);
lval_t* lval_eval_sexpr(lval_t* v);
lval_t* lval_copy(lval_t* v);

#endif // !LVAL_H
