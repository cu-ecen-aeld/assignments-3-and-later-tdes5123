#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

echo "----1----"
export PATH=/home/tdes5123/toolchains/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/bin:$PATH


TC_LIBC_PATH=/home/tdes5123/toolchains/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

echo "----1.1----"
which ${CROSS_COMPILE}gcc
echo "----2----"

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi
echo "----3----"

mkdir -p ${OUTDIR}
if [ $? -ne 0 ]; then
    echo "----3.1----"
    echo "Error: Could not create output directory ${OUTDIR}"
    exit 1
fi

# cd "$OUTDIR"
# if [ ! -d "${OUTDIR}/linux-stable" ]; then
#     #Clone only if the repository does not exist.
# 	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
# 	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
# fi
# if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
#     cd linux-stable
#     echo "Checking out version ${KERNEL_VERSION}"
#     git checkout ${KERNEL_VERSION}

#     # TODO: Add your kernel build steps here
#     make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
#     make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
#     make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
#     #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
#     make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

# fi
# cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image
# echo "----4----"


# echo "Adding the Image in outdir"

# echo "Creating the staging directory for the root filesystem"
# cd "$OUTDIR"
# if [ -d "${OUTDIR}/rootfs" ]
# then
# 	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
#     sudo rm  -rf ${OUTDIR}/rootfs
# fi
# echo "----5----"


# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs/{dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr/{bin,lib,lib64,sbin},var/log}
echo "----6----"


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi
echo "----7----"


# TODO: Make and install busybox
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
echo "----8----"
cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"
echo "----9----"


# TODO: Add library dependencies to rootfs
cp ${TC_LIBC_PATH}/lib/ld-linux-aarch64.so.1 lib/
cp ${TC_LIBC_PATH}/lib64/libm.so.6 lib64/
cp ${TC_LIBC_PATH}/lib64/libresolv.so.2 lib64/
cp ${TC_LIBC_PATH}/lib64/libc.so.6 lib64/
echo "----10----"

# TODO: Make device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3

echo "----11----"

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp writer ${OUTDIR}/rootfs/home/
cp finder.sh ${OUTDIR}/rootfs/home/
cp finder-test.sh ${OUTDIR}/rootfs/home/
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/home/

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
