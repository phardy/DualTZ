# DualTZ

A Pebble watchface for showing local time, and the time in a
user-configurable timezone.

Installation and usage directions on the [mypebblefaces.com page](http://www.mypebblefaces.com/view?fID=5432&aName=stibbons&pageTitle=DualTZ&auID=6748).

Before building, edit common/config.h and check the ANDROID define. Leaving
this defined will build the watchface and standard app with different UUIDs.
Comment it out to build both apps with the same UUID, which is required to
use httpebble on iOS devices.
