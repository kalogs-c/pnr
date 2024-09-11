#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0

int main() {
  puts("Lispy Version 0.0.1");
  puts("Press Ctrl+C to exit\n");

  while (TRUE) {
    char *input = readline("Lispy> ");
    add_history(input);

    printf("Amo %s\n", input);
    free(input);
  }
  return 0;
}
