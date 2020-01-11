#include <iostream>

#include "mimetypes.h"

int main()
{
	mimetypes mt = mimetype_parse_default_file();

	std::string content_type = mt.at("html");
	std::cout << "html=" << content_type << "\n";
	printf("ptr=%p\n", (const void*)content_type.c_str());

	try {
		content_type = mt.at("dave");
	} catch (std::out_of_range& err) {
		std::cerr << err.what() << "\n";
	}

}

