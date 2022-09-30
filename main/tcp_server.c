/* BSD Socket API Example

	 This example code is in the Public Domain (or CC0 licensed, at your option.)

	 Unless required by applicable law or agreed to in writing, this
	 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	 CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

static const char *TAG = "TCP";

extern RingbufHandle_t xRingbuffer;

// https://www.ibm.com/docs/en/i/7.3?topic=designs-example-nonblocking-io-select
// Nonblocking I/O and select()
esp_err_t waitForConnect (int listen_sock, int sec, int usec) {
	fd_set master_set, working_set;
	int max_sd = listen_sock;
	FD_ZERO(&master_set);
	FD_SET(listen_sock, &master_set);

	struct timeval timeout;
	timeout.tv_sec	= sec;
	timeout.tv_usec = usec;
	while (1) {
		memcpy(&working_set, &master_set, sizeof(master_set));
		ESP_LOGD(TAG, "Waiting on select()...");
		int err = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
		ESP_LOGD(TAG, "select err=%d", err);
		if (err == 1) break;
		if (err < 0) {
			ESP_LOGE(TAG, "waitForConnect select() fail");
			return ESP_FAIL;
		}
		if (err == 0) {
			ESP_LOGD(TAG, "waitForConnect select() timed out");
			return ESP_ERR_TIMEOUT;
		}
	}
	return ESP_OK;
}

void tcp_server(void *pvParameters)
{
	char addr_str[128];
	int addr_family = AF_INET;
	int ip_protocol = IPPROTO_IP;

	ESP_LOGI(TAG, "Start TCP PORT=%d", CONFIG_TCP_PORT);
	struct sockaddr_storage dest_addr;
	struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
	dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
	dest_addr_ip4->sin_family = addr_family;
	dest_addr_ip4->sin_port = htons(CONFIG_TCP_PORT);
	ip_protocol = IPPROTO_IP;

	int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
	if (listen_sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		return;
	}
	int opt = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	ESP_LOGI(TAG, "Socket created");

	int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err != 0) {
		ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
		return;
	}
	ESP_LOGI(TAG, "Socket bound, port %d", CONFIG_TCP_PORT);

	err = listen(listen_sock, 1);
	if (err != 0) {
		ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
		return;
	}
	ESP_LOGI(TAG, "Socket listening");

	bool isConnect = false;
	int sock = 0;
	while (1) {
		if (!isConnect) {
			// Wait for connect
			err = waitForConnect (listen_sock, 0, 100);
			if (err == ESP_FAIL) break;
			if (err == ESP_OK) {
				struct sockaddr_storage source_addr;
				socklen_t addr_len = sizeof(source_addr);
				sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
				if (sock < 0) {
					ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
					break;
				}
				// Convert ip address to string
				if (source_addr.ss_family == PF_INET) {
					inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
				}
				ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);
				isConnect = true;
			}
		}

		// Receive from  RingBuffer
		size_t bytes_received = 0;
		char *payload = (char *)xRingbufferReceive( xRingbuffer, &bytes_received, pdMS_TO_TICKS(2500));

		// Send payload to other
		if (payload != NULL) {
			if (isConnect) {
				int sent = send(sock, payload, bytes_received, 0);
				if (sent < 0) {
					ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
					isConnect = false;
				}
			}
			vRingbufferReturnItem(xRingbuffer, (void *)payload);
			//traceHeap();
		}
	}

	// close socket
	ESP_LOGI(TAG, "Close socket");
	err = lwip_close(sock);
	LWIP_ASSERT("err == 0", err == 0);
	vTaskDelete(NULL);
}
