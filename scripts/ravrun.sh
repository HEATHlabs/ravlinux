#!/bin/bash

DIR=/home/arv3

if [[ $1 == *b* ]]
then
  echo "Uploading new bootloader"
  make -C $RL/shboot
  scp $RL/shboot/bootloader.bin arv3@$SB:$DIR/shboot.bin
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
  make -C $RL/linux-3.12.13 dtbs
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

if [[ $1 == *r* ]]
then
    echo "Uploading ramdisk to SHMACBOX"

    # create ramdisk file with ext2 filesystem
    dd if=/dev/zero of=$RL/ramdisk bs=200k count=1
    mke2fs -F -m0 -b 1024 $RL/ramdisk

    # Remove any old mountpoint lying around
    if [ -d $RL/ramdiskmnt ]; then
        sudo umount $RL/ramdiskmnt
        sudo rm -fr $RL/ramdiskmnt
    fi

    # Mount the ramdisk so that we can manipulatie it
    sudo mkdir $RL/ramdiskmnt
    sudo mount -t ext2 -o loop $RL/ramdisk mnt
    sudo mkdir $RL/ramdiskmnt/dev
    sudo mknod $RL/ramdiskmnt/dev/console c 5 1

    # Add all of the userland applications to the ramdisk
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


    # Transmit filesystem to SHMAC host
    scp $RL/ramdisk arv3@$SB:$DIR/ramdisk

    #clean up

    # Remove mountpoint
    sleep 1
    sudo umount $RL/ramdiskmnt
    sudo rm -r $RL/ramdiskmnt
    # Remove ramdisk
    sudo rm $RL/ramdisk
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
