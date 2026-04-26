#ifndef CONSOLE_SERVICE_H
#define CONSOLE_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void console_service_init(void);
bool console_service_test(void);

int console_service_putchar(int ch);
int console_service_putchar_from_isr(int ch);
int console_service_puts(const char *str);
void console_service_write(const char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif