#!/bin/bash

DIR=/home/arv3

if [[ $1 == *b* ]]
then
  echo "Uploading new bootloader"
  make -C $RL/shboot
  scp $RL/shboot/bootloader.bin arv3@$SB:$DIR/shboot.bin
fi

if [[ $1 == *r* ]]
then
  echo "Uploading ramdisk to SHMACBOX"
  scp $RL/ramdisk/something arv3@$SB:$DIR/ramdisk
fi

if [[ $1 == *l* ]]
then
  echo "Uploading linux image to SHMACBOX"
  make -C $RL/linux-3.12.13 Image -j4
  scp $RL/linux-3.12.13/arch/arm/boot/Image arv3@$SB:$DIR/linux.bin
fi

if [[ $1 == *d* ]]
then
  echo "Uploading Device Tree Blob to SHMACBOX"
  scp $RL/linux-3.12.13/arch/arm/boot/dts/shmac.dtb arv3@$SB:$DIR/dtb.bin
fi

ssh arv3@$SB "cd $DIR;./ravrun.sh"
