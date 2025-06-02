By default, the makefile in this repo uses a freescale compiler from NXP. This guide describes the steps required to set it up. If desired, a different ARM compiler may be used, but because of the firmware on the default EVAcharge SE board, the compiler used must be C++03 compatible. 

#### 1. Install necessary packages

Install _net-tools, make, makedepend, rpm, can-utils_ by running the following commands:1
```
sudo apt-get -y install net-tools
sudo apt install make
sudo apt install xutils-dev
sudo apt install rpm
sudo apt install can-utils
```

#### 2. Download the Build Support Package

- The compiler install can be found [here](https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx28-processors/multimedia-applications-processors-dual-ethernet-dual-can-lcd-touch-screen-arm9-core:i.MX287). Access to the file requires a free sign-up and login in order to get the download link.
- The entire file size should be 744.4 MB, last updated on Feb 22, 2013 with revision number _L2.6.35_1.1.0_.

#### 3. Setup GCC toolchain

In order to build C/C++ applications for the target board, we need to setup the GCC toolchain. First extract the BSP downloaded in Step 2 and navigate to _pkgs_ folder:

```
tar -xvf L2.6.35_1.1.0_130130_source.tar.gz
cd L2.6.35_1.1.0_130130_source/pkgs
```

Install the GCC toolchain by running the following command:

```
sudo rpm -i gcc-4.4.4-glibc-2.11.1-multilib-1.0-1.i386.rpm
```

The toolchain binary is originally compiled for a 32-bit system, but since we’re on a 64-bit system, we need to enable i386 package installation support by running:

```
sudo dpkg --add-architecture i386
sudo apt-get update
```

_gcc-multilib_ is the package which will enable running 32bit (x86) binaries on 64bit (amd64/x86_64) system. Install it using the following commands:

```
sudo apt-get install gcc-multilib
sudo apt-get install zlib1g:i386
```

We also need to install _git, build-essential_ and _fakeroot_ to keep version history, GCC/G++ debugger & compiler and grant root privileges for file manipulation:

```
sudo apt-get install git build-essential fakeroot
```

And finally, install some additional packages to compile for ARM:

```
sudo apt-get install u-boot-tools lzop
```
