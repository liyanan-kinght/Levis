<a id="top"></a>
# Raspberry Pi OS is installed on Raspberry Pi 4B

You can visit the raspberry PI website to see how to install the system. You can also use the following method.

* [Return to the top](../README.md#top)

## Install system at SD card

After installing the SYSTEM SD card, there will be two partitions(boot && rootfs).

Raspberry PI website. 

https://www.raspberrypi.org/documentation/raspbian/

https://www.raspberrypi.org/documentation/installation/installing-images/README.md

https://www.raspberrypi.org/downloads/noobs/

* It is recommended that you use a high-speed SD card

![high-speed SD Card Info](art_picture/sd_card_info.png)
![high-speed SD Card](art_picture/sd_card.png)

## Raspberry Pi 4 Model B
You can buy it on Taobao. https://www.taobao.com/

Raspberry PI website buy device. https://www.raspberrypi.org/products/

![Raspberry Pi 4 Model B](art_picture/raspberry_4b.png)


## ReSpeaker 6-Mic Circular Array kit
You can buy it on Taobao. https://www.taobao.com/

Raspberry PI website buy device. https://www.raspberrypi.org/products/

![ReSpeaker 6-Mic Circular Array kit](art_picture/6mic_hat.png)

## Assembled speakers:
![Assembled speakers](art_picture/Assembled_speakers.png)

## Raspberry PI website download os
System image file. https://www.raspberrypi.org/software/operating-systems/

![Raspberry PI website download os](art_picture/system_image.png)

## This is the approach I've tried
1. System image file (You should use Lite version).
*  Download system os file.  https://www.raspberrypi.org/software/operating-systems/
2. Sd card installation system tools
*  Download balenaEtcher. https://www.balena.io/etcher/   
3. If you want to expand your roofs zoning
*  You can use DiskGenius. https://www.diskgenius.cn/

###  If your device is new and you don’t know how to connect network：
1. Create empty file at SD card boot partition.
```
touch ssh 
```
2. Create a file wpa_supplicant.conf at SD card boot partition.The content of the file is as follows:
```

country=CN
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="your_wifi_name"
    psk="your_wifi_password"
    key_mgmt=WPA-PSK
}
```
