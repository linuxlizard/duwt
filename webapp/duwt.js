//import "soup.json"

const myHeading = document.querySelector('h1');
myHeading.textContent = 'Hello world!';

$(document.body).append("<div><h1>Greetings</h1><p>Yoshi here</p></div>")

console.log("hello, world")

$(document).ready(function() {
	var q = {url:"/api/survey", type:"GET", dataType:"json"}
	var done = function(json) {
		console.log(json)
	}
	$.ajax(q).done(done)

})

