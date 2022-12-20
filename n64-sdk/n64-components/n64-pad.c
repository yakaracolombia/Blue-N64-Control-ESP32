#include "n64-pad.h"

int joystick_x_value = 0;
int joystick_y_value = 0;

void joystick_interrupt_handler(void *params)
{
    switch ((int)params)
    {
        default:
        case JOYSTICK_X_AXIS:
            if (gpio_get_level(JOYSTICK_X_INT_PIN) == gpio_get_level(JOYSTICK_X_Q_PIN))
            {
                joystick_x_value--;
            }
            else
            {
                joystick_x_value++;
            }

            //Capping X to -40 <= X <= 40
            if (joystick_x_value < 0 && joystick_x_value < -JOYSTICK_MAX_X)
            {
                joystick_x_value = -JOYSTICK_MAX_X;
            }
            else if (joystick_x_value > 0 && joystick_x_value > JOYSTICK_MAX_X)
            {
                joystick_x_value = JOYSTICK_MAX_X;   
            }
            break;
        case JOYSTICK_Y_AXIS:
            if (gpio_get_level(JOYSTICK_Y_INT_PIN) == gpio_get_level(JOYSTICK_Y_Q_PIN))
            {
                joystick_y_value--;
            }
            else
            {
                joystick_y_value++;
            }
            
            //Capping Y to -40 <= Y <= 40
            if (joystick_y_value < 0 && joystick_y_value < -JOYSTICK_MAX_Y)
            {
                joystick_y_value = -JOYSTICK_MAX_Y;
            }
            else if (joystick_y_value > 0 && joystick_y_value > JOYSTICK_MAX_Y)
            {
                joystick_y_value = JOYSTICK_MAX_Y;   
            }
            break;
    }
}

void n64_init(void)
{
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = BUTTONS_BIT_MASK;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = JOYSTICK_Q_BIT_MASK;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = JOYSTICK_INT_BIT_MASK;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(JOYSTICK_X_INT_PIN, joystick_interrupt_handler, (void *)JOYSTICK_X_AXIS));
    ESP_ERROR_CHECK(gpio_isr_handler_add(JOYSTICK_Y_INT_PIN, joystick_interrupt_handler, (void *)JOYSTICK_Y_AXIS));

    gpio_reset_pin(LED_MODE_PIN);
    gpio_set_direction(LED_MODE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_MODE_PIN, CONFIG_N64_MODE_IDENTIFIER == MODE_SWITCH);
}

int n64_get_joystick_x(void)
{
    return joystick_x_value;
}
int n64_get_joystick_y(void)
{
    return joystick_y_value;
}

uint32_t last_tick = 0;
void n64_handle_buttons(void)
{
    if (!gpio_get_level(TRIGGER_L_PIN) && !gpio_get_level(TRIGGER_R_PIN) && !gpio_get_level(BUTTON_START_PIN))
    {
        if (last_tick == 0)
        {
            last_tick = xTaskGetTickCount();
        }
        else if (xTaskGetTickCount() - last_tick >= CONFIG_N64_SECONDS_FOR_SWITCH_MODE * 1000)
        {
            last_tick = 0;
            n64_switch_mode();
        }
    }
    else
    {
        last_tick = 0;
    }
}

void n64_switch_mode(void)
{
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_APP_OTA_MIN + (CONFIG_N64_MODE_IDENTIFIER == 1 ? 2 : 1) - 1,
        NULL);
    printf("Switching to OTA 0x%02x\n", ESP_PARTITION_SUBTYPE_APP_OTA_MIN + (CONFIG_N64_MODE_IDENTIFIER == 1 ? 2 : 1) - 1);
    if (partition != NULL)
    {
        esp_err_t err = esp_ota_set_boot_partition(partition);
        if (err != ESP_OK)
        {
            printf("%s: esp_ota_set_boot_partition failed (%d).\n", __func__, err);
            abort();
            return;
        }
        esp_restart();
    }
    else
    {
        printf("%s: No partition found for %d.\n", __func__, (CONFIG_N64_MODE_IDENTIFIER == 1 ? 2 : 1));
    }
}