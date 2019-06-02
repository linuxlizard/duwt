#include <iostream>
#include <fstream>
#include <iomanip>

#include "oui.h"

using namespace ieeeoui;

OUI::OUI(const char* csvfile)
	: infile(csvfile)
{
	if (!infile) {
		std::string errmsg { "no such file: " };
		errmsg += csvfile;
		throw OUIException(errmsg);
	}

//	std::clog << "open " << csvfile << "\n";
}

uint32_t OUI::parse_oui(std::string const& s)
{
	// only find the OUI & convert to integer
	return std::stoul(s.substr(5,6), nullptr, 16);
}

std::string OUI::parse_org(std::string const& s)
{
	// Have to be very careful because quoted string can contain embedded commas.
	// State machine FTW.

//	std::clog << "parse_org s=" << s << "\n";

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
//	std::clog << "parse_org found << " << s.substr(start,pos-start) << "\n";
	return s.substr(start,pos-start);
}

//OUI::oui_t OUI::parse(std::string const& s)
//{
//	// TODO how can I best make use of copy ellision?
//	// https://stackoverflow.com/questions/12953127/what-are-copy-elision-and-return-value-optimization
//	return std::make_tuple(0, "unknown");
//}

std::string OUI::get_org_name(uint32_t oui)
{
	std::string line;
	size_t counter { 0 };

//	std::clog << "get_org_name 0x" << std::setw(8) << std::setfill('0') << std::hex << oui << "\n";
	infile.seekg(0, infile.beg);

	while(std::getline(infile, line)) {
		counter++;
//		std::cout << "couter=" << counter << " len=" << line.length() << "\n";
		if (line.length() < 32) {
			continue;
		}

		// verify starts with MA-L,
		if (line[0] != 'M' || line[1] != 'A' || line[2] != '-' || line[3] != 'L' || line[4] != ',') {
			continue;
		}

		uint32_t num = parse_oui(line);
//		std::cout << "oui=" << std::hex << num << "\n";
		if (num == oui) {
			return parse_org(line);
		}
	}

	return std::string {"Foo"};
}

std::string OUI::get_org_name(uint8_t* oui)
{
	uint32_t n;

	// TODO big endian! (currently testing on Intel)
	n = (oui[0]<<16) | (oui[1]<<8) | oui[2];
	return get_org_name(n);
}

std::string ieeeoui::get_org_name(uint32_t oui)
{
	return std::string {"Foo"};
}

std::string ieeeoui::get_org_name(uint8_t* oui)
{
	return get_org_name((oui[0]<<16) | (oui[1]<<8) | oui[2]);
}

