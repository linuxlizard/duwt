#ifndef OUI_H
#define OU_H

#include <unordered_map>

class OUI
{
public:
	OUI(const char* csvfile);

	// disable copy & move
	OUI(const OUI&) = delete; // Copy constructor
	OUI& operator=(const OUI&) = delete;  // Copy assignment operator
	OUI(OUI&& src) = delete;
	OUI& operator=(OUI&& src) = delete; // Move assignment operator

private:
	std::unordered_map<uint32_t, std::string> lut;

};

#endif

