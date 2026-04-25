#ifndef PLATFORM_INIT_H
#define PLATFORM_INIT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void platform_init(void);
bool platform_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
