# libuvc for linux
libuvc example for linux.   
You can check the formats and resolutions supported by libuvc.   

# Installation
```
sudo apt install libusb-1.0-0-dev
sudo apt install libjpeg-dev

git clone https://github.com/libuvc/libuvc
cd libuvc
mkdir build
cd build
cmake ..
make && sudo make install
```

# Execute example code
```
# Insert your usb camera to host
cc example.c -luvc
sudo ./a.out


First format: (MJPG) 1280x720 30fps
bmHint: 0001
bFormatIndex: 1
bFrameIndex: 1
dwFrameInterval: 333333
wKeyFrameRate: 0
wPFrameRate: 0
wCompQuality: 0
wCompWindowSize: 0
wDelay: 0
dwMaxVideoFrameSize: 2764800
dwMaxPayloadTransferSize: 3072
bInterfaceNumber: 1
Streaming...
Enabling auto exposure ...
 ... full AE not supported, trying aperture priority mode
 ... uvc_set_ae_mode failed to enable aperture priority mode: Pipe (-9)
callback! frame_format = 7, width = 1280, height = 720, length = 106908, ptr = 12345
callback! frame_format = 7, width = 1280, height = 720, length = 126828, ptr = 12345
callback! frame_format = 7, width = 1280, height = 720, length = 123400, ptr = 12345

```
