#include <iostream>
#include <fstream>

#include "mimetypes.h"

// gcc-9.3.0/libstdc++-v3/include/bits/basic_string.tcc
void test_io(std::string& str, std::istream& infile)
{
	str.erase();
	std::cout << "max_size=" << str.max_size() << "\n";
//	auto eof = std::char_traits::eof;

	auto flags = std::ios_base::goodbit;

	auto eof = std::char_traits<char>::eof();

	int c = infile.rdbuf()->sgetc();

	while ( !std::char_traits<char>::eq_int_type(c,eof) ) {

		if ( std::char_traits<char>::eq_int_type(c,eof) ) {
			flags |= std::ios_base::eofbit;
			break;
		}

		std::cout << "c=" << c << " rdstate=" << infile.rdstate() << "\n";
		c = infile.rdbuf()->sbumpc();
	}
}

int main()
{
	std::cout << "test mimetypes start\n";

	// trying to understand by badbit being set if I enable exceptions
	std::ifstream mimefile {"/etc/mime.types"};
	std::string line;
	test_io(line, mimefile);
	mimefile.close();

	std::cout << "goodbit=" << std::ios_base::goodbit << "\n";
	std::cout << "eofbit=" << std::ios_base::eofbit << "\n";
	std::cout << "badbit=" << std::ios_base::badbit << "\n";
	std::cout << "failbit=" << std::ios_base::failbit << "\n";

	std::cout << "parse default file\n";
	mimetypes mt;
	try { 
		mt = mimetype_parse_default_file();
	} catch (const std::ios_base::failure& e) {
		std::cerr << "failed to parse default file: code=" << e.code() << " what=" << e.what() << "\n";
		throw;
	}

//	for( const auto& n : mt ) {
//		std::cout << n.first << "=" << n.second << "\n";
//	}

	std::cout << "find mimetype html\n";
	std::string content_type = mt.at("html");
	std::cout << "html=" << content_type << "\n";

	std::cout << "find nonsense mimetype dave (should fail)\n";
	try {
		content_type = mt.at("dave");
		std::cout << "content_type=" << content_type << "\n";
	} catch (std::out_of_range& err) {
		std::cerr << "failed to find mimetype dave err=" << err.what() << "\n";
	}

	std::cout << "parse a non-existant file (should fail)\n";
	std::string nofile { "this-file-doesnt-exist" };
	mt = mimetype_parse_file(nofile);
	std::cout << "size=" << mt.size() << "\n";

	// tinkering with C++ and fileio iostate
	std::ifstream infile{nofile};
	try { 
		infile.exceptions(std::ifstream::failbit);
	} catch (const std::ios_base::failure& e) {
		std::cerr << "failed to parse file: code=" << e.code() << " what=" << e.what() << "\n";
	}
	std::cout << "is_open=" << infile.is_open() << 
		" good=" << infile.good() << 
		" bad="<< infile.bad() << 
		" fail=" << infile.fail() << 
		"\n";
}

