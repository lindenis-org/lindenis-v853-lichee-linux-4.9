#!/bin/bash

echo "=================================================="
echo "1.Copy firmware"
echo "=================================================="
cp ../image/* /lib/firmware/

echo "=================================================="
echo "1.Unload Module"
echo "=================================================="
./unload.sh

./gpio_reset.sh on

echo "=================================================="
echo "2.Set Hardware Capability"
echo "=================================================="

eth0_local_mac=`ifconfig eth0  | grep HWaddr | sed -e 's/.*HWaddr //g' | sed -e 's/ //g' | cut -d ':' -f 4,5,6`
[ "$eth0_local_mac" == "" ] && eth0_local_mac="45:67:89"
local_mac=00:a5:b5:$eth0_local_mac
local_mac_2=`echo $local_mac | cut -d ':' -f 6`
local_mac_2=`printf '%x' $[ ( 16#$local_mac_2 + 1 ) % 4 + ( 16#$local_mac_2 & 16#FC ) ] `
local_mac_2="`echo $local_mac | cut -d ':' -f 1,2,3,4,5`:$local_mac_2"

echo WLAN MAC is $local_mac

sleep 1
./gpio_reset.sh off
sleep 1

cat sta.cfg.template | sed -e "s/MAC_ADDR/$local_mac/g" | sed -e "s/MAC2ADDR/$local_mac_2/g" > sta_local_mac.cfg
./ssvcfg.sh sta_local_mac.cfg

echo "=================================================="
echo "3.Load MMC Module"
echo "=================================================="
modprobe mmc_core
modprobe sdhci
modprobe sdhci-pci
modprobe mmc_block

echo "=================================================="
echo "4.Load SSV6200 Driver"
echo "=================================================="
echo 6 > /proc/sys/kernel/printk

#modprobe ssv6200_sdio
