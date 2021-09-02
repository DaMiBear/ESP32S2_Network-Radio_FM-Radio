#pragma once 
#include "audio_element.h"

#define MAX_HLS_URL_NUM     (5)
typedef struct
{
    const char * hls_url;
    const char * program_name;
} HLS_INFO_t;



void wifi_connect(void);

/**
 * @brief 网络电台开始
 * 
 */
void play_living_stream_start(void);

/**
 * @brief 网络电台暂时结束，没有完全结束，仍占用部分资源
 * 
 */
void play_living_stream_end(void);

/**
 * @brief play_living_stream_end之后重新开始使用
 * 
 */
void play_living_stream_restart(void);