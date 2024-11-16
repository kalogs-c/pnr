#include "lval.h"
#include "mpc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lval_t* lval_num(long num) {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_NUM;
  lval->value.number = num;

  return lval;
}

lval_t* lval_err(char* err) {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_ERR;
  lval->value.error = (char*)malloc(strlen(err) + 1);
  strcpy(lval->value.error, err);

  return lval;
}

lval_t* lval_sym(char* sym) {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_SYM;
  lval->value.symbol = (char*)malloc(strlen(sym) + 1);
  strcpy(lval->value.symbol, sym);

  return lval;
}

lval_t* lval_sexpr() {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_SEXPR;
  lval->value.cell.count = 0;
  lval->value.cell.values = NULL;

  return lval;
}

lval_t* lval_qexpr() {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_QEXPR;
  lval->value.cell.count = 0;
  lval->value.cell.values = NULL;

  return lval;
}

lval_t* lval_fun(lbuiltin_t fun) {
  lval_t* lval = (lval_t*)malloc(sizeof(lval_t));
  lval->type = LVAL_FUN;
  lval->value.function = fun;

  return lval;
}

void lval_destroy(lval_t* v) {
  switch (v->type) {
  case LVAL_FUN:
  case LVAL_NUM:
    break;

  case LVAL_ERR:
    free(v->value.error);
    break;
  case LVAL_SYM:
    free(v->value.symbol);
    break;

  case LVAL_SEXPR:
  case LVAL_QEXPR:
    for (int i = 0; i < v->value.cell.count; i++) {
      lval_destroy(v->value.cell.values[i]);
    }
    free(v->value.cell.values);
    break;
  }

  free(v);
}

lval_t* lval_add(lval_t* v, lval_t* x) {
  v->value.cell.count++;
  v->value.cell.values =
      realloc(v->value.cell.values, sizeof(lval_t*) * v->value.cell.count);
  v->value.cell.values[v->value.cell.count - 1] = x;
  return v;
}

lval_t* lval_prepend(lval_t* v, lval_t* x) {
  v->value.cell.count++;
  v->value.cell.values =
      realloc(v->value.cell.values, sizeof(lval_t*) * v->value.cell.count);

  // Shift
  for (int i = v->value.cell.count - 1; i > 0; i--) {
    v->value.cell.values[i] = v->value.cell.values[i - 1];
  }

  v->value.cell.values[0] = x;

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

  for (int i = 0; i < v->value.cell.count; i++) {
    lval_print(v->value.cell.values[i]);

    if (i != v->value.cell.count - 1) {
      putchar(' ');
    }
  }

  putchar(close);
}

void lval_print(lval_t* v) {
  switch (v->type) {
  case LVAL_NUM:
    printf("%ld", v->value.number);
    break;

  case LVAL_ERR:
    printf("#<error: %s>", v->value.error);
    break;

  case LVAL_SYM:
    printf("%s", v->value.symbol);
    break;

  case LVAL_SEXPR:
    lval_expr_print(v, '(', ')');
    break;

  case LVAL_QEXPR:
    lval_expr_print(v, '{', '}');
    break;

  case LVAL_FUN:
    printf("<function>");
    break;
  }
}

void lval_println(lval_t* v) {
  lval_print(v);
  putchar('\n');
}

lval_t* lval_pop(lval_t* v, int i) {
  lval_t* x = v->value.cell.values[i];

  memmove(&v->value.cell.values[i], &v->value.cell.values[i + 1],
          sizeof(lval_t*) * (v->value.cell.count - i - 1));

  v->value.cell.count--;

  v->value.cell.values =
      realloc(v->value.cell.values, sizeof(lval_t*) * v->value.cell.count);

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
  for (int i = 0; i < v->value.cell.count; i++) {
    if (v->value.cell.values[i]->type != LVAL_NUM) {
      lval_destroy(v);
      return lval_err(
          "arguments must be numbers, cannot operate on non-numbers!");
    }
  }

  lval_t* x = lval_pop(v, 0);

  if (strcmp(op, "-") == 0 && v->value.cell.count == 0) {
    x->value.number = -x->value.number;
  }

  while (v->value.cell.count > 0) {
    lval_t* y = lval_pop(v, 0);

    if (strcmp(op, "+") == 0) {
      x->value.number += y->value.number;
    }

    if (strcmp(op, "-") == 0) {
      x->value.number -= y->value.number;
    }

    if (strcmp(op, "*") == 0) {
      x->value.number *= y->value.number;
    }

    if (strcmp(op, "^") == 0) {
      x->value.number = pow(x->value.number, y->value.number);
    }

    if (strcmp(op, "/") == 0) {
      if (y->value.number == 0) {
        lval_destroy(x);
        lval_destroy(y);
        x = lval_err("division by zero!");
        break;
      }
      x->value.number /= y->value.number;
    }

    if (strcmp(op, "%") == 0) {
      if (y->value.number == 0) {
        lval_destroy(x);
        lval_destroy(y);
        x = lval_err("division by zero!");
        break;
      }
      x->value.number = x->value.number % y->value.number;
    }

    lval_destroy(y);
  }

  lval_destroy(v);
  return x;
}

lval_t* lval_eval_sexpr(lval_t* v) {
  for (int i = 0; i < v->value.cell.count; i++) {
    v->value.cell.values[i] = lval_eval(v->value.cell.values[i]);
  }

  // Checking errors
  for (int i = 0; i < v->value.cell.count; i++) {
    if (v->value.cell.values[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->value.cell.count == 0) {
    return v;
  }

  if (v->value.cell.count == 1) {
    return lval_take(v, 0);
  }

  // Assert first is a symbol
  lval_t* first = lval_pop(v, 0);
  if (first->type != LVAL_SYM) {
    lval_destroy(first);
    lval_destroy(v);
    return lval_err("S-expression does not start with symbol");
  }

  lval_t* result = builtin(v, first->value.symbol);
  lval_destroy(first);
  return result;
};

#define LASSERT(condition, del_args, error)                                    \
  if (!(condition)) {                                                          \
    lval_destroy(del_args);                                                    \
    return lval_err(error);                                                    \
  }

lval_t* builtin_head(lval_t* v) {
  LASSERT(v->value.cell.count == 1, v, "Function head requires 1 argument");
  LASSERT(v->value.cell.values[0]->type == LVAL_QEXPR, v,
          "Function head requires a qexpr, incorrect type");
  LASSERT(v->value.cell.values[0]->value.cell.count > 0, v,
          "Function head requires a non-empty qexpr");

  lval_t* head = lval_take(v, 0);

  while (head->value.cell.count > 1) {
    lval_destroy(lval_pop(head, 1));
  }

  return head;
}

lval_t* builtin_tail(lval_t* v) {
  LASSERT(v->value.cell.count == 1, v, "Function tail requires 1 argument");
  LASSERT(v->value.cell.values[0]->type == LVAL_QEXPR, v,
          "Function tail requires a qexpr, incorrect type");
  LASSERT(v->value.cell.values[0]->value.cell.count > 0, v,
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
  LASSERT(v->value.cell.count == 2, v, "Function cons requires 2 arguments");
  LASSERT(v->value.cell.values[1]->type == LVAL_QEXPR, v,
          "Function cons requires a qexpr as second argument, incorrect type");

  return lval_prepend(lval_pop(v, 1), lval_take(v, 0));
}

lval_t* builtin_len(lval_t* v) {
  LASSERT(v->value.cell.count == 1, v, "Function len requires 1 argument");
  LASSERT(v->value.cell.values[0]->type == LVAL_QEXPR, v,
          "Function len requires a qexpr, incorrect type");

  return lval_num(v->value.cell.values[0]->value.cell.count);
}

lval_t* builtin_init(lval_t* v) {
  LASSERT(v->value.cell.count == 1, v, "Function init requires 1 argument");
  LASSERT(v->value.cell.values[0]->type == LVAL_QEXPR, v,
          "Function init requires a qexpr, incorrect type");
  LASSERT(v->value.cell.values[0]->value.cell.count > 0, v,
          "Function init requires a non-empty qexpr");

  lval_t* list = lval_take(v, 0);

  list->value.cell.count--;
  lval_destroy(list->value.cell.values[list->value.cell.count]);
  list->value.cell.values = realloc(list->value.cell.values,
                                    sizeof(lval_t*) * list->value.cell.count);

  return list;
}

lval_t* builtin_eval(lval_t* v) {
  LASSERT(v->value.cell.count == 1, v, "Function eval requires 1 argument");
  LASSERT(v->value.cell.values[0]->type == LVAL_QEXPR, v,
          "Function eval requires a qexpr, incorrect type");

  lval_t* x = lval_take(v, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval_t* lval_join(lval_t* x, lval_t* y) {
  while (y->value.cell.count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_destroy(y);
  return x;
}

lval_t* builtin_join(lval_t* v) {
  for (int i = 0; i < v->value.cell.count; i++) {
    LASSERT(v->value.cell.values[i]->type == LVAL_QEXPR, v,
            "Function join requires a qexpr, incorrect type");
  }

  lval_t* x = lval_pop(v, 0);

  while (v->value.cell.count > 0) {
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

lval_t* lval_copy(lval_t* v) {
  lval_t* x = (lval_t*)malloc(sizeof(lval_t));
  x->type = v->type;

  switch (x->type) {
  case LVAL_NUM:
    x->value.number = v->value.number;
    break;

  case LVAL_FUN:
    x->value.function = v->value.function;
    break;

  case LVAL_ERR:
    x->value.error = (char*)malloc(strlen(v->value.error) + 1);
    strcpy(x->value.error, v->value.error);
    break;

  case LVAL_SYM:
    x->value.symbol = (char*)malloc(strlen(v->value.symbol) + 1);
    strcpy(x->value.symbol, v->value.symbol);
    break;

  case LVAL_SEXPR:
  case LVAL_QEXPR:
    x->value.cell.count = v->value.cell.count;
    x->value.cell.values = malloc(sizeof(lval_t*) * x->value.cell.count);
    for (int i = 0; i < x->value.cell.count; i++) {
      x->value.cell.values[i] = lval_copy(v->value.cell.values[i]);
    }
    break;
  }

  return x;
}
