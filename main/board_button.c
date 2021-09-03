#include "board_button.h"
#include "esp_log.h"


#define GPIO_INPUT_IO_0    1
#define GPIO_INPUT_IO_1    2
#define GPIO_INPUT_IO_2    3
#define GPIO_INPUT_IO_3    6

static const char * TAG = "board_button";

static button_event_t g_buttons_event[BUTTON_NUM] = {0};
button_handle_t g_buttons[BUTTON_NUM] = {0};


static int get_btn_index(button_handle_t btn)
{
    for (size_t i = 0; i < BUTTON_NUM; i++) {
        if (btn == g_buttons[i]) {
            return i;
        }
    }
    return -1;
}

static void button_num_0_cb(void *arg)
{
    if (iot_button_get_event((button_handle_t)arg) == BUTTON_SINGLE_CLICK)
    {
        g_buttons_event[0] = BUTTON_SINGLE_CLICK;
        // ESP_LOGI(TAG, "BTN%d: BUTTON_SINGLE_CLICK", get_btn_index((button_handle_t)arg));
    }
    
}
static void button_num_1_cb(void *arg)
{
    if (iot_button_get_event((button_handle_t)arg) == BUTTON_SINGLE_CLICK)
    {
        g_buttons_event[1] = BUTTON_SINGLE_CLICK;
        // ESP_LOGI(TAG, "BTN%d: BUTTON_SINGLE_CLICK", get_btn_index((button_handle_t)arg));
    }
    else if (iot_button_get_event((button_handle_t)arg) == BUTTON_LONG_PRESS_HOLD)
    {
        g_buttons_event[1] = BUTTON_LONG_PRESS_HOLD;
        // ESP_LOGI(TAG, "BTN%d: BUTTON_LONG_PRESS_HOLD", get_btn_index((button_handle_t)arg));
    }
    
}
static void button_num_2_cb(void *arg)
{
    if (iot_button_get_event((button_handle_t)arg) == BUTTON_SINGLE_CLICK)
    {
        g_buttons_event[2] = BUTTON_SINGLE_CLICK;
        // ESP_LOGI(TAG, "BTN%d: BUTTON_SINGLE_CLICK", get_btn_index((button_handle_t)arg));
    }
    else if (iot_button_get_event((button_handle_t)arg) == BUTTON_LONG_PRESS_HOLD)
    {
        g_buttons_event[2] = BUTTON_LONG_PRESS_HOLD;
        // ESP_LOGI(TAG, "BTN%d: BUTTON_LONG_PRESS_HOLD", get_btn_index((button_handle_t)arg));
    }
    
}
static void button_num_3_cb(void *arg)
{
    if (iot_button_get_event((button_handle_t)arg) == BUTTON_SINGLE_CLICK)
    {
        g_buttons_event[3] = BUTTON_SINGLE_CLICK;
        // ESP_LOGI(TAG, "BTN%d: BUTTON_SINGLE_CLICK", get_btn_index((button_handle_t)arg));
    }
    
}
/**
 * @brief 独立按键初始化
 * 
 */
void board_button_init()
{
    int32_t button_gpio_port[BUTTON_NUM] = {GPIO_INPUT_IO_0, GPIO_INPUT_IO_1, GPIO_INPUT_IO_2, GPIO_INPUT_IO_3};
    // create gpio button
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = button_gpio_port[i],
            .active_level = 0,
            },
        };
        g_buttons[i] = iot_button_create(&gpio_btn_cfg);
        g_buttons_event[i] = BUTTON_NONE_PRESS;
        if(NULL == g_buttons[i]) 
            ESP_LOGE(TAG, "Button create failed g_buttons[%d]", i);
        else
            ESP_LOGI(TAG, "Button[%d] created", i);
    }

    iot_button_register_cb(g_buttons[0], BUTTON_SINGLE_CLICK, button_num_0_cb);

    iot_button_register_cb(g_buttons[1], BUTTON_SINGLE_CLICK, button_num_1_cb);
    iot_button_register_cb(g_buttons[1], BUTTON_LONG_PRESS_HOLD, button_num_1_cb);

    iot_button_register_cb(g_buttons[2], BUTTON_SINGLE_CLICK, button_num_2_cb);
    iot_button_register_cb(g_buttons[2], BUTTON_LONG_PRESS_HOLD, button_num_2_cb);

    iot_button_register_cb(g_buttons[3], BUTTON_SINGLE_CLICK, button_num_3_cb);
    

}

/**
 * @brief 读取当前是否有按键按下
 * 
 * @return true 按下了
 * @return false 
 */
bool board_button_is_pressed()
{
    if (gpio_get_level(GPIO_INPUT_IO_0) == 0 || 
        gpio_get_level(GPIO_INPUT_IO_1) == 0 || 
        gpio_get_level(GPIO_INPUT_IO_2) == 0 || 
        gpio_get_level(GPIO_INPUT_IO_3) == 0 
        )
        return true;
    else
        return false;

    
}

/**
 * @brief 获取按键事件
 * 
 * @param g_btns_event 接收按键事件的数组首地址
 */
void board_button_get_event(button_event_t *g_btns_event)
{
    if (g_btns_event == NULL)
        return;

    for (int i = 0; i < BUTTON_NUM; i++)
    {
        g_btns_event[i] = g_buttons_event[i];
    }
}

/**
 * @brief 每次进行按键相关操作后必须清除按键事件，否则会重复执行
 * 
 */
void board_button_clean_event()
{
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        g_buttons_event[i] = BUTTON_NONE_PRESS;
    }
}