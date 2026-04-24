#include "app/app_boot.h"

#include "bsp_led.h"
#include "bsp_module.h"
#include "bsp_sensors.h"

void app_boot_init(void)
{
    bsp_led_init();
    bsp_module_init();
    bsp_sensors_init();
}
