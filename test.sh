#!/bin/bash
PROJECT="xac_onehand_pico"
IDEVER="1.8.19"
machine=`uname -m`
if [[ $machine =~ .*armv.* ]] ; then
    WORKDIR="/var/tmp/autobuild_${PROJECT}_$$"
    ARCH="linuxarm"
elif [[ $machine =~ .*aarch64.* ]] ; then
    WORKDIR="/var/tmp/autobuild_${PROJECT}_$$"
    ARCH="linuxaarch64"
else
    WORKDIR="/tmp/autobuild_${PROJECT}_$$"
    ARCH="linux64"
fi
mkdir -p ${WORKDIR}
# Install Ardino IDE in work directory
if [ ! -f ~/Downloads/arduino-${IDEVER}-${ARCH}.tar.xz ]
then
    wget https://downloads.arduino.cc/arduino-${IDEVER}-${ARCH}.tar.xz
    mv arduino-${IDEVER}-${ARCH}.tar.xz ~/Downloads
fi
tar xf ~/Downloads/arduino-${IDEVER}-${ARCH}.tar.xz -C ${WORKDIR}
# Create portable sketchbook and library directories
IDEDIR="${WORKDIR}/arduino-${IDEVER}"
LIBDIR="${IDEDIR}/portable/sketchbook/libraries"
mkdir -p "${LIBDIR}"
export PATH="${IDEDIR}:${PATH}"
cd ${IDEDIR}
which arduino
# Install board package
if [ -d ~/Sync/ard_staging ]
then
    ln -s ~/Sync/ard_staging ${IDEDIR}/portable/staging
fi
arduino --pref "compiler.warning_level=default" \
    --pref "update.check=false" \
    --pref "editor.external=true" \
    --save-prefs
arduino --pref "boardsmanager.additional.urls=https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json" --save-prefs
arduino --install-boards "rp2040:rp2040"
BOARD="rp2040:rp2040:rpipico"
arduino --board "${BOARD}" --save-prefs
arduino --pref "custom_usbstack=rpipico_tinyusb_host" \
        --save-prefs
CC="arduino --verify --board ${BOARD}"
arduino --install-library "Adafruit TinyUSB Library"
ln -s ~/Sync/${PROJECT} ${LIBDIR}/..
cd ${IDEDIR}
ctags -R .
