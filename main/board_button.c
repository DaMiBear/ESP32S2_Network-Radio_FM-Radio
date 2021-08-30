#include "board_button.h"

#define GPIO_INPUT_IO_0    1
#define GPIO_INPUT_IO_1    2
#define GPIO_INPUT_IO_2    3
#define GPIO_INPUT_IO_3    6
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2) | (1ULL<<GPIO_INPUT_IO_3))

/**
 * @brief 独立按键初始化
 * 
 */
void board_button_init()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;   // 板子上已经上拉
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

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
 * @brief 获取当前按下的键值
 * 
 * @return uint32_t 键值
 */
uint32_t board_button_get_key()
{
    if (gpio_get_level(GPIO_INPUT_IO_0) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (gpio_get_level(GPIO_INPUT_IO_0) == 0)
        {
            while(gpio_get_level(GPIO_INPUT_IO_0) == 0);
            return 'q';
        }
    }
    else if (gpio_get_level(GPIO_INPUT_IO_1) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (gpio_get_level(GPIO_INPUT_IO_1) == 0)
        {
            while(gpio_get_level(GPIO_INPUT_IO_1) == 0);
            return 'w';
        }
    }
    else if (gpio_get_level(GPIO_INPUT_IO_2) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (gpio_get_level(GPIO_INPUT_IO_2) == 0)
        {
            while(gpio_get_level(GPIO_INPUT_IO_2) == 0);
            return 'e';
        }
    }
    else if (gpio_get_level(GPIO_INPUT_IO_3) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (gpio_get_level(GPIO_INPUT_IO_3) == 0)
        {
            while(gpio_get_level(GPIO_INPUT_IO_3) == 0);
            return 'r';
        }
    }
  
    return 0;
        
}