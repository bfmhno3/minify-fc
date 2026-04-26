#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include <stdbool.h>
#include "config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void config_service_init(void);
bool config_service_test(void);
bool config_service_is_flying(void);
void config_service_set_flying(bool flying);

const config_param_t* config_service_get(void);
config_param_t* config_service_mut(void);
void config_service_mark_dirty(void);
void config_service_reset_pid(void);

void config_service_task(void* arg);

#ifdef __cplusplus
}
#endif

#endif