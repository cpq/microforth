// Copyright (c) 2018,2019,2020 Cesanta Software Limited
// All rights reserved

#include "../microforth.h"
#include "stm32f1.h"

static int blink_period = 150000;

static int uart_write(const char *ptr, int size, void *userdata) {
  struct uart *uart = userdata;
  for (int i = 0; i < size; i++) {
    uart->DR = ((unsigned char *) ptr)[i];  // Write next byte
    while ((uart->SR & BIT(7)) == 0)        // Wait until byte is scheduled
      ;
  }
  return size;
}

void *_sbrk() {
  return 0;
}

static struct forth f = {.printfn = uart_write, .printfn_data = UART1};

int main(void) {
  INIT_MEMORY;

  RCC->APB2ENR |= BIT(4);                // GPIOC, for LED on PC13
  SET_PIN_MODE(GPIOC, LED_PIN, 0b0110);  // open drain, output 2

  RCC->APB2ENR |= BIT(2);   // GPIOA, for USART1 TX/RX on PA9 and PA10
  RCC->APB2ENR |= BIT(14);  // enable clock on USART1
  SET_PIN_MODE(GPIOA, TX_PIN, 0b1010);     // TX pin mode = AF, push/pull
  SET_PIN_MODE(GPIOA, RX_PIN, 0b0100);     // RX pin mode = input, floating
  UART1->BRR = 0x45;                       // Set baud rate, TRM 27.3.4
  UART1->CR1 = BIT(13) | BIT(2) | BIT(3);  // Enable USART1

  volatile int count = 0, led_on = 0;
  forth_register_core_words(&f);
  for (;;) {
    if (UART_HAS_DATA(UART1)) {
      // char c = UART_READ(UART1);
      // uart_write(&c, 1, UART1);
      forth_process_char(&f, UART_READ(UART1));
    }
    if (++count < blink_period) continue;
    count = 0;
    led_on = !led_on;
    GPIOC->BSRR |= BIT(LED_PIN + (led_on ? 0 : 16));
  }
  return 0;
}
