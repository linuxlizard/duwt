var mysurvey = new Survey("/api/survey?decode=full");
var survey_table;

$(document).ready(function() {
    survey_table = $('#example').DataTable( {
        columns: [
            { width: "20%", title: "SSID" },
            { title: "BSSID" },
            { title: "freq" },
            { title: "dBm" },
            { title: "mode" },
            { title: "width" },
            { title: "security" },
        ]
    } );

	mysurvey.fetch().then(survey => { 
		console.log("survey done. survey=", survey); 
		survey.forEach(network => survey_table.row.add( 
					[escapeHTML(network.ssid), network.bssid, String(network.freq), 
					 String(network.dbm), network.mode, network.chwidth, 
					 network.security]
					).draw(false)
				);
	});
} );
