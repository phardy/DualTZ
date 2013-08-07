#!/bin/bash
#
# This script relies on the TZ database *source files*. Grab the
# data distribution from http://www.iana.org/time-zones , extract it
# somewhere, and point the TZSRCDIR variable at them.
# Parsed files are correct for the 2013d release. Check and modify
# for other releases.

TZSRCDIR=~/src/tz
TZSRCFILES="$TZSRCDIR/africa $TZSRCDIR/antarctica $TZSRCDIR/asia $TZSRCDIR/australasia $TZSRCDIR/europe $TZSRCDIR/northamerica $TZSRCDIR/southamerica"
DESTDIR="../DualTZ-selector/resources/src/TZfiles"
allzones=`tempfile`

awk -f parseZones.awk $TZSRCFILES | grep "/" > $allzones

# Parse links
links=`tempfile`
awk '/^Link/ { print $2" "$3 }' $TZSRCFILES > $links
while read link; do # $links
    src=`echo $link | cut -d " " -f 1`
    dst=`echo $link | cut -d " " -f 2`
    srctime=`grep $src $allzones | cut -d " " -f 2`
    echo "$dst $srctime" >> $allzones
done < $links
rm $links

sortedzones=`tempfile`
sort < $allzones > $sortedzones
mv $sortedzones $allzones

# Split out to final files
rm $DESTDIR/*.txt > /dev/null 2>&1
while read line; do # $allzones
    tzname=`echo $line | cut -d " " -f 1`
    tzoffset=`echo $line | cut -d " " -f 2`

    tzbasename=`echo $tzname | cut -d "/" -f 1`
    shorttzname=`echo $tzname | cut -d "/" -f 2`

    houroffset=`echo $tzoffset | cut -d ":" -f 1`
    minoffset=`echo $tzoffset | cut -d ":" -f 2`
    x=${houroffset:0:1}
    if [ $x == "-" ]; then
	minoffset="-$minoffset"
    fi
    let "secoffset = (($houroffset * 60) + $minoffset) * 60"

    echo "$shorttzname $tzoffset $secoffset" >> $DESTDIR/$tzbasename.txt
done < $allzones

rm $allzones
