# TODO

* We don't get notification if the TZInfo object isn't on the watch.
  This means the watchface never updates past localtime, and the selector
  looks ugly. Would be nice to introduce a timeout.
* Nicer looking font.
* Despite how I configure the minute and hour hand updating, it seems
  to happen regularly: minute hand shifts every ten seconds no matter
  what. Does that mean it's refreshing every tick?
* Refactor TZInfo, remove text offset and regenerate from seconds when
  required. Seems like the easiest way to give visual feedback of actual
  offset when DST is active.
* Find a bigger source for TZ data. Or at least sort the one I have nicer.
