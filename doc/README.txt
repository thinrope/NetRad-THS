		DIY GM counter @TokyoHackerSpace
			by Kalin KOZHUHAROV

1. Intro

Due to the short supply of GM counters after the 2011-03-11 disaster in
Fukushima, Japan, we at TokyoHackerSpace decided to build our own. Initial
design, prototyping and testing was done by Akiba, see his blog for more info:
http://tokyohackerspace.org/en/blog/tokyo-hackerspacerdtn-geiger-shield-dev-history

2. The hardware

--------------------      ----------        ~~~~~~~~~~~~
| FREAKDUINO-CHIBI | ==== | NetRAD | ----  0   SBM-20   0
--------------------      ----------        ~~~~~~~~~~~~

The FREAKDUINO-CHIBI (MCU board) and NetRAD (shield) are open hardware
licensed under Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0):
	http://creativecommons.org/licenses/by-sa/3.0/


As of October 2011, we are using FREAKDUINO-CHIBI-v1.1 (ATmega328P @ 8MHz),
NETRAD-v1.0 boards and SBM-20 GM tubes.

FREAKDUINO-CHIBI
	http://www.freaklabsstore.com/pub/FREAKDUINO-CHIBI%20v1.1%20Datasheet.pdf
	http://freaklabs.org/index.php/Blog/Chibi/Assembling-and-Setting-Up-the-Freakduino-Chibi.html

ATmega328P
	http://www.atmel.com/dyn/products/product_card.asp?part_id=4198

NETRAD
	http://github.com/thinrope/NetRad-THS/blob/master/doc/NetRAD-v1.0.pdf?raw=true
	http://github.com/thinrope/NetRad-THS/blob/master/doc/NetRAD-v1.0_assembly.pdf?raw=true
SBM-20
	http://www.gstube.com/data/2398/


3. The firmware
	https://github.com/thinrope/NetRad-THS


4. Calibration

Every GM counter counts only counts :-) In other words, to be able to compare
the readings of your DIY GM counter to other units, it has to be calibrated to
say uSv/h. This is a hard to do procedure, but there is a way to do it. Please
stay tuned...


5. Publishing your data

As of August 2011 we hope that all the GIY GM counters will be put online in
our Pachube account. To do that you'll get a feed_ID set up on your unit and
will be able to upload data and use it off Pachube.


6. Further info

Please address any firmware/software concerns through the issue tracking
system of github.  Other general questions can probably be answered by a
TokyoHackerSpace member (see http://groups.google.com/group/tokyohackerspace/)
or SafeCast-Japan (see http://groups.google.com/group/safecast-japan/).

