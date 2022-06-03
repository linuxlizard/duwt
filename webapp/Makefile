# just copy the web app over to the httpserv dir

SERVER=../build/files

# using Bootstrap but its CDN doens't have the full jquery with AJAX so
# bringing along my own
JS=duwt.js soup.js bssid.js jquery-3.5.1.min.js jquery-3.5.1.js sample.js datatable.js survey.js

# index.html from https://getbootstrap.com/docs/4.5/getting-started/introduction/#starter-template
HTML=simple.html soup.html

VERBOSE=
#VERBOSE=-v

#JSGRID=$(SERVER)/plugins/jsgrid/jsgrid.js

all: $(SERVER) js html $(JSGRID)

$(SERVER):
	mkdir -p $(SERVER)

js: $(foreach src,$(JS),$(SERVER)/$(src))

html: $(foreach src,$(HTML),$(SERVER)/$(src))

$(dir $(JSGRID)):
	echo dir=$(dir $(JSGRID))
	mkdir -p $@

$(JSGRID) : $(dir $(JSGRID)) 
	cd $(JSGRID) && unzip ../../../../webapp/jsgrid-1.5.3.zip && touch jsgrid.js

$(SERVER)/%: %
	@cp $(VERBOSE) $< $@

clean:
	$(RM) $(SERVER)/*

.PHONY: js html

