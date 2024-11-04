
#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_log.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cinttypes>
#include <bit>
#include <new>
#include <ranges>
#include <string_view>

#include <esp_http_client.h>
#include <esp_timer.h>

#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"

#include "./wav.h"

inline static constexpr auto tag = "http client";
#define BOUNDARY "boundary_tag"
inline static constexpr auto&& boundary_start =
	"--" BOUNDARY "\r\n"
	R"(Content-Disposition: from-data; name=""; filename="")" "\r\n"
	R"(Content-Type: audio/wav)" "\r\n"
	"\r\n"
	;
inline static constexpr auto&& boundary_end = "\r\n--" BOUNDARY "--\r\n";

extern "C" esp_err_t http_client_initialize()
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	/* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
	 * Read "Establishing Wi-Fi or Ethernet Connection" section in
	 * examples/protocols/README.md for more information about this function.
	 */
	ESP_ERROR_CHECK(example_connect());
	ESP_LOGI(tag, "Connected to AP, begin http example");
	return ESP_OK;
}

extern "C" esp_http_client_handle_t http_client_start_send_wav()
{
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
	esp_http_client_config_t config = {
		.url = "http://134.175.177.58:9191/tem/upload-voice",
	};
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic pop
#endif
	auto client = ::esp_http_client_init(&config);

	::esp_http_client_set_method(client, HTTP_METHOD_POST);
	::esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

	return client;
}

extern "C" esp_err_t http_client_send_wav_header(esp_http_client_handle_t client, ::wav_header const* header)
{
	if (auto err = esp_http_client_open(client, sizeof(::wav_header) + header->data_size); err != ESP_OK) {
		ESP_LOGE(tag, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		return err;
	}
	auto result = ::esp_http_client_write(client, reinterpret_cast<const char*>(header), sizeof(::wav_header));
	ESP_LOGI(tag, "write %d", result);

	return ESP_OK;
}

extern "C" esp_err_t http_client_send_wav_data_stream(esp_http_client_handle_t client, uint8_t const* wav, ::std::size_t size)
{
	auto result = ::esp_http_client_write(client, reinterpret_cast<const char*>(wav), size);
	ESP_LOGI(tag, "write %d", result);
	return ESP_OK;
}

extern "C" esp_err_t http_client_end_send_wav(esp_http_client_handle_t client)
{
	if (::esp_http_client_fetch_headers(client) < 0)
	{
		ESP_LOGE(tag, "HTTP client fetch headers failed");
		::esp_http_client_cleanup(client);
		return ESP_FAIL;
	}
	char output_buffer[256]{};
	auto data_read = esp_http_client_read_response(client, output_buffer, ::std::ranges::size(output_buffer));
	if (data_read < 0)
	{
		ESP_LOGE(tag, "Failed to read response");
		return ESP_FAIL;
	}
	ESP_LOGI(tag, "HTTP POST status = %d, content_length = %" PRId64 " data = %s",
		esp_http_client_get_status_code(client),
		esp_http_client_get_content_length(client),
		output_buffer
	);
	ESP_LOG_BUFFER_HEX(tag, output_buffer, data_read);
	esp_http_client_cleanup(client);

	return ESP_OK;
}


extern "C" esp_http_client_handle_t http_client_start_receive_wav(char const* filename, ::std::int64_t* content_length)
{
	auto&& base_url = "http://134.175.177.58:9191/tem/download-voice/";
	char url[256]{};
	if (::std::strlen(filename) > ::std::ranges::size(url) - ::std::ranges::size(base_url))
	{
		ESP_LOGE(tag, "Filename too long");
		::std::abort();
	}
	::std::memcpy(url, base_url, ::std::ranges::size(base_url));
	::std::strcat(url, filename);
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
	ESP_LOGI(tag, "url = %s", url);
	esp_http_client_config_t config = {
		.url = url,
	};
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic pop
#endif
	auto client = ::esp_http_client_init(&config);

	::esp_http_client_set_method(client, HTTP_METHOD_GET);
	::esp_http_client_set_header(client, "Content-Type", "audio/wav");

	if (auto err = esp_http_client_open(client, 0); err != ESP_OK) {
		ESP_LOGE(tag, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		::esp_http_client_close(client);
		::esp_http_client_cleanup(client);
		::std::abort();
	}
	*content_length = ::esp_http_client_fetch_headers(client);
	return client;
}

extern "C" ::std::size_t http_client_receive_wav(esp_http_client_handle_t client, ::std::uint8_t* buffer, ::std::size_t size)
{
	auto data_read = ::esp_http_client_read_response(client, reinterpret_cast<char*>(buffer), size);
	ESP_LOGI(tag, "HTTP POST status = %d", ::esp_http_client_get_status_code(client));
	::esp_http_client_close(client);
	::esp_http_client_cleanup(client);
	return data_read;
}

extern "C" esp_err_t http_client_test()
{
	char output_buffer[256]{};
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
	esp_http_client_config_t config = {
		.url = "http://119.91.44.126:9090/api/text/save",
	};
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic pop
#endif
	auto client = ::esp_http_client_init(&config);

	constexpr auto post_data = ::std::string_view(R"({"data":)");
	constexpr auto post_data2 = ::std::string_view(R"("hello"})");
	::esp_http_client_set_method(client, HTTP_METHOD_POST);
	::esp_http_client_set_header(client, "Content-Type", "application/json");

	if (auto err = esp_http_client_open(client, ::std::ranges::size(post_data) + ::std::ranges::size(post_data2)); err != ESP_OK) {
		ESP_LOGE(tag, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		::esp_http_client_cleanup(client);
		return err;
	}
	if (::esp_http_client_write(client, ::std::ranges::data(post_data), ::std::ranges::size(post_data)) < 0)
	{
		ESP_LOGE(tag, "Write failed");
		::esp_http_client_cleanup(client);
		return ESP_FAIL;
	}
	if (::esp_http_client_write(client, ::std::ranges::data(post_data2), ::std::ranges::size(post_data2)) < 0)
	{
		ESP_LOGE(tag, "Write failed");
		::esp_http_client_cleanup(client);
		return ESP_FAIL;
	}
	if (::esp_http_client_fetch_headers(client) < 0)
	{
		ESP_LOGE(tag, "HTTP client fetch headers failed");
		::esp_http_client_cleanup(client);
		return ESP_FAIL;
	}
	auto data_read = esp_http_client_read_response(client, output_buffer, ::std::ranges::size(output_buffer));
	if (data_read < 0)
	{
		ESP_LOGE(tag, "Failed to read response");
		::esp_http_client_cleanup(client);
		return ESP_FAIL;
	}

	ESP_LOGI(tag, "HTTP POST status = %d, content_length = %" PRId64 " data = %s",
		esp_http_client_get_status_code(client),
		esp_http_client_get_content_length(client),
		output_buffer
	);
	ESP_LOG_BUFFER_HEX(tag, output_buffer, data_read);
	esp_http_client_cleanup(client);

	return ESP_OK;
}