/*
 * oui.h  header file for oui.cc  
 *
 * parse oui files in CSV or other formats
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef OUI_H
#define OUI_H

#include <fstream>
#include <unordered_map>
#include <stdexcept>
#include <array>

namespace ieeeoui {

class OUI
{
public:
	OUI(std::string filename);
	virtual ~OUI() { };

	// disable copy & move
	OUI(const OUI&) = delete; // Copy constructor
	OUI& operator=(const OUI&) = delete;  // Copy assignment operator
	OUI(OUI&& src) = delete;
	OUI& operator=(OUI&& src) = delete; // Move assignment operator

	std::string get_org_name(uint32_t oui);
	std::string get_org_name(uint8_t* oui);

protected:
	std::string filename;
	std::ifstream infile;

	virtual std::string lookup(uint32_t oui) = 0;

	std::unordered_map<uint32_t, std::string> lut;

};

//
//  Read OUI CSV file.
//  Assuming format is the IEEE80211 OUI CSV.
// 
class OUI_CSV : public OUI
{
public:
	OUI_CSV(std::string filename);
	virtual ~OUI_CSV() { };

protected:
	virtual std::string lookup(uint32_t oui) override;

private:
	// just parse the org name
	std::string parse_org(std::string const& s);

	// just find the OUI
	uint32_t parse_oui(std::string const& s);

	enum class State {
		QUOTED_STRING,
		ESCAPE
	};

};

//
// Read IEEE OUI flat text file "MA" format
//
class OUI_MA : public OUI
{
public:
	OUI_MA(std::string filename);
	virtual ~OUI_MA() { };

protected:
	virtual std::string lookup(uint32_t oui) override;

private:
	// just parse the org name
	std::string parse_org(std::string const& s);

	// state machine to parse oui.txt, e.g., 
	// 70BC10     (base 16)		Microsoft Corporation
	enum class State {
		WHITESPACE,
		PAREN_EXPR,
		SEEK_ORG,
		FOUND_ORG,
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

std::string oui_to_string(uint32_t oui, bool zedx=true);
uint32_t string_to_oui(std::string const& s);

} // end namespace ieeeoui

#endif

