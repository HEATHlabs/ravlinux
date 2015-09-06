ravlinux
========

Linux 3.12.13 customized for the Rav processor tile of SHMAC

updates 8/31/15
elf2flat changed 0004-target1_reloc_fix.patch, replacing patch files paths to 
     --- a/elf2flt.c 2014-04-25 16:57:17.957415180 +0200
     +++ b/elf2flt.c 2014-04-25 16:58:39.287411802 +0200
     
updates 9/3/15
(install ubuntu 14.04 in virtualbox)
sudo apt-get install libncurses5-dev
sudo  apt-get install gperf
apt-get install bison
apt-get install flex
apt-get install texinfo
apt-get install gawk
apt-get install libtool
apt-get install automake
apt-get install zlib1g-dev

updates 9/5/15
export PATH=$PATH:$HOME/x-tools/arm-rav-uclinux-uclibcgnueabi/bin

linux: 
export ARCH=arm

make menuconfig
   -> General setup
        ->Initramfs source file(s)
             -> ~/src/ravlinux-master/initramfs/initramfs.list

cd ravlinux             
mkdir ../initramfs
cp -R initramfs/ ../initramfs/

mkdir ~/src/initramfs/etc
cp ~/src/userland/busybox-1.22.1/examples/bootfloppy/etc/* ~/src/initramfs/etc

make menuconfig
   -> boot options
       -> ENABLE Use appended device tree blob to zImage (EXPERIMENTAL)
then build shmac.dtb and cat shmac.dtb >> linux/arch/arm/boot/Image

