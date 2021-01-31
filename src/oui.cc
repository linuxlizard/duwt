/*
 * oui.cc
 *
 * parse oui files in CSV or other formats
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <iostream>
#include <fstream>
#include <iomanip>

#include <cstring>

#include "oui.h"

// OUI sources:
//
// nmap  /usr/share/nmap/nmap-mac-prefixes
// Ubuntu: apt install nmap
// Fedora: dnf install nmap
//
// oui.csv  
// http://standards-oui.ieee.org/oui/oui.csv
//
// hwdata /usr/share/hwdata/oui.txt
// Ubuntu: apt install hwdata
// Fedora: dnf install hwdata
//
// Flat files:
// http://standards-oui.ieee.org/oui/oui.txt
//
// TODO add support for Wireshark
// https://gitlab.com/wireshark/wireshark/-/raw/master/manuf

using namespace ieeeoui;

//#define DEBUG
//#define DEBUG_CSV
//#define DEBUG_MA

OUI::OUI(std::string filename_in)
	: filename(filename_in),
	  infile(filename_in)
{
	if (!infile) {
		std::string errmsg { "no such file: " };
		errmsg += filename;
		throw OUIException(errmsg);
	}

#ifdef DEBUG
	std::clog << "open " << filename << "\n";
#endif
}

std::string OUI::get_org_name(uint32_t oui)
{
	try { 
		return lut.at(oui);
	} 
	catch (std::out_of_range& err) {
		// seekg() will clear the 'eofbit' but I have to clear 'badbit' 
		infile.clear();
		infile.seekg(0);

		// lookup will also throw out_of_range if not found
		std::string org = lookup(oui);

		// save in cache
		lut[oui] = org;
		return org;
	}
}

std::string OUI::get_org_name(uint8_t* oui)
{
	uint32_t n;

	// TODO big endian! (currently testing on Intel)
	n = (oui[0]<<16) | (oui[1]<<8) | oui[2];
	return get_org_name(n);
}



OUI_CSV::OUI_CSV(std::string filename_in): OUI(filename_in)
{
	std::string line;

	// verify this is a MA CSV file
	std::getline(infile, line);

	// look for leading string "Registry,"
#if __cplusplus >= 202002L
	if ( !line.starts_with("Registry,"))  { 
		throw std::invalid_argument(filename);
	}
#else
	if ( line.length() < 32 || !(line.substr(0,9) == "Registry,")) {
		throw std::invalid_argument(filename);
	}
#endif
}

uint32_t OUI_CSV::parse_oui(std::string const& s)
{
	// only find the OUI & convert to integer
	return std::stoul(s.substr(5,6), nullptr, 16);
}

std::string OUI_CSV::parse_org(std::string const& s)
{
	// Have to be very careful because quoted string can contain embedded commas.
	// State machine FTW.

#ifdef DEBUG_CSV
	std::clog << "parse_org s=" << s << "\n";
#endif

	size_t start = 12;
	size_t pos = 12;
	size_t len = s.length();

	if (s[pos] == '"') {
		start += 1;
		State state {State::QUOTED_STRING};

		// quoted string possibly containing embedded commas
		while (++pos < len) {
			if (s[pos] == '\\') {
				// backslash; next char is escaped
				state = State::ESCAPE;
			}
			else if (s[pos] == '"') {
				if (state == State::ESCAPE) {
					state = State::QUOTED_STRING;
				}
				else {
					// hopefully close quote
					// Done!
					break;
				}
			}
		}
	}
	else {
		// simple case - non quoted string so search for first comma
		while (++pos < len && s[pos] != ',') {
		}
	}
#ifdef DEBUG_CSV
	std::clog << "parse_org found \"" << s.substr(start,pos-start) << "\"\n";
#endif
	return s.substr(start,pos-start);
}

std::string OUI_CSV::lookup(uint32_t oui)
{
	std::string line;
	size_t counter { 0 };

#ifdef DEBUG_CSV
	std::clog << "lookup 0x" << std::setw(8) << std::setfill('0') << std::hex << oui << "\n";
#endif

	while(std::getline(infile, line)) {
		counter++;
#ifdef DEBUG_CSV
		std::clog << std::dec << "counter=" << counter << " len=" << line.length() << "\n";
#endif
		if (line.length() < 32) {
			continue;
		}

		// verify starts with MA-L,
		if (line[0] != 'M' || line[1] != 'A' || line[2] != '-' || line[3] != 'L' || line[4] != ',') {
			continue;
		}

		uint32_t num = parse_oui(line);
#ifdef DEBUG_CSV
		std::clog << "oui=" << std::hex << num << "\n";
#endif
		if (num == oui) {
			return parse_org(line);
		}
	}

	throw std::out_of_range(oui_to_string(oui));
}

OUI_MA::OUI_MA(std::string filename_in) 
	: OUI(filename_in)
{
	std::string line;

	// verify this is a OUI/MA file
	std::getline(infile, line);
	if ( line.length() < 32 || line[0] != 'O' || line[1] != 'U' || line[2] != 'I' || line[3] != '/' || line[4] != 'M' || line[5] != 'A') {
		throw std::invalid_argument(filename);
	}

}

std::string OUI_MA::parse_org(std::string const& s)
{
	std::string org;

	size_t pos, len = s.length();
	State state {State::WHITESPACE};

#ifdef DEBUG_MA
	std::clog << "parse_org s=" << s << "\n";
#endif

	for (pos=6 ; pos<len && state != State::FOUND_ORG ; pos++) {
#ifdef DEBUG_MA
		std::clog << "parse_org pos=" << pos << " char=" << s[pos] << " state=" << static_cast<int>(state) << "\n";
#endif
		switch (state) {
			case State::WHITESPACE:
				if (s[pos] == '(') {
					state = State::PAREN_EXPR;
				}
				break;
			case State::PAREN_EXPR:
				if (s[pos] == ')') {
					state = State::SEEK_ORG;
				}
				break;
			case State::SEEK_ORG:
				if (s[pos] != '\t' && s[pos] != ' ') {
					state = State::FOUND_ORG;
				}
				break;
			case State::FOUND_ORG:
				break;
		}
	}

	// we are one char off because of one extra pos++ in the above loop
	pos--;

	if (state != State::FOUND_ORG) {
		throw std::invalid_argument(s);
	}

	// strip \r\n or \n
	len--;
	while ( (s[len] == '\r' || s[len] == '\n') && len != 0) {
		len--;
	}
	return s.substr(pos,len);
}

std::string OUI_MA::lookup(uint32_t oui)
{
	std::string line;
	size_t counter { 0 };

#ifdef DEBUG_MA
	std::clog << "lookup 0x" << std::setw(8) << std::setfill('0') << std::hex << oui << "\n";
#endif

	std::string oui_s = oui_to_string(oui, false);
	const char* oui_ptr = oui_s.data();

	while(std::getline(infile, line)) {
		counter++;

		// super simple check to look for lines starting with hex HHHHHH<space>
		if (line[0] == '\t' || line.length() < 6 || line[6] != ' ') {
			// ignore
			continue;
		}

#ifdef DEBUG_MA
		std::clog << std::dec << "line=" << counter << " line=" << line.substr(0,7) << " oui=" << oui_s << "\n";
#endif
		if (memcmp(line.data(), oui_ptr, 6) == 0) {
			return parse_org(line);
		}
	}

#ifdef DEBUG_MA
	std::clog << "unable to find oui=" << oui_s << " in " << filename << "\n";
#endif
	throw std::out_of_range(oui_s);
}


std::string ieeeoui::oui_to_string(uint32_t oui, bool zedx)
{
	// poTAYto poTAHto (did both ways to see how the ostreamstream works)
#if 1
	char oui_str[12] {};
	if (zedx) {
		snprintf(oui_str, 11, "%#08X", oui);
	}
	else {
		snprintf(oui_str, 11, "%06X", oui);
	}

	return std::string { oui_str };
#else
	std::ostringstream s;
	s << std::setw(6) << std::setfill('0') << std::hex << oui;
	return s.str();
#endif
}

uint32_t ieeeoui::string_to_oui(std::string const& s)
{
	// note no error checking. The strtoul() will return 0 on failure which is
	// an invalid OUI anyway.
	return strtoul(s.c_str(), nullptr, 16);
}

