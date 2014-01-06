function appMessageAck(e) {
    console.log('options sent to Pebble successfully');
}

function appMessageNack(e) {
    console.log('options not sent to Pebble: ' + e.error.message);
}

function sendConfigToWatch(config) {
    var watchconfig = new Object();
    watchconfig.remote-tz-name = config.timezone.name;
    watchconfig.remote-tz-offset = config.timezone.remote-offset;

    var now = new Date();
    // Wow, getTimezoneOffset() is the weirdest thing ever.
    var localtzoffset = now.getTimezoneOffset() * -60;
    watchconfig.local-tz-offset = localtzoffset;

    Pebble.sendAppMessage(watchconfig, appMessageAck, appMessageNack);
}

Pebble.addEventListener("showConfiguration", function() {
    console.log('showing configuration');
    Pebble.openURL('http://hardy.dropbear.id.au/DualTZ/config/3-0.html');
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log('configuration closed');
    if (e.response != '') {
	var params = JSON.parse(decodeURIComponent(e.response));
	var newconfig = new Object();
	newconfig.timezone = new Object();
	if (params.hasOwnProperty("utc") && params.utc) {
	    newconfig.utc = params.utc;
	    newconfig.timezone.remote_tz_name = "UTC";
	    newconfig.timezone.remote_tz_offset = 0;
	} else if (params.hasOwnProperty("timezone")) {
	    var timezone = params.timezone;
	    // decodeURIComponent seems to have trouble with spaces,
	    // leaving them as a plus. So we remove them here.
	    if (timezone.hasOwnProperty("remote_tz_name")) {
		var name = timezone.remote_tz_name.replace('+', ' ');
		newconfig.timezone.remote_tz_name = name;
	    }
	    if (timezone.hasOwnProperty("adminname")) {
		var adminname = timezone.adminname.replace('+', ' ');
		newconfig.timezone.adminname = adminname;
	    }
	    if (timezone.hasOwnProperty("country")) {
		var country = timezone.country.replace('+', ' ');
		newconfig.timezone.country = country;
	    }
	    if (timezone.hasOwnProperty("tz")) {
		var tz = timezone.tz.replace('+', ' ');
		newconfig.timezone.tz = tz;
	    }
	    if (timezone.hasOwnProperty("remote_tz_offset")) {
		newconfig.timezone.remote_tz_offset = timezone.remote_tz_offset;
	    }
	}
	if (params.hasOwnProperty("bluetooth")) {
	    var bluetooth = params.bluetooth;
	    newconfig.bluetooth = bluetooth;
	}
	if (params.hasOwnProperty("lowbat")) {
	    var lowbat = params.lowbat;
	    newconfig.lowbat = lowbat;
	}
	console.log(JSON.stringify(newconfig));
	window.localStorage.setItem('config', newconfig);

	sendConfigToWatch(newconfig);
	Pebble.sendAppMessage(params, appMessageAck, appMessageNack);
    } else {
	console.log('no options received');
    }
});
