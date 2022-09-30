# esp-idf-video-streaming
Capture video from a USB camera using ESP-IDF.   

This example demonstrates how to:   
- Capture video from a USB camera using the libuvc library.
- Stream the video over WiFi by hosting a HTTP server.
- Supports plugging and unplugging USB cameras.   

The example enumerates connected camera, negotiates selected resolution together with FPS and starts capturing video.   

I based it on [this](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/host/uvc).   


# Change from official example

- Changed WiFi connection by example_connect to WiFi connection by handler.   

- Supported VideoStreaming format of UncompressedFormat.   
 This allows you can use the Logitech C270.   
 Conversion from UVC_FRAME_FORMAT_YUYV to JPEG used [this](https://github.com/espressif/esp32-camera/blob/master/conversions/to_jpg.cpp).   

- Change from TCP transmission to HTTP transmission.   
 I base it on [this](https://github.com/espressif/esp-iot-solution/tree/master/examples/camera/video_stream_server).   

# Hardware Required

- ESP32-S2/ESP32-S3
 This example requires any ESP32-S2 or ESP32-S3 with external PSRAM and exposed USB connector attached to USB camera.   
 ESP module without external PSRAM will fail to initialize.   
 I used this board.   
 ![esp32-s3-1](https://user-images.githubusercontent.com/6020549/193182915-ac4dbd55-b3ee-4327-b64d-d055e3774b7d.JPG)

- Stable power supply
 USB cameras consume a lot of electricity.   
 If the power supplied to the USB port is insufficient, the camera will not operate and ESP32 resets.  

- USB camera with UVC support
 [Here](https://www.freebsd.org/cgi/man.cgi?query=uvc&sektion=4&manpath=freebsd-release-ports) is a list of USB cameras that support UVC that work with FreeBSD.   
 From what I've researched, ESP-IDF's USB support is incomplete.   
 For example, the Logitech C615 works with Linux, but not with ESP-IDF.   
 I tested with these cameras.   
 - Logitech C270
 - Logitech C615
 - Logitech QuickCam Pro 9000
 - PAPALOOK AF925
 - Microdia(Very old model)
 ___It is very hard work to find a camera that works with ESP-IDF.___   
 On [this](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/host/uvc) document says that these cameras work with ESP-IDF.   
 - Logitech C980
 - CANYON CNE-CWC2
 When usb support provided by ESP-IDF is updated, there is possible that this limitation may be removed.   
 Detail is [here](https://github.com/nopnop2002/esp-idf-video-streaming/issues).   

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

![config-app-1](https://user-images.githubusercontent.com/6020549/193183047-2afea305-7553-4d34-92e8-ad3cecb8cfe2.jpg)

Some capera support 640x480 streaming. Runs at 15FPS.   
![config-app-2](https://user-images.githubusercontent.com/6020549/193183048-51907f8d-6438-41e7-9299-f19293b5d8e8.jpg)


# How to use

- Build and flash firmware to esp32.

- Connect the USB camera at this timing.   
![WaitingForDevice](https://user-images.githubusercontent.com/6020549/193183218-1a2100c8-7b51-444d-953e-cf7b8acd3313.jpg)

- For available USB cameras, the device information will be displayed.      
![StartStreaming](https://user-images.githubusercontent.com/6020549/193183252-fa3473ef-b0b4-4639-b01f-e7ce8f497583.jpg)

- For unavailable USB cameras, you will see an error like this.   
![NotSupportCamera](https://user-images.githubusercontent.com/6020549/193183435-e35e03e4-e5f7-4efd-bfbf-87e2afde3b6f.jpg)

- Start your browser and enter the ESP32 IP address and port number in the address bar.   
You can use mDNS instead of ESP32 IP addresses.   
![WebStreaming](https://user-images.githubusercontent.com/6020549/193183519-04e5f68d-20bf-4f82-8455-a857190ccd1b.jpg)

# Limitations
- At 320x240, 30FPS is the maximum rate.
- At 640x480, 15FPS is the maximum rate.


