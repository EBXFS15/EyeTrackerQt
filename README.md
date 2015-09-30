# EyeTrackerQt
Tested with Logitech C210, Logitech HD C920, Logitech Quickcam Pro 9000

# Setup
* 1) sudo apt-get install libopencv-dev on BeagleBoneBlack
* 2) sudo apt-get install libv4l-dev on BeagleBoneBlack
* 3) In order to solve unreferenced libraries by libpthread and libc, do the following:
navigate to /usr/lib/arm-linux-gnueabihf on your BBB rootfs (usually opt/embedded/bbb/rootfs/usr/lib/arm-linux-gnueabihf)

<br\>Edit libpthread.so and remove absolute paths like this:
<br\>Before:
```
/* GNU ld script
   Use the shared library, but some functions are only in
   the static library, so try that secondarily.  */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( /lib/arm-linux-gnueabihf/libpthread.so.0 /usr/lib/arm-linux-gnueabihf/libpthread_nonshared.a )
```
<br\>After:
```
/* GNU ld script
   Use the shared library, but some functions are only in
   the static library, so try that secondarily.  */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( libpthread.so.0 libpthread_nonshared.a )
```
<br\>Edit libc.so and remove absolute paths like this:

<br\>Before:
```
/* GNU ld script
   Use the shared library, but some functions are only in
   the static library, so try that secondarily.  */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( /lib/arm-linux-gnueabihf/libc.so.6 /usr/lib/arm-linux-gnueabihf/libc_nonshared.a  AS_NEEDED ( /lib/arm-linux-gnueabihf/ld-linux-armhf.so.3 ) )
```
<br\>After:
```
/* GNU ld script
   Use the shared library, but some functions are only in
   the static library, so try that secondarily.  */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( libc.so.6 libc_nonshared.a  AS_NEEDED ( ld-linux-armhf.so.3 ) )
```
* 4) Git clone https://github.com/EBXFS15/EyeTrackerQt
* 5) Copy haarcascade_eye_tree_eyeglasses.xml in /usr/local/bin/ on BeagleBoneBlack
* 6) Copy uvcvideo.ko from https://github.com/EBXFS15/uvc-from-bbb-sources to /usr/local/bin/
* 7) Copy ebx_monitor.ko from https://github.com/EBXFS15/ebx_monitor to /usr/local/bin/
* 8) Copy setup.sh to /usr/local/bin/
* 9) Open project in QtCreator, compile and deploy to /usr/local/bin (copy the binary «EyeTracker» to /usr/local/bin/)


