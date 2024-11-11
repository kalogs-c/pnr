#include "lval.h"
#include <stdio.h>

lval_t lval_num(long num) {
  lresult_t value = {.num = num};
  lval_t lval = {.value = value, .type = LVAL_NUM};
  return lval;
}

lval_t lval_err(lerr_t err) {
  lresult_t value = {.err = err};
  lval_t lval = {.value = value, .type = LVAL_ERR};
  return lval;
}

void handle_error(int err) {
  switch (err) {
  case LERR_DIV_ZERO:
    printf("Error: division by zero\n");
    break;
  case LERR_BAD_OP:
    printf("Error: bad operator\n");
    break;
  case LERR_BAD_NUM:
    printf("Error: bad number\n");
    break;
  }
}

void lval_print(lval_t *v) {
  switch (v->type) {
  case LVAL_NUM:
    printf("%ld\n", v->value.num);
    break;
  case LVAL_ERR:
    handle_error(v->value.err);
    break;
  }
}

void lval_println(lval_t *v) {
  lval_print(v);
  putchar('\n');
}
