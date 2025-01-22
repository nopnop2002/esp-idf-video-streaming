# libuvc for linux
libuvc example for linux.   

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
```
