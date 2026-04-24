#include "bsp_flash.h"

#include "main.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define BSP_FLASH_BASE_ADDRESS 0x08000000u
#define BSP_FLASH_TOTAL_SIZE_BYTES (512u * 1024u)
#define BSP_FLASH_END_ADDRESS (BSP_FLASH_BASE_ADDRESS + BSP_FLASH_TOTAL_SIZE_BYTES)
#define BSP_FLASH_BOOT_RESERVED_SIZE_BYTES (16u * 1024u)
#define BSP_FLASH_CONFIG_RESERVED_SIZE_BYTES (16u * 1024u)
#define BSP_FLASH_WRITABLE_START_ADDRESS \
    (BSP_FLASH_BASE_ADDRESS + BSP_FLASH_BOOT_RESERVED_SIZE_BYTES + BSP_FLASH_CONFIG_RESERVED_SIZE_BYTES)
#define BSP_FLASH_WORD_SIZE_BYTES 4u

typedef struct
{
    uint32_t start_address;
    uint32_t end_address;
    uint32_t sector;
} bsp_flash_sector_t;

static const bsp_flash_sector_t flash_sectors[] = {
    { 0x08000000u, 0x08004000u, FLASH_SECTOR_0 },
    { 0x08004000u, 0x08008000u, FLASH_SECTOR_1 },
    { 0x08008000u, 0x0800C000u, FLASH_SECTOR_2 },
    { 0x0800C000u, 0x08010000u, FLASH_SECTOR_3 },
    { 0x08010000u, 0x08020000u, FLASH_SECTOR_4 },
    { 0x08020000u, 0x08040000u, FLASH_SECTOR_5 },
    { 0x08040000u, 0x08060000u, FLASH_SECTOR_6 },
    { 0x08060000u, 0x08080000u, FLASH_SECTOR_7 },
};

static bool flash_initialized = false;

static bool bsp_flash_is_aligned(uint32_t address, size_t length)
{
    return ((address % BSP_FLASH_WORD_SIZE_BYTES) == 0u) &&
           ((length % BSP_FLASH_WORD_SIZE_BYTES) == 0u);
}

static bool bsp_flash_get_range_end(uint32_t address, size_t length, uint32_t *end_address)
{
    uint32_t offset = 0u;

    if (end_address == NULL || length == 0u)
    {
        return false;
    }

    offset = (uint32_t)length;

    if (offset > (UINT32_MAX - address))
    {
        return false;
    }

    *end_address = address + offset;
    return true;
}

static const bsp_flash_sector_t *bsp_flash_find_sector(uint32_t address)
{
    size_t index = 0u;

    for (; index < (sizeof(flash_sectors) / sizeof(flash_sectors[0])); ++index)
    {
        if (address >= flash_sectors[index].start_address && address < flash_sectors[index].end_address)
        {
            return &flash_sectors[index];
        }
    }

    return NULL;
}

static bool bsp_flash_is_access_allowed(uint32_t address, size_t length)
{
    uint32_t end_address = 0u;

    if (!bsp_flash_get_range_end(address, length, &end_address))
    {
        return false;
    }

    if (address < BSP_FLASH_WRITABLE_START_ADDRESS)
    {
        return false;
    }

    if (end_address > BSP_FLASH_END_ADDRESS)
    {
        return false;
    }

    return true;
}

static bool bsp_flash_unlock(void)
{
    return HAL_FLASH_Unlock() == HAL_OK;
}

static void bsp_flash_lock(void)
{
    (void)HAL_FLASH_Lock();
}

void bsp_flash_init(void)
{
    flash_initialized = true;
}

bool bsp_flash_is_range_valid(uint32_t address, size_t length)
{
    if (!flash_initialized)
    {
        return false;
    }

    if (length == 0u)
    {
        return false;
    }

    if (!bsp_flash_is_aligned(address, length))
    {
        return false;
    }

    return bsp_flash_is_access_allowed(address, length);
}

bool bsp_flash_read(uint32_t address, void *buffer, size_t length)
{
    if (buffer == NULL)
    {
        return false;
    }

    if (!bsp_flash_is_range_valid(address, length))
    {
        return false;
    }

    memcpy(buffer, (const void *)address, length);
    return true;
}

bool bsp_flash_erase(uint32_t address, size_t length)
{
    const bsp_flash_sector_t *start_sector = NULL;
    const bsp_flash_sector_t *end_sector = NULL;
    FLASH_EraseInitTypeDef erase_init = { 0 };
    uint32_t end_address = 0u;
    uint32_t sector_error = 0u;

    if (!bsp_flash_is_range_valid(address, length))
    {
        return false;
    }

    if (!bsp_flash_get_range_end(address, length, &end_address))
    {
        return false;
    }

    start_sector = bsp_flash_find_sector(address);
    end_sector = bsp_flash_find_sector(end_address - 1u);

    if (start_sector == NULL || end_sector == NULL)
    {
        return false;
    }

    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_init.Sector = start_sector->sector;
    erase_init.NbSectors = (end_sector->sector - start_sector->sector) + 1u;

    if (!bsp_flash_unlock())
    {
        return false;
    }

    if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK)
    {
        bsp_flash_lock();
        return false;
    }

    bsp_flash_lock();
    return true;
}

bool bsp_flash_write(uint32_t address, const void *data, size_t length)
{
    const uint8_t *input = (const uint8_t *)data;
    size_t offset = 0u;
    uint32_t word = 0u;

    if (input == NULL)
    {
        return false;
    }

    if (!bsp_flash_is_range_valid(address, length))
    {
        return false;
    }

    if (!bsp_flash_unlock())
    {
        return false;
    }

    for (; offset < length; offset += BSP_FLASH_WORD_SIZE_BYTES)
    {
        memcpy(&word, &input[offset], sizeof(word));

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + (uint32_t)offset, word) != HAL_OK)
        {
            bsp_flash_lock();
            return false;
        }
    }

    bsp_flash_lock();
    return true;
}
