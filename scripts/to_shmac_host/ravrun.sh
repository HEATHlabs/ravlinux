#!/bin/bash

sudo shmac_reset on

# Write zeroes to the entire memory.
sudo shmac_program 0x0 zeroes

# Load Bootloader 
sudo shmac_program 0x0 shboot.bin

# Load Device Tree Blob
sudo shmac_program 0x4000 dtb.bin

# Load Linux
sudo shmac_program 0x8000 linux.bin

sudo shmac_reset off
