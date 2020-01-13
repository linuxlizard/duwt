#include <fstream>
#include <unordered_map>
#include <vector>
#include <iterator>
//#include <boost/algorithm/string.hpp>

// https://en.cppreference.com/w/cpp/feature_test
#ifdef __has_include
#  if __has_include(<filesystem>)
#    include <filesystem>  // gcc8 (Fedora29+)
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem> // gcc7 (Ubuntu 18.04)
     namespace fs = std::experimental::filesystem;
#  endif
#else
#error no __has_include
#endif

#include "mimetypes.h"

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

