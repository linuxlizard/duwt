# just copy the web app over to the httpserv dir

SERVER=../build/files

# using Bootstrap but its CDN doens't have the full jquery with AJAX so
# bringing along my own
JS=duwt.js bssid.js jquery-3.5.1.min.js jquery-3.5.1.js sample.js

# index.html from https://getbootstrap.com/docs/4.5/getting-started/introduction/#starter-template
# starter.html from AdminLTE https://adminlte.io
HTML=index.html starter.html bssid.html

all: $(SERVER) js html

$(SERVER):
	mkdir -p $(SERVER)

js: $(foreach src,$(JS),$(SERVER)/$(src))

html: $(foreach src,$(HTML),$(SERVER)/$(src))

$(SERVER)/%: %
	@cp -v $< $@

clean:
	$(RM) $(SERVER)/*
