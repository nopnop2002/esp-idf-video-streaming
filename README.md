# esp-idf-video-streaming
Capture video from a USB camera using ESP-IDF.   

This example demonstrates how to:   
- Capture video from a USB camera using the USB Host UVC Driver.   
 __This repository uses components from version 1.0.4.__   
 https://components.espressif.com/components/espressif/usb_host_uvc   
- Stream videos over WiFi using the built-in HTTP server.   
- Supports plugging and unplugging USB cameras.   

This example enumerates the attached camera descriptors, negotiates the selected resolution and FPS, and starts capturing video.   

I based it on [this](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/host/uvc) official example.   


# Change from official example

- Changed Network connection by example_connect to WiFi connection by handler.   

- Supported VideoStreaming format of UncompressedFormat.   
 This will allow you to use Logitech C270n and Microsoft LifeCam.   
 Conversion from UVC_FRAME_FORMAT_YUYV to JPEG used [this](https://github.com/espressif/esp32-camera/blob/master/conversions/to_jpg.cpp).   

- Changed from TCP transmission to HTTP transmission.   
 I base it on [this](https://github.com/espressif/esp-iot-solution/tree/master/examples/camera/video_stream_server).   

# Hardware Required

## ESP32-S2/ESP32-S3
This example requires any ESP32-S2 or ESP32-S3 with external PSRAM and exposed USB connector attached to USB camera.   
ESP module without external PSRAM will fail to initialize.   
I used this board.   
![esp32-s3-1](https://user-images.githubusercontent.com/6020549/193182915-ac4dbd55-b3ee-4327-b64d-d055e3774b7d.JPG)

__Note for ESP32S2__   
In earlier versions of the ESP32-S2 chip, USB transfers can cause SPI data contamination (esp32s2>=ECO1 and esp32s3 do not have this bug).   
Software workaround is [here](https://docs.espressif.com/projects/esp-iot-solution/en/latest/usb/usb_host/usb_stream.html).   

## Stable power supply
USB cameras consume a lot of electricity.   
If the power supplied to the USB port is insufficient, the camera will not operate and ESP32 resets.  

## USB Type-A Femail connector
USB connectors are available from AliExpress or eBay.   
I used it by incorporating it into a Universal PCB.   
![USBConnector](https://github.com/user-attachments/assets/8d7d8f0a-d289-44b8-ae90-c693a1099ca0)

We can buy this breakout on Ebay or AliExpress.   
![usb-conector-11](https://github.com/user-attachments/assets/848998d4-fb0c-4b4f-97ae-0b3ae8b8996a)
![usb-conector-12](https://github.com/user-attachments/assets/6fc34dcf-0b13-4233-8c71-07234e8c6d06)

## USB camera with UVC support
[Here](https://www.freebsd.org/cgi/man.cgi?query=uvc&sektion=4&manpath=freebsd-release-ports) is a list of USB cameras that support UVC that work with FreeBSD.   
From what I've researched, ESP-IDF has limited USB support.   
For example, the Logitech C615 works with Linux, but not with ESP-IDF.   
I tested with these cameras.   
- Logitech C270n -> Success   
- Logitech C615 -> Fail  
- Logitech QuickCam Pro 9000 -> Fail   
- PAPALOOK AF925 -> Fail   
- Microdia(Very old model) -> Success   
- Microdia MSI Starcam Racer -> Success   
- Microsoft LifeCam NX6000 -> Fail   
- Microsoft LifeCam Cinema -> Success   
- Microsoft LifeCam Studio -> Success   
- Microsoft LifeCam HD3000 -> Success   
- Microsoft LifeCam HD5000 -> Success   
- Microsoft LifeCam HD6000 for Notebooks -> Success   

___It is very hard to find a camera that works with ESP-IDF.___   
On [this](https://components.espressif.com/components/espressif/usb_host_uvc) document says that these cameras work with ESP-IDF.   
- Logitech C980
- CANYON CNE-CWC2

When usb support provided by ESP-IDF is updated, this issue may eliminate the problem.   
Detail is [here](https://github.com/nopnop2002/esp-idf-video-streaming/issues).   

![cameras](https://user-images.githubusercontent.com/6020549/195963953-2fd44723-1ef6-4ece-84c3-412f9ca1497c.JPG)

# Software Required
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Wireing   
USB camera connects via USB connector.   
The USB port on the ESP32 development board does not function as a USB-HOST.   

```
+---------+  +-------------+  +----------+
|ESP BOARD|==|USB CONNECTOR|==|USB CAMERA|
+---------+  +-------------+  +----------+
```

```
ESP BOARD          USB CONNECTOR (type A)
                         +--+
5V        -------------> | || VCC
[GPIO19]  -------------> | || D-
[GPIO20]  -------------> | || D+
GND       -------------> | || GND
                         +--+
```

![USBConnector-2](https://github.com/user-attachments/assets/b5ae9a94-175c-46c7-a9a8-6be527cdd56e)

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

Some cameras need to change frame size, frame rate and frame format.   
![config-app-2](https://user-images.githubusercontent.com/6020549/196664222-be32b37d-605c-4ca7-915f-766ae60b599a.jpg)

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

- Streaming example : Some cameras have Auto Focus available.   
![sample-1](https://user-images.githubusercontent.com/6020549/213895050-ae63bf80-dbaf-4631-8ed0-b9be53daee37.jpg)
![sample-2](https://user-images.githubusercontent.com/6020549/213895051-59632bec-04f7-4543-be0e-d59f0d48b392.jpg)
![sample-3](https://user-images.githubusercontent.com/6020549/213895052-08cdaf4f-bb91-4528-8b21-3a234a51ee7b.jpg)

# Limitations
- At 320x240, 30FPS is the maximum rate regardless of camera capabilities.
- At 640x480, 15FPS is the maximum rate regardless of camera capabilities.


# Using ESP32-CAM development board
You can use [this](https://github.com/espressif/esp-iot-solution/tree/master/examples/camera/video_stream_server) repository.

# References
https://github.com/nopnop2002/esp-idf-video-snapshot
