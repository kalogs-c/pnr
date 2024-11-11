#include "lval.h"
#include "mpc.h"
#include <editline/readline.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

lval_t evaluate_op(char *op, lval_t left, lval_t right) {
  if (left.type == LVAL_ERR)
    return left;

  if (right.type == LVAL_ERR)
    return right;

  if (strcmp(op, "+") == 0)
    return lval_num(left.value.num + right.value.num);

  if (strcmp(op, "-") == 0)
    return lval_num(left.value.num - right.value.num);

  if (strcmp(op, "*") == 0)
    return lval_num(left.value.num * right.value.num);

  if (strcmp(op, "/") == 0) {
    if (right.value.num == 0) {
      return lval_err(LERR_DIV_ZERO);
    }

    return lval_num(left.value.num / right.value.num);
  }

  if (strcmp(op, "%") == 0) {
    if (right.value.num == 0) {
      return lval_err(LERR_DIV_ZERO);
    }

    return lval_num(left.value.num % right.value.num);
  }

  if (strcmp(op, "^") == 0)
    return lval_num(pow(left.value.num, right.value.num));

  return lval_err(LERR_BAD_OP);
}

lval_t evaluate(mpc_ast_t *t) {
#ifdef DEBUG
  mpc_ast_print(t);
#endif /* ifdef DEBUG */

  if (strstr(t->tag, "number")) {
    errno = 0;
    long number = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(number) : lval_err(LERR_BAD_NUM);
  }

  char *op = t->children[1]->contents;

  lval_t accumulator = evaluate(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    accumulator = evaluate_op(op, accumulator, evaluate(t->children[i]));
    i++;
  }

  return accumulator;
}

int main() {
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
            " number : /-?[0-9]+(\\.[0-9]+)?/ ; "
            " operator : '+' | '-' | '*' | '/' | '%' | '^' ;            "
            " expr     : <number> | '(' <operator> <expr>+ ')' ;  "
            " lispy    : /^/ <operator> <expr>+ /$/ ;             ",
            Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.0.1");
  puts("Press Ctrl+C to exit\n");

  while (true) {
    char *input = readline("Lispy> ");
    add_history(input);

    mpc_result_t result;
    if (mpc_parse("<stdin>", input, Lispy, &result)) {
      lval_t output = evaluate(result.output);
      lval_println(&output);

      mpc_ast_delete(result.output);
    } else {
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  return 0;
}
