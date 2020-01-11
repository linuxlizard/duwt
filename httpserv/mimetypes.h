#ifndef MIMETYPES_H
#define MIMETYPES_H

#include <string>
#include <unordered_map>
#include <filesystem>

// key: extension
// value: mime-type 
// for example: ["html"] = "text/html" and ["htm"] = "text/html"
using mimetypes = std::unordered_map<std::string, std::string>;

mimetypes mimetype_parse(std::istream& infile);
mimetypes mimetype_parse_file(const std::string& infilename);
mimetypes mimetype_parse_default_file(void);

#endif
