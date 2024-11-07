#include "mpc.h"
#include <editline/readline.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

long evaluate(mpc_ast_t *ast);
long evaluate_op(char *op, long left, long right);

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
      long output = evaluate(result.output);
      printf("%li\n", output);

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

long evaluate(mpc_ast_t *t) {
#ifdef DEBUG
  mpc_ast_print(t);
#endif /* ifdef DEBUG */

  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  char *op = t->children[1]->contents;

  long accumulator = evaluate(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    accumulator = evaluate_op(op, accumulator, evaluate(t->children[i]));
    i++;
  }

  return accumulator;
}

long evaluate_op(char *op, long left, long right) {
  if (strcmp(op, "+") == 0)
    return left + right;

  if (strcmp(op, "-") == 0)
    return left - right;

  if (strcmp(op, "*") == 0)
    return left * right;

  if (strcmp(op, "/") == 0)
    return left / right;

  if (strcmp(op, "%") == 0)
    return left % right;

  if (strcmp(op, "^") == 0)
    return pow(left, right);

  return 0;
}
