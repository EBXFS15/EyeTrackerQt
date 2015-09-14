# EyeTrackerQt
Tested with Logitech C920

# Setup
* 1)apt-get install libopencv-dev on BeagleBoneBlack
* 2)sudo apt-get install libv4l-dev
* 3)In order to solve unreferenced libraries by libpthread and libc, do the following:
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
* 4) git clone https://github.com/EBXFS15/EyeTrackerQt
* 5) copy haarcascade_eye_tree_eyeglasses.xml in /usr/local/bin/ of BBB
* 6) open project in QtCreator and deploy

# ToDos
- [x] Create startup project to capture webcam
- [x] Implement eye tracking algorithm
  - [x] Dummy algoritm
  - [x] optional: real eye detection algorithm
- [ ] Fix opencv dependencies

# Troubleshooting

If you do not see any image with your camera, you can try using opencv
instead of directly using v4l2 by removing the define USE_DIRECT_V4L2 in capturethread.h.
It will then not get any timestamp (opencv version requires a bug fix).

# uvcvideo driver quirks
<br/> rmmod uvcvideo
<br/> then re-load it with the quirks parameter. assuming you want to enable both:
<br/> UVC_QUIRK_FIX_BANDWIDTH (which has the hex-value 0x80, which is 128 in decimal) and
<br/> UVC_QUIRK_RESTRICT_FRAME_RATE (which is 0x200 thus 512)
<br/> you would use a quirks value of 640 (which is 128+512 resp. 0x200|0x80):
<br/> modprobe uvcvideo quirks=640



