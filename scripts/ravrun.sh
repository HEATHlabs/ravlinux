#!/bin/bash

# REQUIRED ENVIRONMENT VARIABLES
# SB -> ip to shmacbox
# RL -> path to ravlinux repo /home/lol/ravlinux (no slash at end)

# Where should our 'workspace' be ?
DIR=/home/arv3

# Username to shmacbox
UNAME=arv3

if [[ $1 == *b* ]]
then
  echo "Uploading new bootloader"
  make -C $RL/shboot
  scp $RL/shboot/bootloader.bin $UNAME@$SB:$DIR/shboot.bin
fi

if [[ $1 == *l* ]]
then
  echo "Uploading linux image to SHMACBOX"
  make -C $RL/linux-3.12.13 Image -j4
  scp $RL/linux-3.12.13/arch/arm/boot/Image $UNAME@$SB:$DIR/linux.bin
fi

if [[ $1 == *d* ]]
then
  echo "Uploading Device Tree Blob to SHMACBOX"
  make -C $RL/linux-3.12.13 dtbs
  scp $RL/linux-3.12.13/arch/arm/boot/dts/shmac.dtb $UNAME@$SB:$DIR/dtb.bin
fi


if [[ $1 == *u* ]]
then
    # Compile all userland applications
    for dir in $(find $RL/userland/ -maxdepth 1 -mindepth 1 -type d | xargs -n1 basename | grep -v include)
    do
        # Temp variables
        dir=${dir%*/}
        app=${dir##*/}
        appdir=$RL/userland/$app

        # Now compile and copy to ramdisk
        echo "Adding application $app to ramdisk"
        make -C $appdir
        chmod +x $appdir/$app #\.flt
        sudo cp $appdir/$app $RL/ramdiskmnt/$app
    done
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
  ssh $UNAME@$SB "cd $DIR;./ravdump.sh $SIZE $DUMPFILE"

  # Download the dumpfile
  scp $UNAME@$SB:$DIR/$DUMPFILE .

  # Disassemble the dumpfile
  arm-none-eabi-objdump -D -marmv4t -Mreg-names-apcs -bbinary $DUMPFILE > $RL/$DUMPFILE
  echo "dump saved to $RL/$DUMPFILE"
  less $RL/$DUMPFILE

fi

if [[ $1 == *s* ]]
then
  scp $RL/scripts/to_shmac_host/ravrun.sh $UNAME@$SB:$DIR/ravrun.sh
fi

if [[ $1 == *h* ]] || [[ $# -eq 0 ]]
then
  echo "Usage: ravrun.sh {[bld] | m [SIZE_IN_BYTES]}"
  echo " l - LINUX "
  echo " d - DEVICE TREE BLOB"
  echo " b - BOOTLOADER"
  echo " -------------------- "
  echo " u - Compile all userland applications"
  echo " s - copy run script to host"
  echo " m [bytes] - Dump bytes from memory"
fi

if [[ $1 == *d* ]] || [[ $1 == *l* ]] || [[ $1 == *b* ]]
then
  echo "Executing"
  ssh $UNAME@$SB "cd $DIR;./ravrun.sh"
fi
