echo "Programming SHMAC"
sudo shmac_reset on
sudo shmac_program 0x0 zeroes
sudo shmac_program 0x80000 Image
sudo shmac_reset off
