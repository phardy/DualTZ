function appMessageAck(e) {
    console.log('options sent to Pebble successfully');
}

function appMessageNack(e) {
    console.log('options not sent to Pebble: ' + e.error.message);
}

Pebble.addEventListener("showConfiguration", function() {
    console.log('showing configuration');
    Pebble.openURL('http://hardy.dropbear.id.au/DualTZ/config/2-0.html');
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log('configuration closed');
    if (e.response != '') {
	var params = JSON.parse(decodeURIComponent(e.response));
	// decodeURIComponent seems to have trouble with spaces,
	// leaving them as a plus. So we remove them here.
	var name = params['remote-tz-name'].replace('+', ' ');
	params['remote-tz-name'] = name;
	console.log(JSON.stringify(params));
	window.localStorage.setItem('remote-tz-name' + params['remote-tz-name']);
	window.localStorage.setItem('remote-tz-offset' + params['remote-tz-offset']);
	window.localStorage.setItem('local-tz-offset' + params['local-tz-offset']);
	Pebble.sendAppMessage(params, appMessageAck, appMessageNack);
    } else {
	console.log('no options received');
    }
});
