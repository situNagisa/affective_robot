#include <cstdint>
#include <cstddef>
#include <memory>
#include <bit>
#include <cassert>
#include <new>
#include <cstdio>
#include <array>

#include "sdkconfig.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>

#include "./http_client.h"
#include "./recorder.h"
#include "./wav.h"
#include "./client.h"
#include "./play.h"

inline static constexpr auto record_time = 4u;
static ::std::uint8_t* wav = nullptr;
static ::std::size_t wav_size = 0;

extern "C" esp_err_t client_initialize()
{
	wav_size = ::recorder_data_size_requires(record_time) + sizeof(::wav_header);
	ESP_LOGI("client", "wav_size = %u largest free block = %u", wav_size, ::heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
	if (wav_size > ::heap_caps_get_largest_free_block(MALLOC_CAP_8BIT))
	{
		wav_size = 0;
		ESP_LOGE("main", "insufficient memory to record 5 seconds");
		return ESP_FAIL;
	}
	wav = static_cast<::std::uint8_t*>(::heap_caps_malloc(wav_size, MALLOC_CAP_8BIT));
	return ESP_OK;
}

extern "C" void client_deinitialize()
{
	::heap_caps_free(wav);
}

extern "C" void client_print_wav()
{
	auto header = ::recorder_wav_header(::recorder_data_size_requires(record_time));
	for (auto i : ::std::bit_cast<::std::array<::std::byte, sizeof(::wav_header)>>(header))
	{
		::std::printf("0x%02x,", static_cast<::std::uint8_t>(i));
	}
	for (auto i = 0u; i < ::recorder_data_size_requires(record_time); i++)
	{
		::std::printf("0x%02x,", wav[i]);
	}
}

static char filename[256]{};

extern "C" void client_send_wav()
{
	auto record_size = ::recorder_record(wav, record_time);
	
	::std::size_t seconds = record_time;
	auto header = ::recorder_wav_header(::recorder_data_size_requires(seconds));

	auto client = ::http_client_start_send_wav();
	if (auto err = ::http_client_send_wav_header(client, &header); err != ESP_OK)
	{
		ESP_LOGE("main", "Failed to send wav header");
		::esp_http_client_cleanup(client);
		::std::abort();
	}

	for (auto i = 0u; i < seconds / record_time; i++)
	{
		auto start = ::esp_timer_get_time();
		if (auto err = ::http_client_send_wav_data_stream(client, wav, record_size); err != ESP_OK) [[unlikely]]
		{
			ESP_LOGE("main", "Failed to send wav data stream");
			::esp_http_client_cleanup(client);
			::std::abort();
		}
		ESP_LOGI("main", "send %u bytes in %lld ms", record_size, (::esp_timer_get_time() - start) / 1'000u);
	}

	if (auto err = ::http_client_end_send_wav(client, filename, ::std::ranges::size(filename)); err != ESP_OK)
	{
		ESP_LOGE("main", "Failed to end send wav");
		::esp_http_client_cleanup(client);
		::std::abort();
	}
}

extern "C" void client_receive_wav()
{
	::std::int64_t content_length;
	ESP_LOGI("client", "start receive wav %s", filename);
	auto client = ::http_client_start_receive_wav(filename, &content_length);
	if (content_length < 0)
	{
		ESP_LOGE("main", "Failed to start receive wav");
		::esp_http_client_cleanup(client);
		::std::abort();
	}
	ESP_LOGI("main", "content_length = %lld", content_length);
	if (content_length > wav_size)
	{
		ESP_LOGE("main", "insufficient memory to receive wav");
		::std::abort();
	}
	auto data_size = ::http_client_receive_wav(client, wav, content_length);
	ESP_LOGI("main", "receive %u bytes", data_size);

	i2s_play(wav, data_size);
	//::vTaskDelay(record_time * 1000 / portTICK_PERIOD_MS);
}