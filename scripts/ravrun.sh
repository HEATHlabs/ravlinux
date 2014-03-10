#!/bin/bash

DIR=/home/pi

if [[ $1 == *b* ]]
then
  echo "Uploading new bootloader"
  make -C ../shboot
  scp ../shboot/bootloader.bin arv3@$SB:$DIR/shboot.bin
fi

if [[ $1 == *r* ]]
then
  echo "Uploading ramdisk to SHMACBOX"
  scp ../ramdisk/something arv3@$SB:$DIR/ramdisk
fi

if [[ $1 == *l* ]]
then
  echo "Uploading linux image to SHMACBOX"
  scp ../linux-3.12.13/arch/arm/boot/Image arv3@$SB:$DIR/linux.bin
fi

if [[ $1 == *d* ]]
then
  echo "Uploading Device Tree Blob to SHMACBOX"
  scp ../linux-3.12.13/arch/arm/boot/dts/shmac.dtb arv3@$SB:$DIR/dtb.bin
fi

ssh arv3@$SB "cd $DIR;./ravrun.sh"
