
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
#include "./wav.h"

void send_wav(::std::size_t seconds)
{
	constexpr auto record_time_uint = static_cast<::std::size_t>(3u);
	auto data_size = ::recorder_data_size_requires(record_time_uint);
	if (data_size > ::heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT))
	{
		ESP_LOGE("main", "Insufficient memory to record %u seconds", record_time_uint);
		::std::abort();
	}
	ESP_LOGI("main", "8 bits free size: %u", ::heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
	ESP_LOGI("main", "spi ram free size: %u", ::heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
	auto data = static_cast<::std::uint8_t*>(::heap_caps_malloc(data_size, MALLOC_CAP_8BIT));
	if (data == nullptr)
	{
		ESP_LOGE("main", "Memory allocation failed!");
		::std::abort();
	}

	auto header = ::recorder_wav_header(::recorder_data_size_requires(seconds));

	auto client = ::http_client_start_send_wav();
	if (auto err = ::http_client_send_wav_header(client, &header); err != ESP_OK)
	{
		ESP_LOGE("main", "Failed to send wav header");
		::esp_http_client_cleanup(client);
		::heap_caps_free(data);
		::std::abort();
	}
	
	for (auto i = 0u; i < seconds / record_time_uint; i++)
	{
		auto record_size = ::recorder_record(data, record_time_uint);
		auto start = ::esp_timer_get_time();
		if (auto err = ::http_client_send_wav_data_stream(client, data, record_size); err != ESP_OK) [[unlikely]]
		{
			ESP_LOGE("main", "Failed to send wav data stream");
			::esp_http_client_cleanup(client);
			::heap_caps_free(data);
			::std::abort();
		}
		ESP_LOGI("main", "send %u bytes in %lld ms", record_size, (::esp_timer_get_time() - start) / 1'000u);
	}
	
	if (auto err = ::http_client_end_send_wav(client); err != ESP_OK)
	{
		ESP_LOGE("main", "Failed to end send wav");
		::esp_http_client_cleanup(client);
		::heap_caps_free(data);
		::std::abort();
	}
	::heap_caps_free(data);
}

extern "C" void app_main(void)
{
	ESP_ERROR_CHECK(::esp_task_wdt_deinit());

	ESP_ERROR_CHECK(::http_client_initialize());
	ESP_ERROR_CHECK(::recorder_initialize());

	::send_wav(3);
	{
		::std::int64_t content_length;
		auto client = ::http_client_start_receive_wav("6968be28-1bbc-4646-a194-af4f49f125d1.wav", &content_length);
		if (content_length < 0)
		{
			ESP_LOGE("main", "Failed to start receive wav");
			::esp_http_client_cleanup(client);
			::std::abort();
		}
		ESP_LOGI("main", "content_length = %lld", content_length);
		if (content_length > ::heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT))
		{
			ESP_LOGE("main", "Insufficient memory to receive wav");
			::std::abort();
		}
		auto data = static_cast<::std::uint8_t*>(::heap_caps_malloc(content_length, MALLOC_CAP_8BIT));
		if (data == nullptr)
		{
			ESP_LOGE("main", "Memory allocation failed!");
			::std::abort();
		}
		auto data_size = ::http_client_receive_wav(client, data, content_length);
		ESP_LOGI("main", "receive %u bytes", data_size);
	}

	while (true);

}
