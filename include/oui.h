#ifndef OUI_H
#define OUI_H

#include <fstream>
#include <unordered_map>
#include <stdexcept>

namespace ieeeoui {

class OUI
{
public:
	OUI(const char* csvfile);

	// disable copy & move
	OUI(const OUI&) = delete; // Copy constructor
	OUI& operator=(const OUI&) = delete;  // Copy assignment operator
	OUI(OUI&& src) = delete;
	OUI& operator=(OUI&& src) = delete; // Move assignment operator

	std::string get_org_name(uint32_t oui);
	std::string get_org_name(uint8_t* oui);

private:
	std::ifstream infile;

	std::unordered_map<uint32_t, std::string> lut;

	// as of this writing, only care about the OUI# and Organization Name but
	// leave room for growth
//	using oui_t = std::tuple<uint32_t, std::string>;
//	oui_t parse(std::string const& s);

	// just find the OUI
	uint32_t parse_oui(std::string const& s);
	// just parse the org name
	std::string parse_org(std::string const& s);

	enum class State {
		QUOTED_STRING,
		ESCAPE
	};
};

class OUIException : public std::runtime_error 
{
public:
	OUIException() 
		: std::runtime_error("OUIException") {}
	OUIException(std::string const& message) 
		: std::runtime_error(message) {}
};

} // end namespace ieeeoui

#endif

