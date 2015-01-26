These are poolers for meterN (http://metern.org/) to collect and render data.

There are 2 blocks of files:
 1. pooler485.php and poolmeters485.sh
 2. pooler485.sh

First ones are the first version, the latter is the last version.

To use pooler485.sh, create a symbolic link
<PRE>ln -s /var/www/metern/comapps /usr/local/bin/pooler485 
</PRE>
then add in /etc/rc.local
<PRE>
touch /run/shm/metern1.txt
chown www-data:www-data /run/shm/metern1.txt
pooler485 1 9600 /dev/ttyUSB0&
</PRE>
where 1 is RS485 address (and meterN ID), 9600 port speed, /dev/ttyUSB0 is USB-RS485 device

In meter configuration use
<PRE>more /run/shm/metern1.txt | egrep "^1\(" | grep "*Wh)"</PRE>
for energy and
<PRE>more /run/shm/metern1.txt  | egrep "^1\(" | grep "*W)"</PRE>
for live power

You can use more meters on the same bus, setting meters with different addresses.
MeterN configuration is the same, dimply change 1 with 2, 3, etc.

NOTE: meterN ID must be equal to RS485 address
