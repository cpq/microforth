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

#ifndef MICROFORTH_MEM_SIZE
#define MICROFORTH_MEM_SIZE 256
#endif

struct forth;
typedef int (*forth_print_fn_t)(const char *buf, int len, void *userdata);

struct forth {
  char buf[MICROFORTH_MAX_WORD_LEN];  // Input buffer
  int buflen;                         // Input buffer length
  double stack[MICROFORTH_STACK_SIZE]; // Stack
  int stacklen;                        // Current stack length
  unsigned char mem[MICROFORTH_MEM_SIZE];  // Memory buffer - word definitions
  int memlen;                              // Used memory
  unsigned char state;                 // VM state
#define MICROFORTH_STATE_IN_WORD 1
#define MICROFORTH_STATE_IN_DEFINITION 2
#define MICROFORTH_STATE_DEFINITION_WORD_SCANNED 4
  forth_print_fn_t printfn;            // Printing function
  void *printfn_data;                  // Printing function data
};

static void forth_add_word(struct forth *f, const char *name, char *code,
                           void (*fn)(struct forth *)) {
  unsigned char *mem = f->mem + f->memlen;
  int namelen = strlen(name);
  int codelen = code == NULL ? 0 : strlen(code);
  int flen = code == NULL ? sizeof(fn) : 0;
  mem[0] = namelen + 1 + codelen + 1 + flen + 1;
  memcpy(mem + 1, name, namelen + 1);
  mem[1 + namelen + 1] = '\0';
  if (code != NULL) memcpy(mem + 1 + namelen + 1, code, codelen + 1);
  if (flen > 0) memcpy(mem + 1 + namelen + 1 + codelen + 1, &fn, flen);
  // printf("ADDED WORD [%s] cl %d flen %d fn %p total len %d\n", &mem[1],
  // codelen, flen, fn, (int) mem[0]);
  f->memlen += mem[0];
}

static int forth_find_word(struct forth *f, const char *name) {
  int n;
  for (n = 0; n < f->memlen; n += f->mem[n]) {
    // printf("%d %d [%s]\n", n, f->memlen, &f->mem[n + 1]);
    if (strcmp((char *) &f->mem[n + 1], name) == 0) return n;
  }
  return -1;
}

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
        // long val = is_long ? va_arg(ap, long) : va_arg(ap, int);
        int len = is_long
                      ? snprintf(buf, sizeof(buf), fc == 'u' ? "%lu" : "%ld",
                                 va_arg(ap, long))
                      : snprintf(buf, sizeof(buf), fc == 'u' ? "%u" : "%d",
                                 va_arg(ap, int));
        n += fn(buf, len, fnd);
      } else if (fc == 'g' || fc == 'f') {
        char buf[40];
        double val = va_arg(ap, double);
        int len = snprintf(buf, sizeof(buf), fc == 'g' ? "%g" : "%f", val);
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
    if (f->stacklen >= (int) (sizeof(f->stack) / sizeof(f->stack[0]))) {
      f->stacklen = 0;
    }
    f->stack[f->stacklen++] = dv;
    forth_printf(f->printfn, f->printfn_data, "%g ok\n", dv);
  } else {
    int n = forth_find_word(f, buf);
    if (n < 0) {
      forth_printf(f->printfn, f->printfn_data, "%.*s error\n", len, buf);
    } else {
      int off = n + 1 + strlen((char *) &f->mem[n + 1]) + 1;
      if (f->mem[off] == 0) {
        void (*fn)(struct forth *);
        memcpy(&fn, &f->mem[off + 1], sizeof(fn));
        // printf("EXECING FN %p\n", fn);
        fn(f);
      }
    }
  }
}

static void forth_word_plus(struct forth *f) {
  if (f->stacklen < 2) {
    forth_printf(f->printfn, f->printfn_data, "%s: stack undeflow\n", __func__);
  } else {
    f->stack[f->stacklen - 2] =
        f->stack[f->stacklen - 2] + f->stack[f->stacklen - 1];
    f->stacklen--;
    forth_printf(f->printfn, f->printfn_data, "%g ok\n",
                 f->stack[f->stacklen - 1]);
  }
}

static void forth_word_words(struct forth *f) {
  int n;
  for (n = 0; n < f->memlen; n += f->mem[n]) {
    const char *delim = n == 0 ? "" : " ";
    forth_printf(f->printfn, f->printfn_data, "%s%s", delim, &f->mem[n + 1]);
  }
  forth_printf(f->printfn, f->printfn_data, " ok\n"); 
}

static void forth_word_l32(struct forth *f) {
  if (f->stacklen < 1) {
  } else {
    unsigned long val, addr = f->stack[f->stacklen - 1];
    memcpy(&val, (void *) addr, sizeof(val));
    f->stack[f->stacklen++] = val;
    forth_printf(f->printfn, f->printfn_data, "%#x ok\n", val);
  }
}

void forth_register_core_words(struct forth *f) {
  forth_add_word(f, "words", NULL, forth_word_words);
  forth_add_word(f, "+", NULL, forth_word_plus);
  forth_add_word(f, "l32", NULL, forth_word_l32);
}

void forth_process_char(struct forth *f, int c) {
  int space = c == ' ' || c == '\r' || c == '\n' || c == '\t';
  int inword = f->state & MICROFORTH_STATE_IN_WORD;
  if (f->buflen >= (int) sizeof(f->buf)) f->buflen = 0;
  if (!inword && space) {
    // Ignore
  } else if (!inword && !space) {
    f->buf[f->buflen++] = c;
    f->state |= MICROFORTH_STATE_IN_WORD;
  } else if (inword && space) {
    f->buf[f->buflen] = '\0'; // NULL - terminate to allow C string functions
    forth_execute_word(f, f->buf, f->buflen);
    f->buflen = 0;
    f->state &= ~MICROFORTH_STATE_IN_WORD;
  } else {
    f->buf[f->buflen++] = c;
  }
}
