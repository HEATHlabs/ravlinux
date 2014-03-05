#!/bin/bash

if [[ $1 == *b* ]]
then
  echo "Uploading new bootloader"
  scp ../bootloader/something arv3@$SB:~/bootloader
fi

if [[ $1 == *r* ]]
then
  echo "Uploading ramdisk to SHMACBOX"
  scp ../ramdisk/something arv3@$SB:~/ramdisk
fi

if [[ $1 == *l* ]]
then
  echo "Uploading linux image to SHMACBOX"
  scp ../linux-3.12.13/arch/arm/boot/Image arv3@$SB:~/Image
fi


ssh arv3@$SB "~/ravlinux/shmacbox/ravrun.sh"
