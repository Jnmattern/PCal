var lang = 0;
var weekstart = 0;
var nwd_country = 0;
var showweeknum = 0;

function logVariables() {
	console.log("	lang: " + lang);
	console.log("	weekstart: " + weekstart);
	console.log("	nwd_country: " + nwd_country);
	console.log("	showweeknum: " + showweeknum);
}

Pebble.addEventListener("ready", function() {
	console.log("Ready Event");
	
	lang = localStorage.getItem("lang");
	if (!lang) {
		lang = 1; // Default: English
	}

	weekstart = localStorage.getItem("weekstart");
	if (!weekstart) {
		weekstart = 0; // Default: Week starts on sunday
	}
	
	nwd_country = localStorage.getItem("nwd_country");
	if (!nwd_country) {
		nwd_country = 0; // Default: Don't show Non Working Days
	}
	
	showweeknum = localStorage.getItem("showweeknum");
	if (!showweeknum) {
		showweeknum = 1; // Default: show Week Number
	}
	
	logVariables();
						
	Pebble.sendAppMessage(JSON.parse('{"lang":'+lang+',"weekstart":'+weekstart+',"nwd_country":'+nwd_country+',"showweeknum":'+showweeknum+'}'));

});

Pebble.addEventListener("showConfiguration", function(e) {
	console.log("showConfiguration Event");

	logVariables();
						
	Pebble.openURL("http://www.famillemattern.com/jnm/pebble/PCal/PCal_3.3.html?" +
				   "lang=" + lang +
				   "&weekstart=" + weekstart +
				   "&nwd_country=" + nwd_country +
				   "&showweeknum=" + showweeknum  );
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("Configuration window closed");
	console.log(e.type);
  console.log("Response: " + decodeURIComponent(e.response));

	var configuration = JSON.parse(decodeURIComponent(e.response));
	Pebble.sendAppMessage(configuration);
	
	lang = configuration["lang"];
	localStorage.setItem("lang", lang);
	
	weekstart = configuration["weekstart"];
	localStorage.setItem("weekstart", weekstart);

	nwd_country = configuration["nwd_country"];
	localStorage.setItem("nwd_country", nwd_country);

	showweeknum = configuration["showweeknum"];
	localStorage.setItem("showweeknum", showweeknum);
});
