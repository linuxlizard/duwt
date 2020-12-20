/*
 * mimetypes.h  parse /etc/mime.types into C++ map
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef MIMETYPES_H
#define MIMETYPES_H

#include <string>
#include <unordered_map>
#include <exception>

#include "fs.h"

// key: extension
// value: mime-type 
// for example: ["html"] = "text/html" and ["htm"] = "text/html"
using mimetypes = std::unordered_map<std::string, std::string>;

mimetypes mimetype_parse(std::istream& infile);
mimetypes mimetype_parse_file(const fs::path& path);
mimetypes mimetype_parse_file(const std::string& infilename);
mimetypes mimetype_parse_default_file(void);

class InvalidMimetypesFile : public std::runtime_error
{
	public:
		InvalidMimetypesFile(const std::string& what, int myerrno=0 ) : 
			std::runtime_error(what) { _errno = myerrno; }
//			std::runtime_error(what), _errno(errno) { }
		InvalidMimetypesFile(const char* what, int myerrno=0 ) :
//			std::runtime_error(what), _errno(errno) { }
			std::runtime_error(what) { _errno = myerrno; }

		int code(void) const {return _errno;};

	private:
		int _errno;
};


#endif
