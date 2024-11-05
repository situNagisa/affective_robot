#include <driver/i2s.h>
#include "esp_log.h"
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <new>
#include <algorithm>
#include <esp_heap_caps.h>
#include "esp_task_wdt.h"
#include "./wav.h"
#include "./i2sconfig.h"

#define I2S_WS 15
#define I2S_SD 13
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0

extern "C" esp_err_t recorder_initialize()
{
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
	i2s_config_t i2s_config = {
		.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
		.sample_rate = I2S_SAMPLE_RATE,
		.bits_per_sample = static_cast<::i2s_bits_per_sample_t>(I2S_SAMPLE_BITS),
		.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
		.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
		.intr_alloc_flags = 0,
		.dma_buf_count = 8,
		.dma_buf_len = 1024,
		.use_apll = 0,
	};
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic pop
#endif

	i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

	i2s_pin_config_t pin_config = {
		.bck_io_num = I2S_SCK,
		.ws_io_num = I2S_WS,
		.data_out_num = I2S_PIN_NO_CHANGE,
		.data_in_num = I2S_SD
	};

	i2s_set_pin(I2S_PORT, &pin_config);

	return ESP_OK;
}

extern "C" esp_err_t recorder_deinitialize()
{
	return ESP_OK;
}

extern "C" ::std::size_t recorder_data_size_requires(::std::size_t record_time_second)
{
	return I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / CHAR_BIT * record_time_second;
}

extern "C" ::wav_header recorder_wav_header(::std::size_t data_size)
{
	return ::wav_header_construct(
		data_size + sizeof(::wav_header) - sizeof(::wav_header::riff) - sizeof(::wav_header::file_size),
		10,
		1,
		I2S_CHANNEL_NUM,
		I2S_SAMPLE_RATE,
		I2S_SAMPLE_RATE * I2S_CHANNEL_NUM * I2S_SAMPLE_BITS / CHAR_BIT,
		I2S_CHANNEL_NUM * I2S_SAMPLE_BITS / CHAR_BIT,
		I2S_SAMPLE_BITS,
		data_size
	);
}

extern "C" ::std::size_t recorder_record(::std::uint8_t* data, ::std::size_t record_time_second)
{
	auto requires_size = ::recorder_data_size_requires(record_time_second);
	::std::size_t size = 0;

	while(size < requires_size)
	{
		ESP_LOGI("recorder", "size: %u (%u%%)", size, size * 100 / requires_size);
		::std::size_t bytes_read;
		::i2s_read(I2S_PORT, data + size, ::std::min<::std::size_t>(I2S_READ_LEN, requires_size - size), &bytes_read, portMAX_DELAY);
		size += bytes_read;
	}

	return size;
}
