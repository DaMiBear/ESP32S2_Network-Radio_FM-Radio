#pragma once

#include "rda5807m.h"


/**
 * @brief 获取5807目前状态，给其他文件使用
 * 
 * @param state 要写入的状态变量
 */
void rda5807m_app_get_state(rda5807m_state_t * state);

/**
 * @brief rda5807初始化
 * 
 */
void rda5807m_app_init();

/**
 * @brief 频率增加100KHz
 * 
 */
void rda5807m_app_add_frequency(uint32_t delta);

/**
 * @brief 频率减少100KHz
 * 
 */
void rda5807m_app_reduce_frequency(uint32_t delta);