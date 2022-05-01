# DUWT
Dave's Useful WiFi Tool(s)

# Introduction

Goal is a web-based application like [WiFi Explorer](https://www.adriangranados.com/).  
I started with a Qt application but switched to Web. Back end uses microhttpd
and is written in plain C.

This is a workshop. There are a lot of half-finished/half-started projects
here. I'm using this project to learn modern C++17 and front end web
development.

## Stuff

The Python dir contains a simple [nl80211](https://wireless.wiki.kernel.org/en/developers/Documentation/nl80211) module.

The pyroute2 dir is my additions + fixes for nl80211 in [pyroute2](https://pypi.org/project/pyroute2/)

The full C++17 implemenation was too heavy for firmware, so backed up and made
a C-only version that lives in libiw. Contains "scan-dump", a super useful
cmdline utility to dump the scan results, one BSS per line.

The back-end for the web application is in httpserv and is written in C.
Uses [microhttpd](https://www.gnu.org/software/libmicrohttpd/)

# Packages Used
 - http://www.digip.org/jansson/
 - [Linux 'iw'](https://git.kernel.org/pub/scm/linux/kernel/git/jberg/iw.git)
 - https://jquery.com/
 - https://getbootstrap.com/
 - https://underscorejs.org/
 - http://js-grid.com/

# Licenses

## Contains chunks of 'iw'
Copyright (c) 2007, 2008        Johannes Berg
Copyright (c) 2007              Andy Lutomirski
Copyright (c) 2007              Mike Kershaw
Copyright (c) 2008-2009         Luis R. Rodriguez

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

## list.h
Doubly-linked list
Copyright (c) 2009-2019, Jouni Malinen <j@w1.fi>

This software may be distributed under the terms of the BSD license.
See README for more details.

Copyright (c) 2009-2019, Jouni Malinen <j@w1.fi>
