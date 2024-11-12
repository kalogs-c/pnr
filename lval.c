#include "lval.h"
#include "mpc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lval_t* lval_num(long num) {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_NUM;
  lval->value.num = num;

  return lval;
}

lval_t* lval_err(char* err) {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_ERR;
  lval->value.err = (char*)malloc(strlen(err) + 1);
  strcpy(lval->value.err, err);

  return lval;
}

lval_t* lval_sym(char* sym) {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_SYM;
  lval->value.sym = (char*)malloc(strlen(sym) + 1);
  strcpy(lval->value.sym, sym);

  return lval;
}

lval_t* lval_sexpr() {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_SEXPR;
  lval->value.sexpr.count = 0;
  lval->value.sexpr.cell = NULL;

  return lval;
}

void lval_destroy(lval_t* v) {
  switch (v->type) {
  case LVAL_NUM:
    break;

  case LVAL_ERR:
    free(v->value.err);
    break;
  case LVAL_SYM:
    free(v->value.sym);
    break;

  case LVAL_SEXPR:
    for (int i = 0; i < v->value.sexpr.count; i++) {
      lval_destroy(v->value.sexpr.cell[i]);
    }
    free(v->value.sexpr.cell);
    break;
  }

  free(v);
}

lval_t* lval_add(lval_t* v, lval_t* x) {
  v->value.sexpr.count++;
  v->value.sexpr.cell =
      realloc(v->value.sexpr.cell, sizeof(lval_t*) * v->value.sexpr.count);
  v->value.sexpr.cell[v->value.sexpr.count - 1] = x;
  return v;
}

lval_t* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long number = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(number) : lval_err("invalid number");
}

lval_t* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }

  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  lval_t* v = NULL;

  // If root (>) then create sexpr
  if (strcmp(t->tag, ">") == 0) {
    v = lval_sexpr();
  }

  if (strstr(t->tag, "sexpr")) {
    v = lval_sexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, ")") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }

    v = lval_add(v, lval_read(t->children[i]));
  }

  return v;
}

void lval_expr_print(lval_t* v, char open, char close) {
  putchar(open);

  for (int i = 0; i < v->value.sexpr.count; i++) {
    lval_print(v->value.sexpr.cell[i]);

    if (i != v->value.sexpr.count - 1) {
      putchar(' ');
    }
  }

  putchar(close);
}

void lval_print(lval_t* v) {
  switch (v->type) {
  case LVAL_NUM:
    printf("%ld", v->value.num);
    break;

  case LVAL_ERR:
    printf("#<error: %s>", v->value.err);
    break;

  case LVAL_SYM:
    printf("%s", v->value.sym);
    break;

  case LVAL_SEXPR:
    lval_expr_print(v, '(', ')');
    break;
  }
}

void lval_println(lval_t* v) {
  lval_print(v);
  putchar('\n');
}

lval_t* lval_pop(lval_t* v, int i) {
  lval_t* x = v->value.sexpr.cell[i];

  memmove(&v->value.sexpr.cell[i], &v->value.sexpr.cell[i + 1],
          sizeof(lval_t*) * (v->value.sexpr.count - i - 1));

  v->value.sexpr.count--;

  v->value.sexpr.cell =
      realloc(v->value.sexpr.cell, sizeof(lval_t*) * v->value.sexpr.count);

  return x;
}

lval_t* lval_take(lval_t* v, int i) {
  lval_t* x = lval_pop(v, i);
  lval_destroy(v);
  return x;
}

lval_t* lval_eval(lval_t* v) {
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(v);
  }

  return v;
}

lval_t* builtin_op(lval_t* v, char* op) {
  // Ensure all arguments are numbers
  for (int i = 0; i < v->value.sexpr.count; i++) {
    if (v->value.sexpr.cell[i]->type != LVAL_NUM) {
      lval_destroy(v);
      return lval_err(
          "arguments must be numbers, cannot operate on non-numbers!");
    }
  }

  lval_t* x = lval_pop(v, 0);

  if (strcmp(op, "-") == 0 && v->value.sexpr.count == 0) {
    x->value.num = -x->value.num;
  }

  while (v->value.sexpr.count > 0) {
    lval_t* y = lval_pop(v, 0);

    if (strcmp(op, "+") == 0) {
      x->value.num += y->value.num;
    }

    if (strcmp(op, "-") == 0) {
      x->value.num -= y->value.num;
    }

    if (strcmp(op, "*") == 0) {
      x->value.num *= y->value.num;
    }

    if (strcmp(op, "^") == 0) {
      x->value.num = pow(x->value.num, y->value.num);
    }

    if (strcmp(op, "/") == 0) {
      if (y->value.num == 0) {
        lval_destroy(x);
        lval_destroy(y);
        x = lval_err("division by zero!");
        break;
      }
      x->value.num /= y->value.num;
    }

    if (strcmp(op, "%") == 0) {
      if (y->value.num == 0) {
        lval_destroy(x);
        lval_destroy(y);
        x = lval_err("division by zero!");
        break;
      }
      x->value.num = x->value.num % y->value.num;
    }

    lval_destroy(y);
  }

  lval_destroy(v);
  return x;
}

lval_t* lval_eval_sexpr(lval_t* v) {
  for (int i = 0; i < v->value.sexpr.count; i++) {
    v->value.sexpr.cell[i] = lval_eval(v->value.sexpr.cell[i]);
  }

  // Checking errors
  for (int i = 0; i < v->value.sexpr.count; i++) {
    if (v->value.sexpr.cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->value.sexpr.count == 0) {
    return v;
  }

  if (v->value.sexpr.count == 1) {
    return lval_take(v, 0);
  }

  // Assert first is a symbol
  lval_t* first = lval_pop(v, 0);
  if (first->type != LVAL_SYM) {
    lval_destroy(first);
    lval_destroy(v);
    return lval_err("S-expression does not start with symbol");
  }

  lval_t* result = builtin_op(v, first->value.sym);
  lval_destroy(first);
  return result;
};
