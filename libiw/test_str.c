/*
 * libiw/test_str.c   test str.c functions
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://jansson.readthedocs.io/en/2.12/
#include <jansson.h>

#include "core.h"
#include "str.h"

void test_str_match(void)
{
	XASSERT(str_match(NULL,1024,NULL,1024),0);
	XASSERT(str_match("hello, world",12,"hello, world",12),0);
	XASSERT(!str_match("hello, world",12,"hello, world",11),0);
	XASSERT(!str_match("hello, world",12,NULL,12),0);
	XASSERT(!str_match("hello, world",12,NULL,13),0);
	XASSERT(!str_match(NULL,12,"hello, world",12),0);
	XASSERT(!str_match(NULL,13,"hello, world",12),0);
	XASSERT(!str_match("hello, world",12,"hello, qorld",12),0);
	XASSERT(str_match("hello\x00world",12,"hello\x00world",12),0);
	XASSERT(str_match("",0,"",0),0);
}

void test_escape(void)
{
	char s[32] = "hello, world";
	char escaped[128];

	memset(escaped, 0, sizeof(escaped));
	int retcode = str_escape(s, strlen(s), escaped, sizeof(escaped)-1);
	XASSERT( retcode == 12, retcode );
	XASSERT( (retcode=strncmp(escaped, s, 12)==0), retcode); 

	memset(escaped,0,sizeof(escaped));
	memset(s, 0, sizeof(s));
	retcode = str_escape(s, 12, escaped, sizeof(escaped)-1);
	XASSERT( retcode == 12*4, retcode );
	XASSERT( str_match(escaped, 48, "\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00", 48),0);

	memset(escaped,0,sizeof(escaped));
	memcpy(s, "\xf0\xf9\xa4\xae\x00", 5);
	retcode = str_escape(s, 4, escaped, sizeof(escaped)-1);
	XASSERT( retcode == 4*4, retcode );

	memset(escaped,0,sizeof(escaped));
	memcpy(s, "foo\x00""bar\x00", 8);
	retcode = str_escape(s, 7, escaped, sizeof(escaped)-1);
	XASSERT( retcode == 3+4+3, retcode );
	XASSERT( (retcode=memcmp(escaped,"foo\\x00bar", 10))==0, retcode);

	memset(escaped,0,sizeof(escaped));
	retcode = str_escape(s, 7, escaped, 5);
	XASSERT(retcode==-ENOMEM, retcode);

	memset(escaped,0,sizeof(escaped));
	retcode = str_escape("\\\\\\\\", 4, escaped, sizeof(escaped)-1);
	XASSERT(retcode==4*4, retcode);
	XASSERT( (retcode=strcmp(escaped,"\\x5c\\x5c\\x5c\\x5c"))==0, retcode);
}

void test_json_escape(void)
{
	// why the flying FOO am I doing all this hard work when I'm already using
	// jansson ?????

	json_t* jstr;

	char src[32] = "hello, world";

	jstr = json_stringn(src, strlen(src));
	XASSERT(json_is_string(jstr), json_typeof(jstr));
	const char* hello = json_string_value(jstr);
	XASSERT(hello, 0);
	XASSERT(str_match(hello,12,src,12),0);
	json_decref(jstr);

	json_t* jobj;
	jobj = json_pack("{ss#}", "ssid", src, strlen(src));
	char* s = json_dumps(jobj, JSON_INDENT(1));
	XASSERT(s, 0);
	printf("%s\n", s);
	PTR_FREE(s);
	json_decref(jobj);

	char unicode[32] = "(╯°□°）╯︵ ┻━┻";
	jobj = json_pack("{ss}", "ssid", unicode);
	s = json_dumps(jobj, JSON_INDENT(1));
	XASSERT(s, 0);
	printf("%s\n", s);
	PTR_FREE(s);
	json_decref(jobj);

	// break a byte in the unicode
	unicode[1] = unicode[1] & 0x7f;
	json_error_t error;
	jobj = json_pack_ex(&error, 0,"{ss}", "ssid", unicode);

	char nulls[32];
	memset(nulls,0,sizeof(nulls));
	jobj = json_pack("{ss#}", "ssid", nulls, 16);
	s = json_dumps(jobj, JSON_INDENT(1));
	XASSERT(s, 0);
	printf("%s\n", s);
	PTR_FREE(s);
	json_decref(jobj);

	char control[32];
	for (int i=0 ; i<32 ; i++) {
		control[i] = i;
	}
	jobj = json_pack("{ss#}", "ssid", control, 32);
	s = json_dumps(jobj, JSON_INDENT(1));
	XASSERT(s, 0);
	printf("%s\n", s);
	PTR_FREE(s);
	json_decref(jobj);

	char blink[32] = "\x1b[5m";
	jobj = json_pack("{ss#}", "ssid", blink, 4);
	s = json_dumps(jobj, JSON_INDENT(1));
	XASSERT(s, 0);
	printf("%s\n", s);
	PTR_FREE(s);
	json_decref(jobj);
}

static void test_str_hexdump(void)
{
	char s[] = "0123456789abcdef";
	char hex[128];

	memset(hex,0xff,sizeof(hex));
	int ret = str_hexdump((unsigned char*)s, strlen(s), hex, sizeof(hex));
	XASSERT(ret==32, ret);
	XASSERT(hex[32]==0, hex[32]);
	XASSERT(str_match(hex,strlen(hex),"30313233343536373839616263646566",32),0);

	// destination exactly fits src (+1 for NULL)
	// str_hexdump() responsible for the terminating NULL
	memset(hex,0xff,sizeof(hex));
	ret = str_hexdump((unsigned char*)s, strlen(s), hex, strlen(s)*2+1);
//	printf("hex=%zu %s\n", strlen(hex), hex);
	XASSERT(ret==32, ret);
	XASSERT(hex[32]==0, hex[32]);

	// not enough space for NULL (one short)
	ret = str_hexdump((unsigned char*)s,strlen(s),hex,32);
	XASSERT(ret == -ENOMEM, ret);

	memset(hex,0xff,sizeof(hex));
	ret = str_hexdump((unsigned char*)s,strlen(s),hex,33);
	XASSERT(ret == 32, ret);
	XASSERT(hex[32]==0, hex[32]);

	// fill 's' with garbage (strlen will no longer work)
	memset(s,0,sizeof(s));
	ret = str_hexdump((unsigned char*)s, sizeof(s), hex, sizeof(hex));
	XASSERT(ret==sizeof(s)*2, ret);
//	printf("hex=%zu %s\n", strlen(hex), hex);
	XASSERT(str_match(hex,sizeof(s)*2,"0000000000000000000000000000000000",sizeof(s)*2),0);

}

void test_str_split(void)
{
	// str_split() is destructive so can't use state strings
	int err;
	char s[1024];
	char* ptrlist[32];
	size_t ptrlist_size;

	strcpy(s, "Hello there all you rabbits");
	ptrlist_size = ARRAY_SIZE(ptrlist);
	err = str_split(s, strlen(s), " ", ptrlist, &ptrlist_size);
	XASSERT(err==0, err);
	XASSERT(ptrlist_size == 5, ptrlist_size);
	XASSERT(str_match(ptrlist[0], 5, "Hello", 5), 0);
	XASSERT(str_match(ptrlist[1], 5, "there", 5), 0);
	XASSERT(str_match(ptrlist[2], 3, "all", 3), 0);
	XASSERT(str_match(ptrlist[3], 3, "you", 3), 0);
	XASSERT(str_match(ptrlist[4], 7, "rabbits", 7), 0);

	strcpy(s, "   Hello   there    all     you     rabbits    ");
	ptrlist_size = ARRAY_SIZE(ptrlist);
	err = str_split(s, strlen(s), " ", ptrlist, &ptrlist_size);
	XASSERT(err==0, err);
	XASSERT(ptrlist_size == 5, ptrlist_size);
	XASSERT(str_match(ptrlist[0], 5, "Hello", 5), 0);
	XASSERT(str_match(ptrlist[1], 5, "there", 5), 0);
	XASSERT(str_match(ptrlist[2], 3, "all", 3), 0);
	XASSERT(str_match(ptrlist[3], 3, "you", 3), 0);
	XASSERT(str_match(ptrlist[4], 7, "rabbits", 7), 0);

	strcpy(s, "   Hello   there    all     you     rabbits    ");
	ptrlist_size = 1;
	err = str_split(s, strlen(s), " ", ptrlist, &ptrlist_size);
	XASSERT(err==-ENOMEM, err);

	strcpy(s, "\t \t \t Hello \t  there \t   all \t\t    you  \t\t\t   rabbits \t\t\t   ");
	ptrlist_size = ARRAY_SIZE(ptrlist);
	err = str_split(s, strlen(s), " \t", ptrlist, &ptrlist_size);
	XASSERT(err==0, err);
	XASSERT(ptrlist_size == 5, ptrlist_size);
	XASSERT(str_match(ptrlist[0], 5, "Hello", 5), 0);
	XASSERT(str_match(ptrlist[1], 5, "there", 5), 0);
	XASSERT(str_match(ptrlist[2], 3, "all", 3), 0);
	XASSERT(str_match(ptrlist[3], 3, "you", 3), 0);
	XASSERT(str_match(ptrlist[4], 7, "rabbits", 7), 0);

	strcpy(s, "/home/davep/src/duwt/build");
	ptrlist_size = ARRAY_SIZE(ptrlist);
	err = str_split(s, strlen(s), "/", ptrlist, &ptrlist_size);
	XASSERT(err==0, err);
	XASSERT(ptrlist_size == 5, ptrlist_size);
	XASSERT(str_match(ptrlist[0], 4, "home", 4), 0);
	XASSERT(str_match(ptrlist[1], 5, "davep", 5), 0);
	XASSERT(str_match(ptrlist[2], 3, "src", 3), 0);
	XASSERT(str_match(ptrlist[3], 4, "duwt", 4), 0);
	XASSERT(str_match(ptrlist[4], 5, "build", 5), 0);

	strcpy(s, "/home/davep/src/duwt/build/..//////");
	ptrlist_size = ARRAY_SIZE(ptrlist);
	err = str_split(s, strlen(s), "/", ptrlist, &ptrlist_size);
	XASSERT(err==0, err);
	XASSERT(ptrlist_size == 6, ptrlist_size);
	XASSERT(str_match(ptrlist[0], 4, "home", 4), 0);
	XASSERT(str_match(ptrlist[1], 5, "davep", 5), 0);
	XASSERT(str_match(ptrlist[2], 3, "src", 3), 0);
	XASSERT(str_match(ptrlist[3], 4, "duwt", 4), 0);
	XASSERT(str_match(ptrlist[4], 5, "build", 5), 0);
	XASSERT(str_match(ptrlist[5], 2, "..", 2), 0);

	strcpy(s,"");
	ptrlist_size = ARRAY_SIZE(ptrlist);
	err = str_split(s, strlen(s), "q", ptrlist, &ptrlist_size);
	XASSERT(err==0, err);

	strcpy(s, "           ");
	ptrlist_size = ARRAY_SIZE(ptrlist);
	err = str_split(s, strlen(s), " ", ptrlist, &ptrlist_size);
	XASSERT(err==0, err);
	XASSERT(ptrlist_size == 0, ptrlist_size);

	static const char* notfound = "\
<html>\n\
<head>\n\
	<title>Not Found</title>\n\
</head>\n\
<body>\n\
	<p>Not Found.</p>\n\
</html>\n\
";

	static const char* expect[] = {
		"<html>",
		"<head>",
		"<title>Not",
		"Found</title>",
		"</head>",
		"<body>",
		"<p>Not",
		"Found.</p>",
		"</html>",
	};

	strncpy(s, notfound, sizeof(s));
	ptrlist_size = ARRAY_SIZE(ptrlist);
	err = str_split(s, strlen(s), WHITESPACE, ptrlist, &ptrlist_size);
	XASSERT(err==0, err);
	XASSERT(ptrlist_size == 9, ptrlist_size);;
	for (size_t i=0 ; i<ptrlist_size ; i++) {
		XASSERT(str_match(ptrlist[i], strlen(ptrlist[i]), expect[i], strlen(expect[i])), i);
	}

}

int main(void)
{
	test_str_match();
	test_escape();
	test_json_escape();
	test_str_hexdump();
	test_str_split();

	return EXIT_SUCCESS;
}

