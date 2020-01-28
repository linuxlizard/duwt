#include <iostream>
#include <fstream>

#include "mimetypes.h"

int main()
{
	mimetypes mt = mimetype_parse_default_file();

	for( const auto& n : mt ) {
		std::cout << n.first << "=" << n.second << "\n";
	}

	std::string content_type = mt.at("html");
	std::cout << "html=" << content_type << "\n";

	try {
		content_type = mt.at("dave");
	} catch (std::out_of_range& err) {
		std::cerr << err.what() << "\n";
	}

	std::string nofile { "this-file-doesnt-exist" };
	mt = mimetype_parse_file(nofile);
	std::cout << "size=" << mt.size() << "\n";

	std::ifstream infile{nofile};
	infile.exceptions(std::ifstream::failbit);
	std::cout << "is_open=" << infile.is_open() << 
		" good=" << infile.good() << 
		" bad="<< infile.bad() << 
		" fail=" << infile.fail() << 
		"\n";
}

