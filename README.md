# MiniAHCI kernel module for Linux

## Description
The MiniAHCI kernel module is designed for data recovery from partially faulty SATA storage devices. This module has minimal functionality and hides the connected devices from the operating system. As result to get full control over connected device you have to use an external application with user application library. For more information see the source code of the QMiniAHCIDevice library.

## Preparation
Before using this module you have to unbind your AHCI controller from the standard AHCI driver. Type `lspci -Dnn | grep 0106`, for example:
```
alexander@asus-b760m-plus-wifi:~/Projects/miniahci$ lspci -Dnn | grep 0106
0000:00:17.0 SATA controller [0106]: Intel Corporation Device [8086:7a62] (rev 11)
0000:05:00.0 SATA controller [0106]: JMicron Technology Corp. JMB363 SATA/IDE Controller [197b:2363] (rev 03)
```
In example above my AHCI controller location is `0000:05:00.0`, so I need to type in terminal:
```
sudo -i
cd /sys/bus/pci/drivers/ahci
echo 0000:05:00.0 > unbind
exit
```
Or you can use `unbind` script.

## How to build
Make sure Linux kernel headers are installed:
```
sudo apt-get install linux-headers-$(uname -r)
```
If everything above is done type in terminal:
```
git clone https://github.com/aekhv/miniahci.git
cd miniahci
make
```
As result file `miniahci.ko` will be generated.

## How to run
Just type in terminal:
```
sudo insmod miniahci.ko
```
If AHCI controller is attached you will see in `dmesg | grep miniahci` output something like this:
```
[ 1516.306959] miniahci: Kernel object loaded
[ 1516.307016] miniahci: PCI device attached: vendor 0x197b, device 0x2363, class 0x0106, revision 0x03
[ 1516.803413] miniahci: Character device created: /dev/miniahci5
```
>P.S. The number at the end of the device name means the PCIe bus number. For example, `/dev/miniahci5` means the device attached to the PCIe bus number 5.
