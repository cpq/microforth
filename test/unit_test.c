// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include "../microforth.h"

static int forth_printer(const char *buf, int len, void *ptr) {
  return printf("%.*s", len, buf);
}

int main(int argc, char *argv[]) {
  struct forth f = {.printfn = forth_printer};
  int c;
  while ((c = getchar()) != EOF) forth_process_char(&f, c);
  return 0;
}
