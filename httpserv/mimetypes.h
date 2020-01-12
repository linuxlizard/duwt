#ifndef MIMETYPES_H
#define MIMETYPES_H

#include <string>
#include <unordered_map>

#ifdef __has_include
#  if __has_include(<filesystem>)
#    include <filesystem>  // gcc8 (Fedora29+)
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem> // gcc7 (Ubuntu 18.04)
#  endif
#else
#error no __has_include
#endif

// key: extension
// value: mime-type 
// for example: ["html"] = "text/html" and ["htm"] = "text/html"
using mimetypes = std::unordered_map<std::string, std::string>;

mimetypes mimetype_parse(std::istream& infile);
mimetypes mimetype_parse_file(const std::string& infilename);
mimetypes mimetype_parse_default_file(void);

#endif
