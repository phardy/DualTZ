# DualTZ

A Pebble watchface for showing local time, and the time in a
user-configurable timezone.

Requires version 2 of the Pebble SDK.

## Server side component

This watchface uses the configuration page feature of Pebblekit JS. The
config page and dependencies are in the html directory.

It currently uses cloud-hosted jQuery mobile. It also makes
use of the tzdata-javascript library, with a local cache of zonefiles.
