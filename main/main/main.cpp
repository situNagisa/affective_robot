
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

#include "./http_client.h"
#include "./recorder.h"
#include "./client.h"
#include "./play.h"
#include "./key.h"


extern "C" void app_main(void)
{
	ESP_ERROR_CHECK(::esp_task_wdt_deinit());

	ESP_ERROR_CHECK(::http_client_initialize());
	ESP_ERROR_CHECK(::recorder_initialize());
	ESP_ERROR_CHECK(::client_initialize());

	client_send_wav();

	http_client_deinitialize();

	::vTaskDelay(3 / portTICK_PERIOD_MS);

	client_print_wav();

	return;

	button_init();
	i2sInitOutput();
	//::xTaskCreate(button_task, "button_task", 4 * 1024, NULL, 10, NULL);
	button_task(nullptr);
	
	while (true);

	::client_deinitialize();
}
