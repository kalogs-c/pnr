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
  lval->value.cellw.count = 0;
  lval->value.cellw.cell = NULL;

  return lval;
}

lval_t* lval_qexpr() {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_QEXPR;
  lval->value.cellw.count = 0;
  lval->value.cellw.cell = NULL;

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
  case LVAL_QEXPR:
    for (int i = 0; i < v->value.cellw.count; i++) {
      lval_destroy(v->value.cellw.cell[i]);
    }
    free(v->value.cellw.cell);
    break;
  }

  free(v);
}

lval_t* lval_add(lval_t* v, lval_t* x) {
  v->value.cellw.count++;
  v->value.cellw.cell =
      realloc(v->value.cellw.cell, sizeof(lval_t*) * v->value.cellw.count);
  v->value.cellw.cell[v->value.cellw.count - 1] = x;
  return v;
}

lval_t* lval_prepend(lval_t* v, lval_t* x) {
  v->value.cellw.count++;
  v->value.cellw.cell =
      realloc(v->value.cellw.cell, sizeof(lval_t*) * v->value.cellw.count);

  // Shift
  for (int i = v->value.cellw.count - 1; i > 0; i--) {
    v->value.cellw.cell[i] = v->value.cellw.cell[i - 1];
  }

  v->value.cellw.cell[0] = x;

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

  if (strstr(t->tag, "qexpr")) {
    v = lval_qexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0 ||
        strcmp(t->children[i]->contents, ")") == 0 ||
        strcmp(t->children[i]->contents, "{") == 0 ||
        strcmp(t->children[i]->contents, "}") == 0) {
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

  for (int i = 0; i < v->value.cellw.count; i++) {
    lval_print(v->value.cellw.cell[i]);

    if (i != v->value.cellw.count - 1) {
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

  case LVAL_QEXPR:
    lval_expr_print(v, '{', '}');
    break;
  }
}

void lval_println(lval_t* v) {
  lval_print(v);
  putchar('\n');
}

lval_t* lval_pop(lval_t* v, int i) {
  lval_t* x = v->value.cellw.cell[i];

  memmove(&v->value.cellw.cell[i], &v->value.cellw.cell[i + 1],
          sizeof(lval_t*) * (v->value.cellw.count - i - 1));

  v->value.cellw.count--;

  v->value.cellw.cell =
      realloc(v->value.cellw.cell, sizeof(lval_t*) * v->value.cellw.count);

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
  for (int i = 0; i < v->value.cellw.count; i++) {
    if (v->value.cellw.cell[i]->type != LVAL_NUM) {
      lval_destroy(v);
      return lval_err(
          "arguments must be numbers, cannot operate on non-numbers!");
    }
  }

  lval_t* x = lval_pop(v, 0);

  if (strcmp(op, "-") == 0 && v->value.cellw.count == 0) {
    x->value.num = -x->value.num;
  }

  while (v->value.cellw.count > 0) {
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
  for (int i = 0; i < v->value.cellw.count; i++) {
    v->value.cellw.cell[i] = lval_eval(v->value.cellw.cell[i]);
  }

  // Checking errors
  for (int i = 0; i < v->value.cellw.count; i++) {
    if (v->value.cellw.cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->value.cellw.count == 0) {
    return v;
  }

  if (v->value.cellw.count == 1) {
    return lval_take(v, 0);
  }

  // Assert first is a symbol
  lval_t* first = lval_pop(v, 0);
  if (first->type != LVAL_SYM) {
    lval_destroy(first);
    lval_destroy(v);
    return lval_err("S-expression does not start with symbol");
  }

  lval_t* result = builtin(v, first->value.sym);
  lval_destroy(first);
  return result;
};

#define LASSERT(condition, del_args, error)                                    \
  if (!(condition)) {                                                          \
    lval_destroy(del_args);                                                    \
    return lval_err(error);                                                    \
  }

lval_t* builtin_head(lval_t* v) {
  LASSERT(v->value.cellw.count == 1, v, "Function head requires 1 argument");
  LASSERT(v->value.cellw.cell[0]->type == LVAL_QEXPR, v,
          "Function head requires a qexpr, incorrect type");
  LASSERT(v->value.cellw.cell[0]->value.cellw.count > 0, v,
          "Function head requires a non-empty qexpr");

  lval_t* head = lval_take(v, 0);

  while (head->value.cellw.count > 1) {
    lval_destroy(lval_pop(head, 1));
  }

  return head;
}

lval_t* builtin_tail(lval_t* v) {
  LASSERT(v->value.cellw.count == 1, v, "Function tail requires 1 argument");
  LASSERT(v->value.cellw.cell[0]->type == LVAL_QEXPR, v,
          "Function tail requires a qexpr, incorrect type");
  LASSERT(v->value.cellw.cell[0]->value.cellw.count > 0, v,
          "Function tail requires a non-empty qexpr");

  lval_t* tail = lval_take(v, 0);

  lval_destroy(lval_pop(tail, 0));
  return tail;
}

lval_t* builtin_list(lval_t* v) {
  v->type = LVAL_QEXPR;
  return v;
}

lval_t* builtin_cons(lval_t* v) {
  LASSERT(v->value.cellw.count == 2, v, "Function cons requires 2 arguments");
  LASSERT(v->value.cellw.cell[1]->type == LVAL_QEXPR, v,
          "Function cons requires a qexpr as second argument, incorrect type");

  return lval_prepend(lval_pop(v, 1), lval_take(v, 0));
}

lval_t* builtin_len(lval_t* v) {
  LASSERT(v->value.cellw.count == 1, v, "Function len requires 1 argument");
  LASSERT(v->value.cellw.cell[0]->type == LVAL_QEXPR, v,
          "Function len requires a qexpr, incorrect type");

  return lval_num(v->value.cellw.cell[0]->value.cellw.count);
}

lval_t* builtin_init(lval_t* v) {
  LASSERT(v->value.cellw.count == 1, v, "Function init requires 1 argument");
  LASSERT(v->value.cellw.cell[0]->type == LVAL_QEXPR, v,
          "Function init requires a qexpr, incorrect type");
  LASSERT(v->value.cellw.cell[0]->value.cellw.count > 0, v,
          "Function init requires a non-empty qexpr");

  lval_t* list = lval_take(v, 0);

  list->value.cellw.count--;
  lval_destroy(list->value.cellw.cell[list->value.cellw.count]);
  list->value.cellw.cell = realloc(list->value.cellw.cell,
                                   sizeof(lval_t*) * list->value.cellw.count);

  return list;
}

lval_t* builtin_eval(lval_t* v) {
  LASSERT(v->value.cellw.count == 1, v, "Function eval requires 1 argument");
  LASSERT(v->value.cellw.cell[0]->type == LVAL_QEXPR, v,
          "Function eval requires a qexpr, incorrect type");

  lval_t* x = lval_take(v, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval_t* lval_join(lval_t* x, lval_t* y) {
  while (y->value.cellw.count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_destroy(y);
  return x;
}

lval_t* builtin_join(lval_t* v) {
  for (int i = 0; i < v->value.cellw.count; i++) {
    LASSERT(v->value.cellw.cell[i]->type == LVAL_QEXPR, v,
            "Function join requires a qexpr, incorrect type");
  }

  lval_t* x = lval_pop(v, 0);

  while (v->value.cellw.count > 0) {
    x = lval_join(x, lval_pop(v, 0));
  }

  lval_destroy(v);
  return x;
}

lval_t* builtin(lval_t* v, char* fun) {
  if (strcmp(fun, "list") == 0)
    return builtin_list(v);

  if (strcmp(fun, "head") == 0)
    return builtin_head(v);

  if (strcmp(fun, "tail") == 0)
    return builtin_tail(v);

  if (strcmp(fun, "cons") == 0)
    return builtin_cons(v);

  if (strcmp(fun, "len") == 0)
    return builtin_len(v);

  if (strcmp(fun, "init") == 0)
    return builtin_init(v);

  if (strcmp(fun, "join") == 0)
    return builtin_join(v);

  if (strcmp(fun, "eval") == 0)
    return builtin_eval(v);

  if (strstr("+-*/%^", fun))
    return builtin_op(v, fun);

  lval_destroy(v);
  return lval_err("function not implemented");
}
