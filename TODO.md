# TODO

* We don't get notification if the TZInfo object isn't on the watch.
  This means the watchface never updates past localtime, and the selector
  looks ugly. Would be nice to introduce a timeout.
* Nicer looking font.
* Despite how I configure the minute and hour hand updating, it seems
  to happen regularly: minute hand shifts every ten seconds no matter
  what. Does that mean it's refreshing every tick?
* Add an AM/PM text layer for 12h mode.
