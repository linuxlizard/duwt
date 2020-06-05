# just copy the web app over to the httpserv dir

server=../build/public

# using Bootstrap but its CND doens't have the full jquery with AJAX so
# bringing along my own
JS=duwt.js jquery-3.5.1.min.js

HTML=index.html

all: js html

js: $(foreach src,$(JS),$(server)/$(src))

html: $(foreach src,$(HTML),$(server)/$(src))

$(server)/%: %
	cp $< $@

clean:
	$(RM) $(server)/*
