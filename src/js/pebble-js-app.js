/*global console: false*/
/*global localStorage: false*/
/*global Pebble: false*/

function appMessageAck() {
    "use strict";
    console.log('options sent to Pebble successfully');
}

function appMessageNack(e) {
    "use strict";
    console.log('options not sent to Pebble: ' + e.error.message);
}

function sendConfigToWatch(config) {
    "use strict";
    var watchconfig = {},
        now = new Date(),
        localtzoffset = now.getTimezoneOffset() * -60;
    watchconfig.remote_tz_name = config.timezone.remote_tz_name;
    watchconfig.remote_tz_offset = config.timezone.remote_tz_offset;
    // Booleans not supported in AppMessages
    if (config.bluetooth) {
        watchconfig.btdisco_notification = 1;
    } else {
        watchconfig.btdisco_notification = 0;
    }
    if (config.lowbat) {
        watchconfig.lowbat_notification = 1;
    } else {
        watchconfig.lowbat_notification = 0;
    }

    watchconfig.local_tz_offset = localtzoffset;
    console.log("local_tz_offset from phone: " + localtzoffset);
    console.log("sending config to watch");
    Pebble.sendAppMessage(watchconfig, appMessageAck, appMessageNack);
}

Pebble.addEventListener("showConfiguration", function() {
    "use strict";
    console.log('showing configuration');
    var currentconfig = localStorage.getItem("config"),
        URL = "http://hardy.dropbear.id.au/DualTZ/config/3-1.html";
    if (currentconfig === null) {
        console.log("no stored data found");
    } else {
        console.log("calling config with " + currentconfig);
        URL += "?data=" + encodeURIComponent(currentconfig);
    }
    Pebble.openURL(URL);
});

Pebble.addEventListener("webviewclosed", function(e) {
    "use strict";
    console.log('configuration closed');
    if (e.response !== '') {
        var params = JSON.parse(decodeURIComponent(e.response)),
            newconfig = {},
            timezone = null;
        newconfig.timezone = {};
        if (params.hasOwnProperty("utc") && params.utc) {
            newconfig.utc = params.utc;
            newconfig.timezone.remote_tz_name = "UTC";
            newconfig.timezone.remote_tz_offset = 0;
        } else if (params.hasOwnProperty("timezone")) {
            timezone = params.timezone;
            // decodeURIComponent seems to have trouble with spaces,
            // leaving them as a plus. So we remove them here.
            if (timezone.hasOwnProperty("remote_tz_name")) {
                newconfig.timezone.remote_tz_name = timezone.remote_tz_name.replace('+', ' ');
            }
            if (timezone.hasOwnProperty("adminname")) {
                newconfig.timezone.adminname = timezone.adminname.replace('+', ' ');
            }
            if (timezone.hasOwnProperty("country")) {
                newconfig.timezone.country = timezone.country.replace('+', ' ');
            }
            if (timezone.hasOwnProperty("tz")) {
                newconfig.timezone.tz = timezone.tz.replace('+', ' ');
            }
            if (timezone.hasOwnProperty("remote_tz_offset")) {
                newconfig.timezone.remote_tz_offset = timezone.remote_tz_offset;
            }
        }
        if (params.hasOwnProperty("bluetooth")) {
            newconfig.bluetooth = params.bluetooth;
        }
        if (params.hasOwnProperty("lowbat")) {
            newconfig.lowbat = params.lowbat;
        }
        console.log(JSON.stringify(newconfig));
        console.log("saving config");
        localStorage.setItem('config', JSON.stringify(newconfig));

        sendConfigToWatch(newconfig);
        Pebble.sendAppMessage(params, appMessageAck, appMessageNack);
    } else {
        console.log('no options received');
    }
});
