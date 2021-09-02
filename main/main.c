/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "screen_driver.h"
#include "esp_log.h"

#include "lvgl_gui.h"

#include "board_button.h"

#include "play_living_stream.h"

#include "get_time.h"
#include <time.h>
#include <sys/time.h>

#include "rda5807m_app.h"
static const char *TAG = "main";

extern lv_obj_t* data_time_label1;             // 当前时间标签
extern lv_obj_t* data_time_label2;

static void screen_clear(scr_driver_t *lcd, int color)
{
    scr_info_t lcd_info;
    lcd->get_info(&lcd_info);
    uint16_t *buffer = malloc(lcd_info.width * sizeof(uint16_t));
    if (NULL == buffer) {
        for (size_t y = 0; y < lcd_info.height; y++) {
            for (size_t x = 0; x < lcd_info.width; x++) {
                
                lcd->draw_pixel(x, y, color);
            }
        }
    } else {
        for (size_t i = 0; i < lcd_info.width; i++) {
            buffer[i] = color;
        }

        for (int y = 0; y < lcd_info.height / 8; y++) {
            lcd->draw_bitmap(0, y*8, lcd_info.width, 8, buffer);
        }
        
        free(buffer);
    }
}

//回调函数
static void state_task(void *pvParameters)
{
	// static char InfoBuffer[512] = {
    // 0};
	while (1)
	{
	    // vTaskList((char *) &InfoBuffer);
		// printf("任务名      任务状态 优先级   剩余栈 任务序号\r\n");
		// printf("\r\n%s\r\n", InfoBuffer);
        size_t free_size = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        // size_t free_size_IRAM = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT) 
        //                         - heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        
        ESP_LOGW(TAG, "DRAM free size is : %u", free_size);
        // ESP_LOGW(TAG, "IRAM free size is : %u", free_size_IRAM);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
static void time_task(void *pvParameters)
{
    time_t now = 0;
    struct tm timeinfo = { 0 };
    char strftime_buf[64];
    obtain_time();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);     // update 'now' variable with current time
        localtime_r(&now, &timeinfo);

        setenv("TZ", "CST-8", 1);
        tzset();
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d      %H:%M:%S", &timeinfo);
        if (data_time_label1 != NULL && data_time_label2 != NULL)
        {
            lv_label_set_text(data_time_label1, strftime_buf);
            lv_label_set_text(data_time_label2, strftime_buf);
        }
        ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
    }
    
}



void app_main(void)
{
    /* 开机默认网络电台输出 */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << 42) | (1ULL << 41);
    io_conf.pull_down_en = 0;   
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_42, 1);
    gpio_set_level(GPIO_NUM_41, 1);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
    ESP_ERROR_CHECK(esp_netif_init());
#else
    tcpip_adapter_init();
#endif

    /* ssd1306 LVGL初始化 */
    SSD1306_init();

    /* 加载动画显示 */
    my_lvgl_load_anim();

    wifi_connect();

    xTaskCreate(state_task, "state_task", 2048, NULL, 15, NULL);
    xTaskCreate(time_task, "time_task", 1024*2, NULL, 0, NULL);
    /* HLS初始化 */
    play_living_stream_start();
    
    /* 独立按键初始化 */
    board_button_init();
    
    /* FM模块rda5807初始化 */
    rda5807m_app_init();

    /* APP_GUI绘制 */
    my_lvgl_app();
    
    
}
