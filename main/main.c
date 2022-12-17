#include "main.h"

#define ADC_STICK_X ADC1_CHANNEL_0
#define ADC_STICK_Y ADC1_CHANNEL_3

void button_task(hoja_button_data_s *button_data)
{
    // Buttons
    //   A
    button_data->button_right       |= !gpio_get_level(BUTTON_A_PIN);
    //   B
    button_data->button_down        |= !gpio_get_level(BUTTON_B_PIN);

    // Dpad
    button_data->dpad_up            |= !gpio_get_level(DPAD_UP_PIN);
    button_data->dpad_down          |= !gpio_get_level(DPAD_DOWN_PIN);
    button_data->dpad_left          |= !gpio_get_level(DPAD_LEFT_PIN);
    button_data->dpad_right         |= !gpio_get_level(DPAD_RIGHT_PIN);
    
    // Triggers
    button_data->trigger_l          |= !gpio_get_level(TRIGGER_L_PIN);
    button_data->trigger_r          |= !gpio_get_level(TRIGGER_R_PIN);
    button_data->trigger_zr         |= !gpio_get_level(TRIGGER_Z_PIN);

    // MISC
    button_data->button_start       |= !gpio_get_level(BUTTON_START_PIN);

    //NC
    button_data->button_up          = 0;
    button_data->button_left        = 0;
    button_data->trigger_zl         = 0;
    button_data->button_select      = 0;
    button_data->button_stick_left  = 0;
    button_data->button_stick_right = 0;
}

void event_task(hoja_event_type_t type, uint8_t evt, uint8_t param)
{
    printf("Blue-N64 Control Event: \n\ttype: %d\n\tevent: %d\n\tparam: %d\n", type, evt, param);
    if (type == HOJA_EVT_BT && evt == HEVT_BT_DISCONNECT)
    {
        while (true)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            if (!gpio_get_level(BUTTON_START_PIN))
            {
                while(hoja_start_core() != HOJA_OK)
                {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    printf("Blue-N64 Control. Retrying...\n");
                }
                printf("Blue-N64 Control. Connected!\n");
                break;
            }
        }
    }
}

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

// Separate task to read sticks.
// This is essential to have as a separate component as ADC scans typically take more time and this is only
// scanned once between each polling interval. This varies from core to core.
void stick_task(hoja_analog_data_s* analog_data)
{
    // Joystick
    analog_data->ls_x = (joystick_x_value + JOYSTICK_MAX_X) * JOYSTICK_ABS_MAX / JOYSTICK_MAX_X;
    analog_data->ls_y = (joystick_y_value + JOYSTICK_MAX_Y) * JOYSTICK_ABS_MAX / JOYSTICK_MAX_Y;

    // C Buttons
    //   Y (Up/Down)
    if (!gpio_get_level(C_UP_PIN))
    {
        analog_data->rs_y = JOYSTICK_ABS_MAX;
        
    }
    else if (!gpio_get_level(C_DOWN_PIN))
    {
        analog_data->rs_y = 0;
    }
    else
    {
        analog_data->rs_y = JOYSTICK_ABS_MAX / 2;
    }
    //   X (Left/Right)
    if (!gpio_get_level(C_LEFT_PIN))
    {
        analog_data->rs_x = JOYSTICK_ABS_MAX;
        
    }
    else if (!gpio_get_level(C_RIGHT_PIN))
    {
        analog_data->rs_x = 0;
    }
    else
    {
        analog_data->rs_x = JOYSTICK_ABS_MAX / 2;
    }

    return;
}

void app_main(void)
{
    printf("Blue-N64 Control. HEAP=%#010lx\n", esp_get_free_heap_size());

    gpio_config_t io_conf = {};

    // Set up ADC
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_STICK_X, ADC_ATTEN_DB_11));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_STICK_Y, ADC_ATTEN_DB_11));

    hoja_register_button_callback(button_task);
    hoja_register_analog_callback(stick_task);
    hoja_register_event_callback(event_task);

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

    hoja_init();
    hoja_set_core(HOJA_CORE_NS);

    while(hoja_start_core() != HOJA_OK)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        printf("Blue-N64 Control. Retrying...\n");
    }
    printf("Blue-N64 Control. Connected!\n");
}