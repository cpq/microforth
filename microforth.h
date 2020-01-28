// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifndef MICROFORTH_MAX_WORD_LEN
#define MICROFORTH_MAX_WORD_LEN 64
#endif

#ifndef MICROFORTH_STACK_SIZE
#define MICROFORTH_STACK_SIZE 10
#endif

typedef int (*forth_print_fn_t)(const char *buf, int len, void *userdata);
//typedef int (*forth_vprint_fn_t)(forth_print_fn_t, void *, va_list *);

struct word {
  struct word *next;
};

struct forth {
  char buf[MICROFORTH_MAX_WORD_LEN];
  int buflen;
  double stack[MICROFORTH_STACK_SIZE];
  int stacklen;
  unsigned char inword;
  forth_print_fn_t printfn;
  void *printfn_data;
  struct word *words;
};

int forth_vprintf(forth_print_fn_t fn, void *fnd, const char *fmt, va_list p) {
  int i = 0, n = 0;
  va_list ap;
  va_copy(ap, p);
  while (fmt[i] != '\0') {
    if (fmt[i] == '%') {
      char fc = fmt[++i];
      int is_long = 0;
      if (fc == 'l') {
        is_long = 1;
        fc = fmt[i + 1];
      }
      if (fc == 's') {
        char *buf = va_arg(ap, char *);
        n += fn(buf, strlen(buf), fnd);
      } else if (strncmp(&fmt[i], ".*s", 3) == 0) {
        int len = va_arg(ap, int);
        char *buf = va_arg(ap, char *);
        n += fn(buf, len, fnd);
        i += 2;
      } else if (fc == 'd' || fc == 'u') {
        char buf[40];
        int len = is_long
                      ? snprintf(buf, sizeof(buf), fc == 'u' ? "%lu" : "%ld",
                                 va_arg(ap, long))
                      : snprintf(buf, sizeof(buf), fc == 'u' ? "%u" : "%d",
                                 va_arg(ap, int));
        n += fn(buf, len, fnd);
      } else if (fc == 'g' || fc == 'f') {
        char buf[40];
        int len = snprintf(buf, sizeof(buf), fc == 'g' ? "%g" : "%f",
                           va_arg(ap, double));
        n += fn(buf, len, fnd);
      } else if (fc == 'H') {
        const char *hex = "0123456789abcdef";
        int i, len = va_arg(ap, int);
        const unsigned char *p = va_arg(ap, const unsigned char *);
        n += fn("\"", 1, fnd);
        for (i = 0; i < len; i++) {
          n += fn(&hex[(p[i] >> 4) & 15], 1, fnd);
          n += fn(&hex[p[i] & 15], 1, fnd);
        }
        n += fn("\"", 1, fnd);
#if 0
      } else if (fc == 'M') {
        mjson_vprint_fn_t vfn = va_arg(ap, mjson_vprint_fn_t);
        n += vfn(fn, fndata, &ap);
#endif
      }
      i++;
    } else {
      n += fn(&fmt[i++], 1, fnd);
    }
  }
  va_end(p);
  va_end(ap);
  return n;

}

static int forth_printf(forth_print_fn_t fn, void *fnd, const char *fmt, ...) {
  va_list ap;
  int len;
  va_start(ap, fmt);
  len = forth_vprintf(fn, fnd, fmt, ap);
  va_end(ap);
  return len;
}

static void forth_execute_word(struct forth *f, char *buf, int len) {
  char *end = NULL;
  double dv = strtod(buf, &end);
  if (end == buf + len) {
    if (f->stacklen >= sizeof(f->stack) / sizeof(f->stack[0])) {
      f->stacklen = 0;
    }
    forth_printf(f->printfn, f->printfn_data, "NUMBER: [%.*s]\n", len, buf);
  } else {
    forth_printf(f->printfn, f->printfn_data, "WORD: [%.*s]\n", len, buf);
  }
}

static void forth_process_char(struct forth *f, int c) {
  int space = c == ' ' || c == '\r' || c == '\n' || c == '\t';
  if (f->buflen >= sizeof(f->buf)) f->buflen = 0;
  if (!f->inword && space) {
    // Ignore
  } else if (!f->inword && !space) {
    f->buf[f->buflen++] = c;
    f->inword = 1;
  } else if (f->inword && space) {
    f->buf[f->buflen] = '\0'; // NULL - terminate to allow C string functions
    forth_execute_word(f, f->buf, f->buflen);
    f->buflen = 0;
    f->inword = 0;
  } else {
    f->buf[f->buflen++] = c;
  }
}
