/* Simple HTTP Server Example

	 This example code is in the Public Domain (or CC0 licensed, at your option.)

	 Unless required by applicable law or agreed to in writing, this
	 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	 CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "esp_http_server.h"

static const char *TAG = "HTTP";

extern RingbufHandle_t xRingbuffer;

#define PART_BOUNDARY "123456789000000000000987654321"

static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

/* root get handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "root_get_handler");

	esp_err_t res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
	assert(res==ESP_OK);
	res = httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	assert(res==ESP_OK);
	res = httpd_resp_set_hdr(req, "X-Framerate", "60");
	assert(res==ESP_OK);


	int tv_sec = 0;
	int tv_usec = 0;
	while (true)
	{
		// Receive stream data from RingBuffer
		size_t bytes_received = 0;
		char *payload = (char *)xRingbufferReceive( xRingbuffer, &bytes_received, pdMS_TO_TICKS(2500));
		if (payload != NULL) {
			ESP_LOGD(TAG, "bytes_received=%d", bytes_received);
			ESP_LOGD(TAG, "payload[0]=%02x payload[1]=%02x", payload[0], payload[1]);
			ESP_LOGD(TAG, "payload[bytes_received-3]=%02x", payload[bytes_received-3]);
			ESP_LOGD(TAG, "payload[bytes_received-2]=%02x", payload[bytes_received-2]);
			ESP_LOGD(TAG, "payload[bytes_received-1]=%02x", payload[bytes_received-1]);

			// Chack if data is valid jpeg payload
#if 0
MJPEGFormat
I (40482) HTTP: payload[0]=ff payload[1]=d8 --> Start marker
I (40492) HTTP: payload[bytes_received-3]=ff --> End marker
I (40492) HTTP: payload[bytes_received-2]=d9 --> End marker
I (40502) HTTP: payload[bytes_received-1]=d9 --> End marker

UncompressedFormat
I (16832) HTTP: payload[0]=ff payload[1]=d8 --> Start marker
I (16842) HTTP: payload[bytes_received-3]=9f
I (16842) HTTP: payload[bytes_received-2]=ff --> End marker
I (16852) HTTP: payload[bytes_received-1]=d9 --> End marker
#endif
			if (payload[0] == 0xff && payload[1] == 0xd8) { 
				if ((payload[bytes_received-2] == 0xff && payload[bytes_received-1] == 0xd9) ||
					(payload[bytes_received-3] == 0xff && payload[bytes_received-2] == 0xd9)) {
					tv_sec++;
					ESP_LOGD(TAG, "tv_sec=%d bytes_received=%d", tv_sec, bytes_received);
					res = httpd_resp_sendstr_chunk(req, _STREAM_BOUNDARY);
					ESP_LOGD(TAG, "res(1)=%d", res);
					if (res != ESP_OK) {
						vRingbufferReturnItem(xRingbuffer, (void *)payload);
						break;
					}
					char part_buf[128];
					size_t hlen = snprintf(part_buf, 128, _STREAM_PART, bytes_received, tv_sec, tv_usec);
					res = httpd_resp_send_chunk(req, part_buf, hlen);
					ESP_LOGD(TAG, "res(2)=%d", res);
					if (res != ESP_OK) {
						vRingbufferReturnItem(xRingbuffer, (void *)payload);
						break;
					}
					res = httpd_resp_send_chunk(req, (const char *)payload, bytes_received);
					ESP_LOGD(TAG, "res(3)=%d", res);
					if (res != ESP_OK) {
						vRingbufferReturnItem(xRingbuffer, (void *)payload);
						break;
					}
					//httpd_resp_sendstr_chunk(req, NULL);
				}
			}
			vRingbufferReturnItem(xRingbuffer, (void *)payload);
		}
	}

	return ESP_OK;
}

/* favicon get handler */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "favicon_get_handler");
	return ESP_OK;
}

/* Function to start the web server */
esp_err_t start_server(int port)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Purge“"Least Recently Used” connection
	config.lru_purge_enable = true;
	// TCP Port number for receiving and transmitting HTTP traffic
	config.server_port = port;

	// Start the httpd server
	//ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start file server!");
		return ESP_FAIL;
	}

	// Set URI handlers
	httpd_uri_t _root_get_handler = {
		.uri		 = "/",
		.method		 = HTTP_GET,
		.handler	 = root_get_handler,
		//.user_ctx  = "Hello World!"
	};
	httpd_register_uri_handler(server, &_root_get_handler);

	httpd_uri_t _favicon_get_handler = {
		.uri		 = "/favicon.ico",
		.method		 = HTTP_GET,
		.handler	 = favicon_get_handler,
		//.user_ctx  = "Hello World!"
	};
	httpd_register_uri_handler(server, &_favicon_get_handler);

	return ESP_OK;
}



void http_server(void *pvParameters)
{
	char *task_parameter = (char *)pvParameters;
	ESP_LOGI(TAG, "Start task_parameter=%s", task_parameter);
	char url[64];
	int port = 8080;
	sprintf(url, "http://%s:%d", task_parameter, port);
	ESP_LOGI(TAG, "Starting HTTP server on %s", url);
	ESP_ERROR_CHECK(start_server(port));

	while(1) {
		vTaskDelay(100);
	}

	// Never reach here
	ESP_LOGI(TAG, "finish");
	vTaskDelete(NULL);
}
