#include <iostream>
#include <fstream>
#include <iomanip>

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

using namespace ieeeoui;

//#define DEBUG

OUI::OUI(std::string filename)
	: infile(filename)
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



OUI_CSV::OUI_CSV(std::string filename): OUI(filename)
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

#ifdef DEBUG
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
#ifdef DEBUG
	std::clog << "parse_org found \"" << s.substr(start,pos-start) << "\"\n";
#endif
	return s.substr(start,pos-start);
}

std::string OUI_CSV::lookup(uint32_t oui)
{
	std::string line;
	size_t counter { 0 };

#ifdef DEBUG
	std::clog << "lookup 0x" << std::setw(8) << std::setfill('0') << std::hex << oui << "\n";
#endif
	infile.seekg(0, infile.beg);

	while(std::getline(infile, line)) {
		counter++;
#ifdef DEBUG
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
#ifdef DEBUG
		std::clog << "oui=" << std::hex << num << "\n";
#endif
		if (num == oui) {
			return parse_org(line);
		}
	}

	throw std::out_of_range(oui_to_string(oui));
}

OUI_MA::OUI_MA(std::string filename) : OUI(filename)
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

	for (pos=6 ; pos<len && state != State::FOUND_ORG ; pos++) {
#ifdef DEBUG
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

#ifdef DEBUG
	std::clog << "lookup 0x" << std::setw(8) << std::setfill('0') << std::hex << oui << "\n";
#endif

	infile.seekg(0, infile.beg);
	while(std::getline(infile, line)) {
		counter++;

		// super simple check to look for lines starting with hex HHHHHH<space>
		if (line.length() < 32 || line[6] != ' ') {
			// ignore
			continue;
		}

		uint32_t num32 = oui_string_to_num(line);
#ifdef DEBUG
		std::clog << std::dec << "line=" << counter << " oui=" << std::hex << num32 << "\n";
#endif
		if (num32 == oui) {
			return parse_org(line);
		}
	}

	throw std::out_of_range(oui_to_string(oui));
}


uint32_t OUI_MA::oui_string_to_num(std::string const& s)
{
	uint32_t num32 { 0 };

	// For a simple subset of hex strings. 
	// Note: assumes capital letters for hex in the OUI
	try { 
		for (size_t i=0 ; i<6 ; i++) {
			uint8_t n = hexlut.at(static_cast<int>(s[i]-'0'));
			num32 |= n;
			num32 <<= 4;
		} 
	}
	catch (std::out_of_range& err) {
		return 0;
	}
	num32 >>= 4;
	return num32;
}

std::string ieeeoui::oui_to_string(uint32_t oui)
{
	// poTAYto poTAHto (did both ways to see how the ostreamstream works)
//	char oui_str[12] = {};
//	snprintf(oui_str, 11, "%#08lx", oui);
//	return std::string { oui_str };

	std::ostringstream s;
	s << std::setw(6) << std::setfill('0') << std::hex << oui;
	return s.str();
}

uint32_t ieeeoui::string_to_oui(std::string const& s)
{
	return strtoul(s.c_str(), nullptr, 16);
}

