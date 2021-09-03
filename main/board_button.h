#pragma once
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"

#define BUTTON_NUM  4
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
 */
void board_button_get_event(button_event_t *g_buttons_event);

/**
 * @brief 每次进行按键相关操作后必须清除按键事件，否则会重复执行
 * 
 */
void board_button_clean_event();
