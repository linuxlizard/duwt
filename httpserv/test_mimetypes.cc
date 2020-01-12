#include <iostream>

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

}

