#pragma once
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/**
 * @brief 独立按键初始化
 * 
 */
void board_button_init(void);

/**
 * @brief 读取当前是否有按键按下
 * 
 * @return true 按下了
 * @return false 
 */
bool board_button_is_pressed(void);

/**
 * @brief 获取当前按下的键值
 * 
 * @return uint32_t 键值
 */
uint32_t board_button_get_key(void);

