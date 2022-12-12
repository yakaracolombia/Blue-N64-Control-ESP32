#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

void app_main(void)
{
    printf("Blue-N64 Control. HEAP=%#010lx\n", esp_get_free_heap_size());

    while(1)
    {
        vTaskDelay(1);
    }
}