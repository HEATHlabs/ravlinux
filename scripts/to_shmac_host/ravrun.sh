#!/bin/bash

sudo shmac_reset on

sudo shmac_program 0x0 zeroes
sudo shmac_program 0x0 shboot.bin
sudo shmac_program 0x4000 dtb.bin
sudo shmac_program 0x8000 linux.bin
sudo shmac_program 0x01000000 ramdisk
sudo shmac_reset off
