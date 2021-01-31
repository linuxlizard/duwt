function ieee80211_frequency_to_channel(freq)
{
	// math from iw util.c 
	// https://git.kernel.org/pub/scm/linux/kernel/git/jberg/iw.git
	/* see 802.11-2007 17.3.8.3.2 and Annex J */
	if (freq == 2484)
		return 14;
	else if (freq < 2484)
		return (freq - 2407) / 5;
	else if (freq >= 4910 && freq <= 4980)
		return (freq - 4000) / 5;
	else if (freq <= 45000) /* DMG band lower limit */
		return (freq - 5000) / 5;
	else if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;
	else
		return 0;
}

// https://stackoverflow.com/questions/3043775/how-to-escape-html
function escapeHTML(str){
    var p = document.createElement("p");
    p.appendChild(document.createTextNode(str));
    return p.innerHTML;
}

class Survey {
	constructor(url){
		this.url = url;
		this.survey = [];
	}

	IE_RSN = 48;

	length() { return this.survey.length; }

	decode_rsn(network) { 
		// convert the RSN IE to a simple "Security" string
		if (!("IE" in network)) {
			return "unknown";
		}

		// search for the RSN IE
		var rsn_ie = $.grep(network.IE, ie => ie.id == this.IE_RSN);
		if (!rsn_ie.length) {
			console.log("rsn_ie=",rsn_ie);
			return "Open";
		}

		// grep returns array so should convert our one element array into a
		// single value for convenience
		rsn_ie = rsn_ie.shift();

		var auth = [];
		if ($.grep(rsn_ie.auth, a => a.includes("PSK")).length) {
			auth = auth.concat("PSK");
		}
		if ($.grep(rsn_ie.auth, a => a.includes("802.1X")).length) {
			auth = auth.concat("8021.X");
		}

		// search for TKIP and CCMP which indicates WPA1 / WPA2
		var cipher = "";
		if (rsn_ie.pairwise.includes("TKIP")) {
			cipher += "WPA1";
		}
		if (rsn_ie.pairwise.includes("CCMP")) {
			cipher += "WPA2";
		}

		if (cipher) {
			return cipher + "/" + auth.join(",");
		}

		console.log("rsn_ie=",rsn_ie);
		return "unknown/todo";
	}

	fetch() {
		return fetch(this.url)
			.then(response => { console.log("controller response",response)
								if (! response.ok || response.headers.get("content-type") != "application/json") {
									throw new TypeError("get survey failed"); 
								}
								return response.json();
							})
			.then(json => { console.log("json=",json);
							var new_survey = $.map(json, (network,idx) => {
										return {
											date: new Date(),
											"security": this.decode_rsn(network),
											...network,
											};
									});
							this.survey = new_survey;

							return Promise.resolve(new_survey);
						  })
	}
}


