/*
 * mimetypes.h  parse /etc/mime.types into C++ map
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <iterator>

#include "mimetypes.h"
#include "fs.h"

//#define DEBUG

enum class State
{
	START = 1,
	TYPE,
	SEEK_EXTENSION,
	EXTENSION,
	ERROR
};

static const std::array<const char*,7> state_names {
	"",
	"START", 
	"TYPE",
	"SEEK_EXTENSION",
	"EXTENSION",
	"ERROR"
};

static const char LF = 0x0a;
static const char CR = 0x0d;
static const char SP = ' ';
static const char HT = '\t';

static State eat_eol(std::istream& infile)
{
	char c = infile.get();
	if (infile.eof() || c != LF) {
		return State::ERROR;
	}
	return State::START;
}

static State eat_line(std::istream& infile)
{
	// read chars until end of line
	while ( !infile.eof() ) {
		char c = infile.get();

		if (c==LF) {
			break;
		}
		else if (c==CR) {
			// handle CRLF
			return eat_eol(infile);
		}
	}

	return State::START;
}

#ifdef DEBUG
static void dbg_cout_file(std::istream& infile)
{
	std::cout << "is_open=" << "?" << 
		" good=" << infile.good() << 
		" bad="<< infile.bad() << 
		" fail=" << infile.fail() << 
		" eof=" << infile.eof() << 
		"\n";
}
#endif

mimetypes mimetype_parse(std::istream& infile)
{
	if (infile.fail()) {
		throw InvalidMimetypesFile("file open failed");
	}

	mimetypes mt {};
	State state { State::START };

	std::string type;
	std::string ext;

	type.reserve(128);
	ext.reserve(64);

	while (true) {
#ifdef DEBUG
		dbg_cout_file(infile);
#endif
		char c = infile.get();
		if (infile.eof()) {
			break;
		}

//		dbg_cout_file(infile);

		if (infile.fail()) {
			throw std::runtime_error("file fail bit set");
		}

//		std::cout << "state=" << state_names[static_cast<int>(state)] << " c=" << c << "\n";
		switch (state) {
			case State::START:
				if (c=='#') {
					state = eat_line(infile);
				}
				else if (c==CR) {
					state = eat_eol(infile);
				}
				else if (c==LF || c==SP || c==HT) {
				}
				else {
//					std::cout << "State::TYPE start\n";
					type.erase();
					type.push_back(c);
					state = State::TYPE;
				}
				break;

			case State::TYPE:
				if (c==CR) {
					// end of type
					state = eat_eol(infile);
				}
				else if (c==LF) {
					// end of type
					state = State::START;
				}
				else if (c==LF || c==SP || c==HT) {
					// end of type
					state = State::SEEK_EXTENSION;
				}
				else {
					ext.erase();
					type.push_back(c);
				}
				break;

			case State::SEEK_EXTENSION:
				if (c==CR) {
					// end of line
					state = eat_eol(infile);
				}
				else if (c==LF) {
					// end of line
					state = State::START;
				}
				else if (!(c==SP || c==HT)) {
					ext.erase();
					ext.push_back(c);
					state = State::EXTENSION;
				}
				break;

			case State::EXTENSION:
				if (c==CR) {
					// end of line; save extension
					mt[ext] = type;
					state = eat_eol(infile);
				}
				else if (c==LF) {
					// end of line; save extension
//					std::cout << "type=" << type << " ext=" << ext << "\n";
					mt[ext] = type;
					state = State::START;
				}
				else if (c==SP || c==HT) {
					// end of this extension
					mt[ext] = type;
					state = State::SEEK_EXTENSION;
				}
				else {
					ext.push_back(c);
				}
				break;

			case State::ERROR:
				break;
		}

	}

	return mt;
}


static const std::string whitespace {" \t"};

mimetypes old_mimetype_parse(std::istream& infile)
{
	mimetypes mt {};
	std::string line;

	std::size_t line_counter {0};
	while( std::getline(infile,line) ) {
		if (infile.eof()) {
			break;
		}
		line_counter++;
		if (line.length() == 0 || line[0] == '#') {
			// ignore blank lines
			// ignore comment lines
			continue;
		}

		auto pos = line.find_first_of(whitespace);
		if (pos == std::string::npos) {
//			std::cout << "no extension for file type=" << line ;
			continue;
		}

		// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
		std::istringstream iss {line};
		std::vector<std::string> fields {std::istream_iterator<std::string>{iss},
				                                 std::istream_iterator<std::string>()};

		for (auto idx = fields.size()-1 ; idx > 0 ; --idx) {
			mt[fields[idx]] = fields[0];
		}
	}

	return mt;
}

mimetypes mimetype_parse_file(const fs::path& path)
{
	std::ifstream infile{path};
	std::cout << "path=" << path << " exceptions=" << infile.exceptions() << "\n";
	infile.exceptions(std::ios_base::badbit);
//	infile.exceptions(std::ios_base::badbit | std::ios_base::failbit);
	std::cout << "path=" << path << " exceptions=" << infile.exceptions() << "\n";
	return mimetype_parse(infile);
}

mimetypes mimetype_parse_file(const std::string& infilename)
{
	fs::path path {infilename};
	return mimetype_parse_file(path);
}

mimetypes mimetype_parse_default_file(void)
{
	// look for a /etc/mime.types and parse it if it exists
	std::string default_name {"/etc/mime.types"};

	return mimetype_parse_file(default_name);
}

