#include "lval.h"
#include "mpc.h"
#include <editline/readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

int main() {
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* SExpr = mpc_new("sexpr");
  mpc_parser_t* PNR = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
            " number : /-?[0-9]+(\\.[0-9]+)?/ ;                 "
            " symbol : '+' | '-' | '*' | '/' | '%' | '^' ;      "
            " sexpr    : '(' <expr>* ')' ;                      "
            " expr     : <number> | <symbol> | <sexpr> ;        "
            " lispy    : /^/ <expr>* /$/ ;             ",
            Number, Symbol, SExpr, Expr, PNR);

  puts("PNR Version 0.0.1");
  puts("Press Ctrl+C to exit\n");

  while (true) {
    char* input = readline("pnr> ");
    add_history(input);

    mpc_result_t result;
    if (mpc_parse("<stdin>", input, PNR, &result)) {
      /*lval_t output = evaluate(result.output);*/
      lval_t* output = lval_eval(lval_read(result.output));
      lval_println(output);
      lval_destroy(output);

      mpc_ast_delete(result.output);
    } else {
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Symbol, SExpr, Expr, PNR);
  return 0;
}
