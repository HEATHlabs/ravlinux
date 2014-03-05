#!/bin/bash

if [ "x$RAV" == "x" ]; then echo "Need to set \$RAV to ravlink repo path (no slash at the end)"; exit 1; fi
if [ "x$SB" == "x" ]; then echo "Need to set \$SB to SHMAC box ip address."; exit 1; fi

if [ ! -F /usr/local/bin/ravrun ]; then
  echo "Creating file /usr/local/bin/ravrun"
  echo "This file calls the ./ravrun.sh script" 
  sudo cp ravrun /usr/local/bin/ravrun
fi
