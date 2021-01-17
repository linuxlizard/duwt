
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

class Survey {
	constructor(url){
		this.url = url;
		this.survey = [];
	}

	length() { return this.survey.length; }

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
											...network,
											};
									});
							this.survey = new_survey;
							$("#jsGrid1").jsGrid("loadData");
							return Promise.resolve(new_survey);
						  })
	}
}

var mysurvey = new Survey("/api/survey");
mysurvey.fetch();
var survey_grid;

$(function () {
	survey_grid = $("#jsGrid1").jsGrid({
		height: "auto",
		width: "100%",
 
		sorting: true,
		paging: true,
		autoload: true,
		filtering: true,
 
		onDataLoaded: function(grid,data) {
				console.log("onDataLoaded data=",data," grid=",grid);
				},

		controller: { 
			loadData: function(filter) {
				console.log("loadData filter=",filter);
				if (mysurvey.length() > 0) {
					console.log("use cache");
					var filtered = $.grep(mysurvey.survey, network => {
								return (!filter.ssid || network.ssid.indexOf(filter.ssid) > -1)
									&& (!filter.bssid || network.bssid.indexOf(filter.bssid) > -1);
							});
					// save the actual bssid then create a link to a bssid page
					var new_survey = $.map(filtered, (network,idx) => {
								return {
									savebssid: network.bssid,
									...network,
									bssid:`<a href=/bssid/${network.bssid}>${network.bssid}</a>`,
									};
							});
					return new_survey;
				}
			},
		},
 
		fields: [
			{ name: "ssid", type: "text", width: 100 },
			{ name: "bssid", type: "text", width: 75  },
			{ name: "freq", type: "number", width: 50 },
			{ name: "dbm", type: "number", width: 50 },
		]
	});
});

