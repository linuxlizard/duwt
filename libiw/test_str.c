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
	XASSERT( (retcode=strncmp(escaped, "\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00", 48))==0, retcode);

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

int main(void)
{
	test_escape();
	test_json_escape();

	return EXIT_SUCCESS;
}

