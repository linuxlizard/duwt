//var mysurvey = new Survey("/api/survey");

function document_get_bssid() {
	var url = new URL(document.baseURI);
	return url.searchParams.get("bssid");
}

class BSS {
	constructor(bssid) {
		this.bssid = encodeURI(bssid);
		this.json = "";
	}

	fetch() {
		return fetch("/api/bssid?bssid=" + this.bssid)
			.then(response => { 
					console.log("bssid response",response)
					if (! response.ok || response.headers.get("content-type") != "application/json") {
						throw new TypeError("get bssid failed"); 
					}
					return response.json();
				})
			.then(json => { 
					console.log("json=",json);
					this.json = json;
					return Promise.resolve(json);
			});
	}
};

var bss = new BSS(document_get_bssid());
bss.fetch();

$(function() {
	$("#title").text(document_get_bssid());
});


