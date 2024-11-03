#pragma once

#include <esp_err.h>
#include <stdint.h>
#include <stddef.h>
#include <esp_http_client.h>

#include "./wav.h"

#ifdef __cplusplus
extern "C" {
#endif

	esp_err_t http_client_initialize();
	esp_err_t http_client_test();

	esp_http_client_handle_t http_client_start_send_wav();
	esp_err_t http_client_send_wav_header(esp_http_client_handle_t client, ::wav_header const* header);
	esp_err_t http_client_send_wav_data_stream(esp_http_client_handle_t client, uint8_t const* wav, size_t size);
	esp_err_t http_client_end_send_wav(esp_http_client_handle_t client);
#ifdef __cplusplus
}
#endif