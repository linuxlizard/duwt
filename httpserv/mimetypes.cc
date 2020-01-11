#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <iterator>
//#include <boost/algorithm/string.hpp>

#include "mimetypes.h"

namespace fs = std::filesystem;

static const std::string whitespace {" \t"};

mimetypes mimetype_parse(std::istream& infile)
{
	mimetypes mt {};
	std::string line;

	std::size_t line_counter {0};
	while( std::getline(infile,line) ) {
		line_counter++;
		if (line.length() == 0 || line[0] == '#') {
			// ignore blank lines
			// ignore comment lines
//			std::cout << "drop blank//comment line\n";
			continue;
		}

		std::cout << line_counter << " len=" << line.length() << " line=\"" << line << "\"\n";

		auto pos = line.find_first_of(whitespace);
		if (pos == std::string::npos) {
			std::cout << "no extension for file type=" << line ;
			continue;
		}

		// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
		std::istringstream iss {line};
		std::vector<std::string> fields {std::istream_iterator<std::string>{iss},
				                                 std::istream_iterator<std::string>()};
//		for (auto s: results) {
//			std::cout << s << "\n";
//		}

		for (auto idx = fields.size()-1 ; idx > 0 ; --idx) {
			std::cout << "idx=" << idx << " field=" << fields[idx] << "\n";
			mt[fields[idx]] = fields[0];
		}

	}

	for( const auto& n : mt ) {
		std::cout << n.first << "=" << n.second << "\n";
	}

	const std::string& foo = mt.at("html");
	printf("ptr=%p\n", (const void*)foo.c_str());

	const std::string& bar = mt.at("html");
	printf("ptr=%p\n", (const void*)bar.c_str());

	return mt;
}

mimetypes mimetype_parse_file(const fs::path& path)
{
	std::ifstream infile{path};
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

