/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <unistd.h>
#include "libuvc/libuvc.h"
#include "libuvc_helper.h"
#include "libuvc_adapter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "lwip/dns.h"

#include "usb/usb_host.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "img_converters.h"


static const char *TAG = "example";

#if 0
#define USB_DISCONNECT_PIN  GPIO_NUM_0
#endif

#define FPS_15 15
#define FPS_30 30
#define WIDTH 320
#define HEIGHT 240
#define FORMAT UVC_COLOR_FORMAT_MJPEG // UVC_COLOR_FORMAT_YUYV

// Attached camera can be filtered out based on (non-zero value of) PID, VID, SERIAL_NUMBER
#define PID 0
#define VID 0
#define SERIAL_NUMBER NULL

#define UVC_CHECK(exp) do {                 \
    uvc_error_t _err_ = (exp);              \
    if(_err_ < 0) {                         \
        ESP_LOGE(TAG, "UVC error: %s",      \
                 uvc_error_string(_err_));  \
        assert(0);                          \
    }                                       \
} while(0)

static SemaphoreHandle_t ready_to_uninstall_usb;
static EventGroupHandle_t app_flags;



RingbufHandle_t xRingbuffer;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

esp_err_t wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&event_handler,
		NULL,
		&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&event_handler,
		NULL,
		&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ESP_WIFI_SSID,
			.password = CONFIG_ESP_WIFI_PASSWORD,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,

			.pmf_cfg = {
				.capable = true,
				.required = false
			},
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	//ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	esp_err_t ret_value = ESP_OK;
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
		WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
		ret_value = ESP_FAIL;
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
		ret_value = ESP_FAIL;
	}

#if 0
	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
#endif
	vEventGroupDelete(s_wifi_event_group);
	return ret_value;
}

void initialise_mdns(void)
{
	//initialize mDNS
	ESP_ERROR_CHECK( mdns_init() );
	//set mDNS hostname (required if you want to advertise services)
	ESP_ERROR_CHECK( mdns_hostname_set(CONFIG_MDNS_HOSTNAME) );
	ESP_LOGI(TAG, "mdns hostname set to: [%s]", CONFIG_MDNS_HOSTNAME);

#if 0
	//set default mDNS instance name
	ESP_ERROR_CHECK( mdns_instance_name_set("ESP32 with mDNS") );
#endif
}


// Handles common USB host library events
static void usb_lib_handler_task(void *args)
{
    while (1) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // Release devices once all clients has deregistered
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
        // Give ready_to_uninstall_usb semaphore to indicate that USB Host library
        // can be deinitialized, and terminate this task.
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            xSemaphoreGive(ready_to_uninstall_usb);
        }
    }

    vTaskDelete(NULL);
}

static esp_err_t initialize_usb_host_lib(void)
{
    TaskHandle_t task_handle = NULL;

    const usb_host_config_t host_config = {
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };

    esp_err_t err = usb_host_install(&host_config);
    if (err != ESP_OK) {
        return err;
    }

    ready_to_uninstall_usb = xSemaphoreCreateBinary();
    if (ready_to_uninstall_usb == NULL) {
        usb_host_uninstall();
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(usb_lib_handler_task, "usb_events", 4096, NULL, 2, &task_handle) != pdPASS) {
        vSemaphoreDelete(ready_to_uninstall_usb);
        usb_host_uninstall();
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

static void uninitialize_usb_host_lib(void)
{
    xSemaphoreTake(ready_to_uninstall_usb, portMAX_DELAY);
    vSemaphoreDelete(ready_to_uninstall_usb);

    if ( usb_host_uninstall() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall usb_host");
    }
}

static void displayBuffer(uint8_t * buf, int buf_len, bool flag) {
	if (flag == false) return;
	ESP_LOGI(TAG, "buf_len=%d", buf_len);
	ESP_LOGI(TAG, "buf[0]=%02x", *buf);
	ESP_LOGI(TAG, "buf[1]=%02x", *(buf+1));
	ESP_LOGI(TAG, "buf[buf_len-3]=%02x", *(buf+buf_len-3));
	ESP_LOGI(TAG, "buf[buf_len-2]=%02x", *(buf+buf_len-2));
	ESP_LOGI(TAG, "buf[buf_len-1]=%02x", *(buf+buf_len-1));
}


/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
void frame_callback(uvc_frame_t *frame, void *ptr)
{
	static size_t fps;
	static size_t bytes_per_second;
	static int64_t start_time;
	int64_t current_time = esp_timer_get_time();
	bytes_per_second += frame->data_bytes;
	fps++;

	if (!start_time) {
		start_time = current_time;
	}

	if (current_time > start_time + 1000000) {
		ESP_LOGI(TAG, "fps: %u, bytes per second: %u", fps, bytes_per_second);
		start_time = current_time;
		bytes_per_second = 0;
		fps = 0;
	}

	// Stream received frame to other
	ESP_LOGD(TAG, "frame_format=%d width=%ld height=%ld data_bytes=%d", frame->frame_format, frame->width, frame->height, frame->data_bytes);
	if (frame->frame_format == UVC_FRAME_FORMAT_YUYV) {
		size_t _jpg_buf_len = 0;
		uint8_t *_jpg_buf = NULL;
		bool ret = fmt2jpg(frame->data, frame->data_bytes, frame->width, frame->height, PIXFORMAT_YUV422, 60, &_jpg_buf, &_jpg_buf_len);
		ESP_LOGD(TAG, "fmt2jpg ret=%d", ret);
		if (!ret) {
			ESP_LOGE(TAG, "JPEG compression failed");
		} else {
			displayBuffer(_jpg_buf, _jpg_buf_len, false);
			if ( xRingbufferSend(xRingbuffer, _jpg_buf, _jpg_buf_len, pdMS_TO_TICKS(1)) != pdTRUE ) {
				ESP_LOGD(TAG, "Failed to send frame to ring buffer.");
			}
			free(_jpg_buf);
		}
	} else if (frame->frame_format == UVC_COLOR_FORMAT_MJPEG) {
		displayBuffer(frame->data, frame->data_bytes, false);
		if ( xRingbufferSend(xRingbuffer, frame->data, frame->data_bytes, pdMS_TO_TICKS(1)) != pdTRUE ) {
			ESP_LOGD(TAG, "Failed to send frame to ring buffer.");
		}
	} else {
		ESP_LOGW(TAG, "Unknown frame format %d", frame->frame_format);
	}
}

void button_callback(int button, int state, void *user_ptr)
{
    printf("button %d state %d\n", button, state);
}

static void libuvc_adapter_cb(libuvc_adapter_event_t event)
{
    xEventGroupSetBits(app_flags, event);
}

static EventBits_t wait_for_event(EventBits_t event)
{
    return xEventGroupWaitBits(app_flags, event, pdTRUE, pdFALSE, portMAX_DELAY) & event;
}

static void camera_not_available(char *message) {
	ESP_LOGE(TAG, "%s", message);
	ESP_LOGE(TAG, "Change other camera and re-start");
	while(1) { vTaskDelay(1); }
}

void http_server(void *pvParameters);

int app_main(int argc, char **argv)
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize WiFi
	wifi_init_sta();

	// Initialize mDNS
	initialise_mdns();

	// Create RingBuffer
	xRingbuffer = xRingbufferCreate(100000, RINGBUF_TYPE_BYTEBUF);
	configASSERT( xRingbuffer );

	// Create HTTP Server Task 
	char cparam0[64];
	esp_netif_ip_info_t ip_info;
	ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info));
	sprintf(cparam0, IPSTR, IP2STR(&ip_info.ip));
	ESP_LOGI(TAG, "cparam0=[%s]", cparam0);
	xTaskCreate(http_server, "HTTP", 1024*6, (void *)cparam0, 2, NULL);
	vTaskDelay(100);






    uvc_context_t *ctx;
    uvc_device_t *dev;
    uvc_device_handle_t *devh;
    uvc_stream_ctrl_t ctrl;
    uvc_error_t res;

    app_flags = xEventGroupCreate();
    assert(app_flags);

    ESP_ERROR_CHECK( initialize_usb_host_lib() );

    libuvc_adapter_config_t config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .callback = libuvc_adapter_cb
    };

    libuvc_adapter_set_config(&config);

    UVC_CHECK( uvc_init(&ctx, NULL) );

	while(1) {

        printf("Waiting for device\n");
        wait_for_event(UVC_DEVICE_CONNECTED);

        UVC_CHECK( uvc_find_device(ctx, &dev, PID, VID, SERIAL_NUMBER) );
        puts("Device found");

        UVC_CHECK( uvc_open(dev, &devh) );

        // Uncomment to print configuration descriptor
        // libuvc_adapter_print_descriptors(devh);

        uvc_set_button_callback(devh, button_callback, NULL);

        // Print known device information
        uvc_print_diag(devh, stderr);

		// Set default setting
		int fps = FPS_30;
		int width = WIDTH;
		int height = HEIGHT;
		int format = FORMAT;

		// Get FrameDescriptor 
		const uvc_format_desc_t *format_desc = uvc_get_format_descs(devh);
		const uvc_frame_desc_t *frame_desc = format_desc->frame_descs;
		if (frame_desc) {
			ESP_LOGI(TAG, "format_desc->fourccFormat=(%4s)", format_desc->fourccFormat);
			ESP_LOGI(TAG, "frame_desc->wWidth=%d", frame_desc->wWidth);
			ESP_LOGI(TAG, "frame_desc->wHeight=%d", frame_desc->wHeight);
			ESP_LOGI(TAG, "format_desc->bDescriptorSubtype=%d", format_desc->bDescriptorSubtype);
			ESP_LOGI(TAG, "frame_desc->dwDefaultFrameInterval=%lu", frame_desc->dwDefaultFrameInterval);
			if (format_desc->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED) { // Logitech C270/C270N
				ESP_LOGI(TAG, "format_desc->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED");
				width = frame_desc->wWidth;
				height = frame_desc->wHeight;
				fps = 10000000 / frame_desc->dwDefaultFrameInterval;
				//format = UVC_FRAME_FORMAT_UNCOMPRESSED;
				format = UVC_FRAME_FORMAT_YUYV;
			} else if (format_desc->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG) { // 
				ESP_LOGI(TAG, "format_desc->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG");
				width = frame_desc->wWidth;
				height = frame_desc->wHeight;
				fps = 10000000 / frame_desc->dwDefaultFrameInterval;
				//format = UVC_COLOR_FORMAT_MJPEG;
				format = UVC_FRAME_FORMAT_MJPEG;
			} else {
				camera_not_available("Unknown UVC_VS_FORMAT");
			}
		}

#if CONFIG_SIZE_640x480
		ESP_LOGW(TAG, "FRAME SIZE=640x480 FRAME RATE=%d", CONFIG_FRAME_RATE);
		width = 640;
		height = 480;
		fps = CONFIG_FRAME_RATE;
#elif CONFIG_SIZE_352x288
		ESP_LOGW(TAG, "FRAME SIZE=352x288 FRAME RATE=%d", CONFIG_FRAME_RATE);
		width = 352;
		height = 288;
		fps = CONFIG_FRAME_RATE;
#elif CONFIG_SIZE_320x240
		ESP_LOGW(TAG, "FRAME SIZE=320x240 FRAME RATE=%d", CONFIG_FRAME_RATE);
		width = 320;
		height = 240;
		fps = CONFIG_FRAME_RATE;
#elif CONFIG_SIZE_160x120
		ESP_LOGW(TAG, "FRAME SIZE=160x120 FRAME RATE=%d", CONFIG_FRAME_RATE);
		width = 160;
		height = 120;
		fps = CONFIG_FRAME_RATE;
#else
		if (width >= 640 || height >= 480) {
			width = 640;
			height = 480;
			if (fps > FPS_15) {
				ESP_LOGW(TAG, "Frame rate changed from %d to %d", fps, FPS_15);
				fps = FPS_15;
			}
		}
#endif

#if CONFIG_FORMAT_YUY2
		ESP_LOGW(TAG, "FRAME FORMAT=YUYV");
		format = UVC_FRAME_FORMAT_YUYV;
#elif CONFIG_FORMAT_MJPG
		ESP_LOGW(TAG, "FRAME FORMAT=MJPEG");
		format = UVC_FRAME_FORMAT_MJPEG;
#endif

#if 0
		// Force setting
		width = 640;
		height = 480;
		fps = 10;
		format = UVC_FRAME_FORMAT_MJPEG;
#endif

		// Negotiate stream profile
		int retry = 0;
		ESP_LOGI(TAG, "format=%d width=%d height=%d fps=%d", format, width, height, fps);
		//res = uvc_get_stream_ctrl_format_size(devh, &ctrl, FORMAT, WIDTH, HEIGHT, FPS_30 );
		res = uvc_get_stream_ctrl_format_size(devh, &ctrl, format, width, height, fps );
		while (res != UVC_SUCCESS) {
			printf("Negotiating streaming format failed, trying again...\n");
			//res = uvc_get_stream_ctrl_format_size(devh, &ctrl, FORMAT, WIDTH, HEIGHT, FPS_30 );
			res = uvc_get_stream_ctrl_format_size(devh, &ctrl, format, width, height, fps );
			sleep(1);
			retry++;
			if (retry == 10) {
				camera_not_available("uvc_get_stream_ctrl_format_size fail");
			}
		}

        // dwMaxPayloadTransferSize has to be overwritten to MPS (maximum packet size)
        // supported by ESP32-S2(S3), as libuvc selects the highest possible MPS by default.
        ctrl.dwMaxPayloadTransferSize = 512;

        uvc_print_stream_ctrl(&ctrl, stderr);

        //UVC_CHECK( uvc_start_streaming(devh, &ctrl, frame_callback, NULL, 0) );
		res = uvc_start_streaming(devh, &ctrl, frame_callback, NULL, 0);
		ESP_LOGI(TAG, "uvc_start_streaming=%d", res);
		if (res != 0) {
			camera_not_available("uvc_start_streaming fail");
		}
        puts("Streaming...");

        wait_for_event(UVC_DEVICE_DISCONNECTED);

        uvc_stop_streaming(devh);
        puts("Done streaming.");

        uvc_close(devh);

    }

	// Never reach here
    uvc_exit(ctx);
    puts("UVC exited");

    uninitialize_usb_host_lib();
	vTaskDelete(NULL);
}
