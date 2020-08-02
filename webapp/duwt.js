//import "soup.json"

const myHeading = document.querySelector('h1');
myHeading.textContent = 'Hello world!';

class Survey {
	constructor(url){
		this.url = url;
		this.survey = [];
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
											...network,
											};
									});
							this.survey = new_survey;
							return Promise.resolve(new_survey);
						  })
	}

}


// playground for learning wtf I'm doing
$(document).ready(function() {
	console.log("hello, world");
	console.log("survey=",window.survey);

	$.each(window.survey, (idx,network) => console.log(network));

	var new_survey = $.map(window.survey, (network,idx) => {
				return {
					savebssid:network.bssid,
					...network,
					bssid:`<a href=/bssid/${network.bssid}>${network.bssid}</a>`,
					};
			});

	var survey = new Survey("/api/survey");
	var survey_promise = survey.fetch();
	console.log("promise=",survey_promise);
	console.log("survey=",survey.survey);
	var table = $("#table-duwt");
	var tablebody = $("#table-duwt > tbody");

	var filltable = function(survey) {
		console.log("survey complete; survey=", survey); 
		console.log("table=",table);
		console.log("this=",this);
		$.each( survey, function(index,value) {
			console.log(`${value.bssid} ${value.ssid} ${value.freq}`);
			// BSSID SSID Frequency
			tablebody.append(
					"<tr>" + 
						`<td>${value.bssid}</td>` +
						`<td>${value.ssid}</td>` + 
						`<td>${value.freq}</td>` + 
					"</tr>");
				});
	}

	// assuming I'm running from index.html which has a simple table; update
	// the table
	survey_promise.then(filltable.bind(tablebody));
//	survey_promise.then( survey => { 
//				console.log("survey complete; survey=", survey); 
//				console.log("table=",table);
//			} );

})


