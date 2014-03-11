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
  make $RL/linux-3.12.13 dtbs
  scp $RL/linux-3.12.13/arch/arm/boot/dts/shmac.dtb arv3@$SB:$DIR/dtb.bin
fi

if [[ $1 == *m* ]]
then
  SIZE=1000
  DUMPFILE="dump"

  if [ $# -eq 2 ]
    then
      SIZE=$2
    fi

  echo "Downloading memory dump with size (bytes) $SIZE" 

  # Dump the memory, and save it to a file
  ssh arv3@$SB "cd $DIR;./ravdump.sh $SIZE $DUMPFILE"

  # Download the dumpfile
  scp arv3@$SB:$DIR/$DUMPFILE .

  # Disassemble the dumpfile
  arm-none-eabi-objdump -D -marmv4t -Mreg-names-apcs -bbinary $DUMPFILE > $RL/$DUMPFILE
  echo "dump saved to $RL/$DUMPFILE"
  less $RL/$DUMPFILE

fi

if [[ $1 == *h* ]]
then
  echo "Usage: ravrun.sh {[bld] | m [SIZE_IN_BYTES]}"
fi

if [[ $1 == *d* ]] || [[ $1 == *l* ]] || [[ $1 == *b* ]]
then
  echo "Executing"
  ssh arv3@$SB "cd $DIR;./ravrun.sh"
fi
