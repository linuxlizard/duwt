#include <iostream>
#include <sstream>
#include <fstream>
#include <exception>

#include "mimetypes.h"

class TestFailure : public std::runtime_error
{
	public:
		TestFailure(const std::string& what ) : 
			std::runtime_error(what) { }
		TestFailure(const char* what ) :
			std::runtime_error(what) { }
};

// figuring out how hitting eof works
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

static void dbg_cout_mimetypes(const mimetypes& mt)
{
	for( const auto& n : mt ) {
		std::cout << n.first << "=" << n.second << "\n";
	}
}

static void dbg_cout_file(std::ifstream& infile)
{
	std::cout << "is_open=" << infile.is_open() << 
		" good=" << infile.good() << 
		" bad="<< infile.bad() << 
		" fail=" << infile.fail() << 
		"\n";
}

static void test1(void)
{
	std::string s {
"# this is a test\n\
application/x-dave\t\t\t\tdave\n"
	};

	std::istringstream is { s };
	mimetypes mt = mimetype_parse(is);
	dbg_cout_mimetypes(mt);

	std::string content_type = mt.at("dave");
}

static void test2(void)
{
	std::string s {
"# this is a test\n\
application/x-dave\t\t\t\tdave1\t\tdave2\n"
	};

	std::istringstream is { s };
	mimetypes mt = mimetype_parse(is);
	dbg_cout_mimetypes(mt);

	std::string content_type = mt.at("dave1");
	content_type = mt.at("dave2");
}

static void test3(void)
{
	std::string s {
"# this is a test\n\
application/x-dave    dave1  dave2\n"
	};

	std::istringstream is { s };
	mimetypes mt = mimetype_parse(is);
	dbg_cout_mimetypes(mt);

	std::string content_type = mt.at("dave1");
	content_type = mt.at("dave2");
}

static void test4(void)
{
	std::string s {
"application/x-dave    dave1  dave2 # foo bar baz\n"
	};

	std::istringstream is { s };
	mimetypes mt = mimetype_parse(is);
	dbg_cout_mimetypes(mt);

	std::string content_type = mt.at("dave1");
	content_type = mt.at("dave2");
}

static void test5(void)
{
	std::string s {
""
	};

	std::istringstream is { s };
	mimetypes mt = mimetype_parse(is);
	dbg_cout_mimetypes(mt);

}

static void parse_from_string(void)
{
	test1();
	test2();
	test3();
	test4();
	test5();
}

int main()
{
	std::cout << "test mimetypes start\n";

	parse_from_string();

	// trying to understand by badbit being set if I enable exceptions
//	std::ifstream mimefile {"/etc/mime.types"};
//	std::string line;
//	test_io(line, mimefile);
//	mimefile.close();

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

	dbg_cout_mimetypes(mt);
	std::cout << "find mimetype html\n";
	std::string content_type = mt.at("html");
	std::cout << "html=" << content_type << "\n";

	std::cout << "find nonsense mimetype dave (should fail)\n";
	try {
		content_type = mt.at("dave");
		std::cout << "content_type=" << content_type << "\n";
		throw TestFailure {"excepted dave to fail"};
	} catch (std::out_of_range& err) {
		std::cerr << "correctly failed to find mimetype dave err=" << err.what() << "\n";
	}

	std::cout << "parse a non-existant file (should fail)\n";
	std::string nofile { "this-file-doesnt-exist" };
	try { 
		mt = mimetype_parse_file(nofile);
		throw TestFailure {"excepted parse of nofile to fail"};
	} catch (const InvalidMimetypesFile & e) {
		std::cerr << "correctly failed to parse nofile: code=" << e.code() << " what=" << e.what() << "\n";
	}

	std::cout << "size=" << mt.size() << "\n";

	// tinkering with C++ and fileio iostate
	std::ifstream infile{nofile};
	try { 
		infile.exceptions(std::ifstream::failbit);
		infile.close();
		throw TestFailure {"excepted nofile to fail"};
	} catch (const std::ios_base::failure& e) {
		std::cerr << "correctly failed to parse file: code=" << e.code() << " what=" << e.what() << "\n";
	}
	dbg_cout_file(infile);

	try { 
		infile.open(nofile);
		infile.close();
	} catch (const std::ios_base::failure& e) {
		std::cerr << "correctly failed to open file: code=" << e.code() << " what=" << e.what() << "\n";
	}
	dbg_cout_file(infile);

}

