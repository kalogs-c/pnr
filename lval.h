#ifndef LVAL_H
#define LVAL_H
typedef enum LTypes {
  LVAL_NUM,
  LVAL_ERR,
} lval_type_t;

typedef enum LErrors {
  LERR_DIV_ZERO,
  LERR_BAD_OP,
  LERR_BAD_NUM,
} lerr_t;

typedef union LResult {
  long num;
  lerr_t err;
} lresult_t;

typedef struct lval_t {
  lval_type_t type;
  lresult_t value;
} lval_t;

lval_t lval_num(long num);
lval_t lval_err(lerr_t err);

void lval_print(lval_t *v);
void lval_println(lval_t *v);
#endif // !LVAL_H
