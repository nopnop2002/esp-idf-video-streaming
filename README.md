# esp-idf-video-streaming
Capture video from a USB camera using ESP-IDF.   

This example demonstrates how to:   
- Capture video from a USB camera using the USB Host UVC Dreiver.
 https://components.espressif.com/components/espressif/usb_host_uvc   
- Stream videos over WiFi using the built-in HTTP server.   
- Supports plugging and unplugging USB cameras.   

This example enumerates the attached camera descriptors, negotiates the selected resolution and FPS, and starts capturing video.   

I based it on [this](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/host/uvc).   


# Change from official example

- Changed Network connection by example_connect to WiFi connection by handler.   

- Supported VideoStreaming format of UncompressedFormat.   
 This will allow you to use Logitech C270 and Microsoft LifeCam.   
 Conversion from UVC_FRAME_FORMAT_YUYV to JPEG used [this](https://github.com/espressif/esp32-camera/blob/master/conversions/to_jpg.cpp).   

- Change from TCP transmission to HTTP transmission.   
 I base it on [this](https://github.com/espressif/esp-iot-solution/tree/master/examples/camera/video_stream_server).   

# Hardware Required

## ESP32-S2/ESP32-S3
This example requires any ESP32-S2 or ESP32-S3 with external PSRAM and exposed USB connector attached to USB camera.   
ESP module without external PSRAM will fail to initialize.   
I used this board.   
![esp32-s3-1](https://user-images.githubusercontent.com/6020549/193182915-ac4dbd55-b3ee-4327-b64d-d055e3774b7d.JPG)

## Stable power supply
USB cameras consume a lot of electricity.   
If the power supplied to the USB port is insufficient, the camera will not operate and ESP32 resets.  

## USB camera with UVC support
[Here](https://www.freebsd.org/cgi/man.cgi?query=uvc&sektion=4&manpath=freebsd-release-ports) is a list of USB cameras that support UVC that work with FreeBSD.   
From what I've researched, ESP-IDF's USB support is incomplete.   
For example, the Logitech C615 works with Linux, but not with ESP-IDF.   
I tested with these cameras.   
- Logitech C270 -> Success   
- Logitech C615 -> Fail  
- Logitech QuickCam Pro 9000 -> Fail   
- PAPALOOK AF925 -> Fail   
- Microdia(Very old model) -> Success   
- Microdia MSI Starcam Racer -> Success   
- Microsoft LifeCam NX6000 -> Fail   
- Microsoft LifeCam Cinema -> Success   
- Microsoft LifeCam HD3000 -> Success   
- Microsoft LifeCam HD5000 -> Success   

___It is very hard work to find a camera that works with ESP-IDF.___   
On [this](https://components.espressif.com/components/espressif/usb_host_uvc) document says that these cameras work with ESP-IDF.   
- Logitech C980
- CANYON CNE-CWC2

When usb support provided by ESP-IDF is updated, this issue may eliminate the problem.   
Detail is [here](https://github.com/nopnop2002/esp-idf-video-streaming/issues).   

![cameras](https://user-images.githubusercontent.com/6020549/195963953-2fd44723-1ef6-4ece-84c3-412f9ca1497c.JPG)

# Software Required
esp-idf v5.0 or later.   
A compilation error occurs in ESP-IDF Ver4.   

# Wireing   
```
ESP BOARD          USB CONNECTOR (type A)
                         +--+
5V        -------------> | || VCC
[GPIO19]  -------------> | || D-
[GPIO20]  -------------> | || D+
GND       -------------> | || GND
                         +--+
```


# Installation
```
git clone https://github.com/nopnop2002/esp-idf-video-streaming
cd esp-idf-video-streaming
idf.py set-target {esp32s2/esp32s3}
idf.py menuconfig
idf.py flash monitor
```

# Configuration

![config-main](https://user-images.githubusercontent.com/6020549/193183008-7114e7d5-672b-4e51-9afd-84af7e5b7aa1.jpg)

![config-app-1](https://user-images.githubusercontent.com/6020549/194474315-7cdc917a-ba5b-47d8-a0a0-673f3a51644f.jpg)

Some cameras need to change frame size and rate.   
![config-app-2](https://user-images.githubusercontent.com/6020549/194474320-f08c9f96-81e5-4423-9b1e-a568ec1fe34e.jpg)


# How to use

- Build and flash firmware to esp32.

- Connect the USB camera at this timing.   
![WaitingForDevice](https://user-images.githubusercontent.com/6020549/193183218-1a2100c8-7b51-444d-953e-cf7b8acd3313.jpg)

- For available USB cameras, device information will be displayed and video streaming will begin.   
![StartStreaming](https://user-images.githubusercontent.com/6020549/193183252-fa3473ef-b0b4-4639-b01f-e7ce8f497583.jpg)

- For unavailable USB cameras, you will see an error like this.   
![NotSupportCamera](https://user-images.githubusercontent.com/6020549/193183435-e35e03e4-e5f7-4efd-bfbf-87e2afde3b6f.jpg)

- Start your browser and enter the ESP32 IP address and port number in the address bar.   
You can use mDNS instead of ESP32 IP addresses.   
![WebStreaming](https://user-images.githubusercontent.com/6020549/193183519-04e5f68d-20bf-4f82-8455-a857190ccd1b.jpg)

# Limitations
- At 320x240, 30FPS is the maximum rate.
- At 640x480, 15FPS is the maximum rate.


# Using ESP32-CAM development board
You can use [this](https://github.com/espressif/esp-iot-solution/tree/master/examples/camera/video_stream_server) repository.

