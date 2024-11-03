#pragma once

#include <esp_err.h>
#include <stdint.h>
#include <stddef.h>
#include "./wav.h"

#ifdef __cplusplus
extern "C" {
#endif

	esp_err_t recorder_initialize();
	esp_err_t recorder_deinitialize();
	size_t recorder_data_size_requires(size_t record_time_second);
	size_t recorder_record(uint8_t* wav, size_t record_time_second);
	wav_header recorder_wav_header(size_t data_size);

#ifdef __cplusplus
}
#endif