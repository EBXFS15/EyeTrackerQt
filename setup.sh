#!/bin/bash

measPoint="125"


if test -z "$1"
then
	echo "Default number of measurement points will be used."
else
	measPoint=$1
fi

rmmod uvcvideo
rmmod ebx_monitor
insmod ./ebx_monitor.ko nbrOfMeasurementPoints=$measPoint deferredFifo=1
chmod 666 /sys/devices/virtual/misc/ebx_monitor/readOneShot

# Prepare to read out the results without waiting for the next measurement.
echo 1 > /sys/devices/virtual/misc/ebx_monitor/readOneShot

insmod ./uvcvideo.ko quirks=640
