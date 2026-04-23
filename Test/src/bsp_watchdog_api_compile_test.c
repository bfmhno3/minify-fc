#include "bsp_watchdog.h"

int main(void)
{
    bsp_watchdog_init(150u);
    bsp_watchdog_kick();
    return 0;
}
