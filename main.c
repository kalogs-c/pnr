#include "lval.h"
#include "mpc.h"
#include <editline/readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

void debugp(mpc_result_t* t) {
#ifdef DEBUG
  putchar('\n');
  puts("## AST DEBUG ##");
  mpc_ast_print(t->output);
  puts("## DEBUG AST ##");
  putchar('\n');
#endif /* ifdef DEBUG */
}

int main() {
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* SExpr = mpc_new("sexpr");
  mpc_parser_t* QExpr = mpc_new("qexpr");
  mpc_parser_t* PNR = mpc_new("lispy");

  mpca_lang(
      MPCA_LANG_DEFAULT,
      " number : /-?[0-9]+(\\.[0-9]+)?/ ;                                "
      " symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;                      "
      " sexpr  : '(' <expr>* ')' ;                                       "
      " qexpr  : '{' <expr>* '}' ;                                       "
      " expr   : <number> | <symbol> | <sexpr> | <qexpr> ;               "
      " lispy  : /^/ <expr>* /$/ ;                                       ",
      Number, Symbol, SExpr, QExpr, Expr, PNR);

  puts("PNR Version 0.0.1");
  puts("Press Ctrl+C to exit\n");

  while (true) {
    char* input = readline("pnr> ");
    add_history(input);

    mpc_result_t result;
    if (mpc_parse("<stdin>", input, PNR, &result)) {
      debugp(&result);

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

  mpc_cleanup(6, Number, Symbol, SExpr, QExpr, Expr, PNR);
  return 0;
}
