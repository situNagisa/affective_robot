#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

	esp_err_t client_initialize();
	void client_deinitialize();
	void client_print_wav();
	void client_send_wav();
	void client_receive_wav();

#ifdef __cplusplus
}
#endif