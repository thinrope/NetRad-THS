A repositroy to host firmware/software related to the DIY Geiger-Mueller
counter units.

http://www.tokyohackerspace.org/en/event/diy-geiger-mueller-counter-workshop-20-2011-08-08
http://www.tokyohackerspace.org/en/event/diy-geiger-mueller-counter-workshop-20-2011-08-22

See this for more insights on the history of this kit:
http://tokyohackerspace.org/en/blog/tokyo-hackerspacerdtn-geiger-shield-dev-history

This kit is a THS[1] project in collaboration with SafeCast[2]. Once you buy
and build your device, you own it (hardware and software) and the data it
produces. You can opt-in to have your data logged to the THS account on
Pachube[3] (for free) and have it available to the public (for free).

[1]	http://tokyohackerspace.org/
[2]	http://safecast.org/
[3]	http://pachube.com/feeds?user=tokyohackerspace





To be able to update/change the firmware of your DIY GM counter, you'll need:
a) a toolchain to build (compile/link) the source for the Arduino
b) a tool to upload the binary to the flash of the Arduino
c) way to access the source code and related libraries

One relatively simple way is to download the appropriate "Arduino IDE" for your
OS of choice from http://arduino.cc/en/Main/Software since this provides both
a) and b) above.

If you are not into development just get a stable snapshot from
https://github.com/thinrope/NetRad-THS, otherwise use your favorte git tool to
improve the source and send pull requests.

To have your unit upload data to http://pachube.com/ you need to setup an
account with them, or use the THS shared account. To use the THS shared
account, please contact kalin@safecast.org and you'll be given an ID.

THIS IS A WORK IN PROGRESS, PLEASE BEAR WITH ME!
Feel free to submit issues on github, or contact the developers directly.


To use the bleeding edge, on Linux:

cd sketchbook
git clone https://github.com/thinrope/NetRad-THS.git
cd NetRad-THS/
git checkout testing
git submodule init
git submodule update
cd src/netrad_ths
make
make upload
make monitor
