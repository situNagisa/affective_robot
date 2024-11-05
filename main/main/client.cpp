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

inline static constexpr auto record_time = 4u;
static ::std::uint8_t* wav = nullptr;

extern "C" esp_err_t client_initialize()
{
	auto requires_size = ::recorder_data_size_requires(record_time);
	ESP_LOGI("client", "requires_size = %u largest free block = %u", requires_size, ::heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
	if (requires_size > ::heap_caps_get_largest_free_block(MALLOC_CAP_8BIT))
	{
		ESP_LOGE("main", "insufficient memory to record 5 seconds");
		return ESP_FAIL;
	}
	wav = static_cast<::std::uint8_t*>(::heap_caps_malloc(requires_size, MALLOC_CAP_8BIT));
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
		::std::printf("0x%02x", static_cast<::std::uint8_t>(i));
	}
	for (auto i = 0u; i < ::recorder_data_size_requires(record_time); i++)
	{
		::std::printf("0x%02x", wav[i]);
	}
}

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

	if (auto err = ::http_client_end_send_wav(client); err != ESP_OK)
	{
		ESP_LOGE("main", "Failed to end send wav");
		::esp_http_client_cleanup(client);
		::std::abort();
	}
}

extern "C" void client_receive_wav()
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
	if (content_length > ::recorder_data_size_requires(record_time))
	{
		ESP_LOGE("main", "insufficient memory to receive wav");
		::std::abort();
	}
	auto data_size = ::http_client_receive_wav(client, wav, content_length);
	ESP_LOGI("main", "receive %u bytes", data_size);
}