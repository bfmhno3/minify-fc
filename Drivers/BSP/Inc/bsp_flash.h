#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bsp_flash_init(void);
bool bsp_flash_is_range_valid(uint32_t address, size_t length);
bool bsp_flash_read(uint32_t address, void *buffer, size_t length);
bool bsp_flash_erase(uint32_t address, size_t length);
bool bsp_flash_write(uint32_t address, const void *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
