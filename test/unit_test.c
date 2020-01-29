// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include "../microforth.h"

static int forth_printer(const char *buf, int len, void *userdata) {
  return fprintf((FILE *) userdata, "%.*s", len, buf);
}

int main(int argc, char *argv[]) {
  struct forth f = {.printfn = forth_printer, .printfn_data = stdout};
  int i, j, c;

  forth_register_core_words(&f);

  // Process words from a command line arguments
  for (i = 1; i < argc; i++) {
    for (j = 0; argv[i][j] != '\0'; j++) forth_process_char(&f, argv[i][j]);
    forth_process_char(&f, ' ');
  }

  // If command line args where not given, process words from stdin
  while (argc < 2 && (c = getchar()) != EOF) forth_process_char(&f, c);

  return 0;
}
