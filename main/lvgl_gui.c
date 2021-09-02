// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
/* LVGL includes */
#include "lvgl_gui.h"

/* 独立按键includes */
#include "board_button.h"

/* ADF includes */
#include "audio_element.h"
#include "audio_pipeline.h"
#include "periph_wifi.h"
#include "get_time.h"

#include "play_living_stream.h"

#include "rda5807m_app.h"

static const char *TAG = "lvgl_gui";

typedef struct {
    scr_driver_t *lcd_drv;
    touch_panel_driver_t *touch_drv;
} lvgl_drv_t;

// wait for execute lv_task_handler and lv_tick_inc to avoid some widget don't refresh.
#define LVGL_TICK_MS 1

static void lv_tick_timercb(void *timer)
{
    /* Initialize a Timer for 1 ms period and
     * in its interrupt call
     * lv_tick_inc(1); */
    lv_tick_inc(LVGL_TICK_MS);
}

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
static SemaphoreHandle_t xGuiSemaphore = NULL;
static void gui_task(void *args)
{
    esp_err_t ret;
    lvgl_drv_t *lvgl_driver = (lvgl_drv_t *)args;

    /* Initialize LittlevGL */
    lv_init();

    /* Display interface */
    ret = lvgl_display_init(lvgl_driver->lcd_drv);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "lvgl display initialize failed");
        // lv_deinit(); /**< lvgl should be deinit here, but it seems lvgl doesn't support it */
        vTaskDelete(NULL);
    }

#if defined(CONFIG_LVGL_DRIVER_TOUCH_SCREEN_ENABLE)
    if (NULL != lvgl_driver->touch_drv) {
        /* Input device interface */
        ret = lvgl_indev_init(lvgl_driver->touch_drv);
        if (ESP_OK != ret) {
            ESP_LOGE(TAG, "lvgl indev initialize failed");
            // lv_deinit(); /**< lvgl should be deinit here, but it seems lvgl doesn't support it */
            vTaskDelete(NULL);
        }
    }
#endif

    esp_timer_create_args_t timer_conf = {
        .callback = lv_tick_timercb,
        .name     = "lv_tick_timer"
    };
    esp_timer_handle_t g_wifi_connect_timer = NULL;
    esp_timer_create(&timer_conf, &g_wifi_connect_timer);
    esp_timer_start_periodic(g_wifi_connect_timer, LVGL_TICK_MS * 1000U);

    xGuiSemaphore = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Start to run LVGL");

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
}

esp_err_t lvgl_init(scr_driver_t *lcd_drv, touch_panel_driver_t *touch_drv)
{
    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
      * Otherwise there can be problem such as memory corruption and so on.
      * NOTE: When not using Wi-Fi nor Bluetooth you can pin the gui_task to core 0 */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "GUI Run at %s Pinned to Core%d", CONFIG_IDF_TARGET, chip_info.cores - 1);

    static lvgl_drv_t lvgl_driver;
    lvgl_driver.lcd_drv = lcd_drv;
    lvgl_driver.touch_drv = touch_drv;

    xTaskCreatePinnedToCore(gui_task, "lv gui", 1024 * 4, &lvgl_driver, 0, NULL, chip_info.cores - 1);

    uint16_t timeout = 20;
    while (NULL == xGuiSemaphore) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
        if (0 == timeout) {
            ESP_LOGW(TAG, "GUI Task Start Timeout");
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

LV_FONT_DECLARE(lv_font_myfont_8)       // 额外字体来支持中文

static lv_obj_t* my_scr1 = NULL;            // 网络收音机屏幕
static lv_obj_t* my_scr2 = NULL;            // FM收音机模式
static lv_indev_t* my_indev = NULL;         // 设备对象
static lv_group_t* g = NULL;                // 组对象
static lv_obj_t* radio_label = NULL;        // 网络节目名称标签
static lv_obj_t* radio_info_label = NULL;   // 网络节目信息标签
static lv_obj_t* fre_label = NULL;          // 当前频段标签
static lv_obj_t* rssi_label = NULL;          // 当前rssi标签
lv_obj_t* data_time_label1 = NULL;          // 当前时间标签
lv_obj_t* data_time_label2 = NULL;

TaskHandle_t updata_rda5807m_info_task_handle = NULL;   // 更新RDA5807M信息任务句柄

extern audio_pipeline_handle_t pipeline;
extern audio_element_handle_t http_stream_reader, output_stream_writer, aac_decoder;
extern audio_event_iface_handle_t evt;
extern esp_periph_set_handle_t set;
extern HLS_INFO_t HLS_list[];
extern uint8_t HLS_list_index;

extern uint32_t rda5807m_current_fre;

static void updata_rda5807m_info_task(void *pvParameters)
{
    rda5807m_state_t state;
    
    while (1)
    {
        if (fre_label != NULL && rssi_label != NULL && lv_scr_act() == my_scr2)
        {
            // 在标签已经创建后，并且当前屏幕是FM的情况下才进行更新
            rda5807m_app_get_state(&state);     // 获得5807目前状态
            lv_label_set_text_fmt(fre_label, "%.1fMHz", ((float)(state.frequency)/1000));
            lv_label_set_text_fmt(rssi_label, "RSSI:%d", state.rssi);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
}

/**
 * @brief 读取键盘回调函数
 * 
 * @param indev_drv 
 * @param data 
 */
static void keypad_read_cb(struct _lv_indev_drv_t* indev_drv, lv_indev_data_t* data)
{
    if (board_button_is_pressed())
    {
        uint32_t ch = board_button_get_key();
        if (ch == 'q')
        {
            data->key = 'q';
            data->state = LV_INDEV_STATE_PRESSED;
            if (lv_scr_act() == my_scr1)
            {
                lv_scr_load_anim(my_scr2, LV_SCR_LOAD_ANIM_OVER_LEFT, 300, 100, false);
                lv_group_focus_obj(fre_label);     // 切换聚焦对象
                /* 停止living_stream */
                play_living_stream_end();
                gpio_set_level(GPIO_NUM_41, 1);     // 模拟开挂切换至FM输出
                gpio_set_level(GPIO_NUM_42, 0);
                // 创建FM屏幕上RDA5807M信息更新的任务
                xTaskCreate(updata_rda5807m_info_task, "updata_rda5807m_task", 2048, NULL, 0, &updata_rda5807m_info_task_handle);  
                configASSERT(updata_rda5807m_info_task_handle);
                printf("changed to scr2\n");
            }
            else if (lv_scr_act() == my_scr2)
            {
                if (updata_rda5807m_info_task_handle != NULL)
                    vTaskDelete(updata_rda5807m_info_task_handle);   // 删除FM屏幕上RDA5807M信息更新任务

                lv_scr_load_anim(my_scr1, LV_SCR_LOAD_ANIM_OVER_RIGHT, 300, 100, false);
                lv_group_focus_obj(radio_label);      // 切换聚焦对象
                /* 开始living_stream */
                play_living_stream_restart();
                gpio_set_level(GPIO_NUM_42, 1);     // 模拟开挂切换至网络电台输出
                gpio_set_level(GPIO_NUM_41, 1);
                printf("changed to scr1\n");
            }
            printf("Got 'q'\n");
        }
        else if (ch == 'w')
        {
            data->key = 'w';
            data->state = LV_INDEV_STATE_PRESSED;
            printf("Got 'w'\n");
        }
        else if (ch == 'e')
        {
            data->key = 'e';
            data->state = LV_INDEV_STATE_PRESSED;
            printf("Got 'e'\n");
        }
        else if (ch == 'r')
        {
            static uint8_t cnt = 0;
            data->key = 'r';      // 即使注释，也会触发LV_EVENT_KEY事件，只是获取不了键值
            data->state = LV_INDEV_STATE_PRESSED;
            gpio_set_level(GPIO_NUM_41, cnt % 2);   // 反转电平, 运放使能控制
            cnt++; 
            printf("Got 'r'\n");
        }
        
    }
    else
        data->state = LV_INDEV_STATE_RELEASED;
}

/**
 * @brief 网络电台label回调函数
 * 
 * @param e 
 */
static void radio_label_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);     // 获取触发事件的对象
    lv_event_code_t code = lv_event_get_code(e);
    printf("radio_label_event_cb\n");
    
    if (code == LV_EVENT_KEY)
    {
        uint32_t get_key = lv_indev_get_key(lv_indev_get_act());
        if (get_key == 'w' && my_scr1 == lv_scr_act())
        {
            // 当按下按键，且当前屏幕为my_scr1时，才能切换电台
            // 切换下一个电台URL  参考mp3_control
            HLS_list_index++;
            if (HLS_list_index == MAX_HLS_URL_NUM)
                HLS_list_index = 0;
            audio_pipeline_stop(pipeline);
            audio_pipeline_wait_for_stop(pipeline);
            audio_pipeline_terminate(pipeline);
            ESP_LOGW(TAG, "URL: %s", HLS_list[HLS_list_index].hls_url);
            audio_element_set_uri(http_stream_reader, HLS_list[HLS_list_index].hls_url);          
            audio_pipeline_reset_ringbuffer(pipeline);
            audio_pipeline_reset_elements(pipeline);
            audio_pipeline_run(pipeline);
           
            lv_label_set_text_fmt(obj, "%s", HLS_list[HLS_list_index].program_name);
        }
        else if (get_key == 'e' && my_scr1 == lv_scr_act())
        {
            // 切换上一个电台URL  参考mp3_control
            HLS_list_index--;
            if (HLS_list_index == 255)
                HLS_list_index = MAX_HLS_URL_NUM - 1;
            audio_pipeline_stop(pipeline);
            audio_pipeline_wait_for_stop(pipeline);
            audio_pipeline_terminate(pipeline);
            ESP_LOGW(TAG, "URL: %s", HLS_list[HLS_list_index].hls_url);
            audio_element_set_uri(http_stream_reader, HLS_list[HLS_list_index].hls_url);          
            audio_pipeline_reset_ringbuffer(pipeline);
            audio_pipeline_reset_elements(pipeline);
            audio_pipeline_run(pipeline);

            lv_label_set_text_fmt(obj, "%s", HLS_list[HLS_list_index].program_name);
        }
    }

}

static void fre_label_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);     // 获取触发事件的对象
    lv_event_code_t code = lv_event_get_code(e);
    printf("fre_label_event_cb\n");
    if (code == LV_EVENT_KEY)
    {
        uint32_t get_key = lv_indev_get_key(lv_indev_get_act());
        if (get_key == 'w' && my_scr2 == lv_scr_act())
        {
            // 当前为my_scr2时，才能切换频段
            rda5807m_app_add_frequency(100);
            lv_label_set_text_fmt(obj, "%.1fMHz", ((float)rda5807m_current_fre/1000));
        }
        else if (get_key == 'e' && my_scr2 == lv_scr_act())
        {
            rda5807m_app_reduce_frequency(100);
            lv_label_set_text_fmt(obj, "%.1fMHz", ((float)rda5807m_current_fre/1000));
        }
    }
}

static void internet_radio_scr_create()
{
    my_scr1 = lv_obj_create(NULL);    //创建屏幕

    /* 设置布局 */
    //lv_obj_set_layout(my_scr1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(my_scr1, LV_FLEX_FLOW_COLUMN);

    static lv_style_t row_container_style;
    lv_style_init(&row_container_style);
    lv_style_set_pad_row(&row_container_style, 0);  // 改变元素间距
    lv_obj_add_style(my_scr1, &row_container_style, 0);

    /*
     * 创建label
     */
    /* IP地址 */
    lv_obj_t* ip_address_label = lv_label_create(my_scr1); // 从当前活动屏幕创建
    //lv_obj_set_pos(ip_address_label, 0, 0);
    //lv_obj_set_size(ip_address_label, 128, 8);
    lv_label_set_text(ip_address_label, "192.168.101.100");
    /* 节目名称 */
    radio_label = lv_label_create(my_scr1);
    //lv_obj_set_pos(radio_label, 0, 9);
    lv_obj_set_width(radio_label, 128);
    // lv_label_set_long_mode(radio_label, LV_LABEL_LONG_SCROLL);       // 来回滑动显示
    lv_obj_set_style_text_font(radio_label, &lv_font_myfont_8, 0);      // 设置支持中文字体
    lv_label_set_text_fmt(radio_label, "%s", HLS_list[0].program_name);
    /* 节目信息 */
    radio_info_label = lv_label_create(my_scr1);
    lv_obj_set_width(radio_info_label, 128);
    lv_label_set_long_mode(radio_info_label, LV_LABEL_LONG_SCROLL);     // 来回滑动显示
    /* 当前日期时间 */
    data_time_label1 = lv_label_create(my_scr1);
    lv_obj_set_width(data_time_label1, 128);
    // lv_label_set_long_mode(data_time_label1, LV_LABEL_LONG_SCROLL);
    // get_current_time(current_time, sizeof(current_time));
    lv_label_set_text(data_time_label1, "current_time");

    lv_group_add_obj(g, radio_label);   // 控制的对象添加到组中
    lv_obj_add_event_cb(radio_label, radio_label_event_cb, LV_EVENT_KEY, NULL);     // 添加事件

}

static void FM_radio_scr_create()
{
    my_scr2 = lv_obj_create(NULL);    //创建屏幕

    /* 设置布局 */
    //lv_obj_set_layout(lv_scr_act(), LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(my_scr2, LV_FLEX_FLOW_COLUMN);

    static lv_style_t row_container_style;
    lv_style_init(&row_container_style);
    lv_style_set_pad_row(&row_container_style, 0);  // 改变元素间距
    lv_obj_add_style(my_scr2, &row_container_style, 0);

    /* 当前频段 */
    fre_label = lv_label_create(my_scr2);
    lv_obj_set_style_text_font(fre_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(fre_label, "%.1fMHz", ((float)rda5807m_current_fre/1000));
    lv_group_add_obj(g, fre_label);     // 添加到组中
    lv_obj_add_event_cb(fre_label, fre_label_event_cb, LV_EVENT_KEY, NULL);     // 配置事件

    /* 当前信号RSSI */
    rssi_label = lv_label_create(my_scr2);
    lv_obj_set_style_text_font(rssi_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(rssi_label, "RSSI:0");
    /* 当前日期时间 */
    data_time_label2 = lv_label_create(my_scr2);
    lv_obj_set_width(data_time_label2, 128);
    //lv_obj_set_pos(data_time_label2, 0, 50);
    lv_label_set_text(data_time_label2, "current_time");


}

void SSD1306_init()
{
    scr_driver_t g_lcd; // A screen driver
    esp_err_t ret = ESP_OK;

    spi_bus_handle_t bus_handle = NULL;
    spi_config_t bus_conf = {
        .miso_io_num = -1,      // 不用设置为-1
        .mosi_io_num = 35,      // LCD_SDA
        .sclk_io_num = 36,      // LCD_SCK
    }; // spi_bus configurations
    bus_handle = spi_bus_create(SPI2_HOST, &bus_conf);
    
    scr_interface_spi_config_t spi_ssd1306_cfg = {
        .spi_bus = bus_handle,    /*!< Handle of spi bus */
        .pin_num_cs = 38,           /*!< SPI Chip Select Pin 找个不用的引脚 */
        .pin_num_dc = 33,           /*!< Pin to select Data or Command for LCD */
        .clk_freq = 20*1000*1000,   /*!< SPI clock frequency */
        .swap_data = false,          /*!< Whether to swap data */
    };
    
    scr_interface_driver_t *iface_drv;
    scr_interface_create(SCREEN_IFACE_SPI, &spi_ssd1306_cfg, &iface_drv);
    /** Find screen driver for SSD1306 */
    ret = scr_find_driver(SCREEN_CONTROLLER_SSD1306, &g_lcd);
    if (ESP_OK != ret) {
        return;
        ESP_LOGE(TAG, "screen find failed");
    }
    
    /** Configure screen controller */
    scr_controller_config_t lcd_cfg = {
        .interface_drv = iface_drv,
        .pin_num_rst = 34,      // The reset pin is 34
        .pin_num_bckl = -1,     // The backlight pin is not connected
        .rst_active_level = 0,
        .bckl_active_level = 1,
        .offset_hor = 0,
        .offset_ver = 0,
        .width = 128,
        .height = 64,
        .rotate = SCR_DIR_RLBT,
    };

    /** Initialize SSD1306 screen */
    g_lcd.init(&lcd_cfg);

    lvgl_init(&g_lcd, NULL);    /* Initialize LittlevGL */
}

/**
 * @brief 更新标签内容 显示当前节目的名称、采样率等信息
 * 
 * @param music_info 
 */
void updata_radio_info_label(audio_element_info_t music_info)
{
    if (radio_info_label != NULL)
    {
        lv_label_set_text_fmt(radio_info_label, "Sample=%d, Bits=%d, Ch=%d", 
                          music_info.sample_rates, music_info.bits, music_info.channels);
    }
    
}

/**
 * @brief 开机加载动画显示
 * 
 */
void my_lvgl_load_anim()
{
    lv_obj_t* load_scr = lv_obj_create(NULL);    //创建屏幕
    lv_scr_load(load_scr);
    lv_obj_t* symbol_label = lv_label_create(load_scr);
    lv_obj_set_style_text_font(symbol_label, &lv_font_montserrat_20, 0);
    lv_obj_align(symbol_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(symbol_label, LV_SYMBOL_LOOP"Loading...");
}

/**
 * @brief LVGL GUI程序
 * 
 */
void my_lvgl_app()
{
    /* 登记输入设备 */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read_cb;  // 注册读取键盘状态的回调函数
    my_indev = lv_indev_drv_register(&indev_drv);   // 注册输入设备
    g = lv_group_create();  // 创建组
    
    internet_radio_scr_create();
    FM_radio_scr_create();

    lv_scr_load(my_scr1);

  /*  lv_group_add_obj(g, my_scr1);
    lv_group_add_obj(g, my_scr2);*/
    lv_indev_set_group(my_indev, g);    // 把组分配给输入设备
}
