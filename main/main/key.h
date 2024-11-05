#pragma once 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

// 定义按键 GPIO 和长按时间阈值（毫秒）
#define BUTTON_GPIO_PIN   (gpio_num_t)42     // 按键 GPIO 引脚
#define DEBOUNCE_TIME     50    // 去抖时间 50ms

// 按键事件类型
typedef enum {
    BUTTON_PRESS,
    BUTTON_RELEASE,
} button_event_t;

// 按键状态结构体
typedef struct {
    int64_t press_time;   // 按键按下的时间戳
    bool is_pressed;      // 当前是否按下
} button_state_t;


void button_init();
void button_task(void *arg);