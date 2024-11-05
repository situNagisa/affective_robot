#include "key.h"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <bit>
#include <cassert>
#include <new>

#include "sdkconfig.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_psram.h>

#include "play.h"
#include "./client.h"

static const char *TAG = "ButtonFramework";
// 定义按键事件队列
static QueueHandle_t button_event_queue = NULL;
button_state_t button_state = {0, false};


// 中断处理函数，处理按键状态变化
static void IRAM_ATTR button_isr_handler(void *arg) {
    button_event_t event = gpio_get_level(BUTTON_GPIO_PIN) == 1 ? BUTTON_PRESS : BUTTON_RELEASE;
    xQueueSendFromISR(button_event_queue, &event, NULL);
}

// 按键初始化函数
void button_init() {
    // 配置 GPIO 为输入模式并启用中断
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // 创建按键事件队列
    button_event_queue = xQueueCreate(10, sizeof(button_event_t));

    // 注册中断服务并设置中断处理函数
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO_PIN, button_isr_handler, NULL);
}

// 按键处理任务
void button_task(void *arg) {
    button_event_t event;
    while (true) {
        // 等待按键事件
        if (xQueueReceive(button_event_queue, &event, portMAX_DELAY)) {
            if (event == BUTTON_PRESS) {
                client_send_wav();
            } 
            // 去抖延迟
            vTaskDelay(DEBOUNCE_TIME / portTICK_PERIOD_MS);
        }
    }
}