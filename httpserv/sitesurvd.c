/*
 * web api for retrieving wifi scan data
 *
 * started from https://www.gnu.org/software/libmicrohttpd/tutorial.html
 * davep 20191227
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#define _GNU_SOURCE 1

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <search.h>
#include <limits.h>

// netlink
#include <linux/if_ether.h>
#include <linux/nl80211.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include <microhttpd.h>

// microhttpd switched to enums after this point
#if MHD_VERSION <= 0x00097000
#error
#undef MHD_NO
#undef MHD_YES
enum MHD_Result {
	MHD_NO = 0,
	MHD_YES = 1 
};
#endif

#include "core.h"
#include "iw.h"
#include "iw-scan.h"
#include "ssid.h"
#include "nlnames.h"
#include "hdump.h"
#include "ie.h"
#include "mimetypes.h"
#include "args.h"
#include "bss_json.h"
#include "ssid.h"

#define PORT 8081
//#define PORT 8888

int survd_debug = 0;

static struct hsearch_data mimetypes;

static const char* notfound = "\
<html>\n\
<head>\n\
   <title>Not Found</title>\n\
</head>\n\
<body>\n\
<p>Not Found.</p>\n\
</html>\n\
";

// storage for the glibc binary tree data structure 
// "tsearch, tfind, tdelete, twalk, tdestroy - manage a binary search tree  "
typedef struct 
{
	size_t count;
	void* root;
} BSSMap;

#define BUNDLE_COOKIE 0x01448f44
struct netlink_socket_bundle 
{
	uint32_t cookie;
	const char* name;

	struct nl_cb* cb;
	struct nl_sock* nl_sock;
	int sock_fd;
	int nl80211_id;
	int cb_err;
	void* more_args;

	// if we're already reading a CMD_GET_SCAN result, don't push another
	// request for scan results.
	// https://github.com/linuxlizard/duwt/issues/3
	bool busy;
};

// tsearch() callback
static int bss_compare(const void* p1, const void* p2)
{
	struct BSS* bss_one = (struct BSS*)p1;
	struct BSS* bss_two = (struct BSS*)p2;

	XASSERT(bss_one->cookie == BSS_COOKIE, 0);
	XASSERT(bss_two->cookie == BSS_COOKIE, 0);

	printf("compare %s to %s\n", bss_one->bssid_str, bss_two->bssid_str);

	// TODO switch to 64-bit encapsulation of the 6-byte bssid?
	return strcmp(bss_one->bssid_str, bss_two->bssid_str);
}

// twalk() callback
static void bss_print(const void* nodep, const VISIT which, const int depth)
{
	struct BSS* bss = *(struct BSS**)nodep;

	(void)depth;
	if (which==postorder || which==leaf) {
		printf("%s %s", __func__, bss->bssid_str);
		ssid_print(bss, stdout, 32, "\n");
	}
}

static void bss_map_dump(const BSSMap* bss_map)
{
	twalk(bss_map->root, bss_print);
}

// netlink callback on the NL80211_CMD_GET_SCAN message
static int scan_survey_valid_handler(struct nl_msg *msg, void *arg)
{
	INFO("%s\n", __func__);

	if (survd_debug > 1) {
		nl_msg_dump(msg, stdout);
	}

	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)arg;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);

//	struct dl_list* bss_list = (struct dl_list*)bun->more_args;
	BSSMap* map = (BSSMap*)bun->more_args;

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	INFO("%s len=%" PRIu32 " type=%u flags=%u seq=%" PRIu32 " pid=%" PRIu32 "\n", __func__,
			hdr->nlmsg_len, hdr->nlmsg_type, hdr->nlmsg_flags, 
			hdr->nlmsg_seq, hdr->nlmsg_pid);

	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	int err = nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);
	XASSERT(err==0, err);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	int ifidx = -1;
	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		ifidx = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
		DBG("%s ifindex=%d\n", __func__, ifidx);
	}

	struct BSS* new_bss = NULL;
	err = bss_from_nlattr(tb_msg, &new_bss);
	if (err) {
		ERR("%s bss_from_nlattr failed err=%d\n", __func__, err);
		goto fail;
	}
	else {
//		uint64_t key = BSSID_U64(new_bss->bssid);

		// find tree entry with matching bssid
		void* p = tfind((void*)new_bss, &map->root, bss_compare);
		if (p) {
			// item found ; release the memory
			struct BSS* bss = *(struct BSS**)p;
			XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
			tdelete(bss, &map->root, bss_compare);
			DBG("%s free %p\n", __func__, (void*)bss);
			bss_free(&bss);
		}
		else { 
			// not found ; this is new 
			// so increment our count
			map->count++;
		}

		// add it
		p = tsearch((void*)new_bss, &map->root, bss_compare);

	}

	DBG("%s success\n", __func__);
	return NL_OK;
fail:
	DBG("%s failed\n", __func__);
	return NL_SKIP;
}

// libnl callback 
static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	(void)nla;

	// TODO better error decode
	ERR("%s seq=%" PRIu32 " error=%d: %s\n", __func__, err->msg.nlmsg_seq, err->error, strerror(err->error));

	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)arg;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);
	bun->busy = false;

	return NL_STOP;
}

// libnl callback
static int finish_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);

	if (survd_debug > 1) {
		nl_msg_dump(msg, stdout);
	}

	(void)msg;

	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)arg;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);
	bun->cb_err = 0;
	bun->busy = false;

	return NL_SKIP;
}

// libnl callback
static int ack_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);

	if (survd_debug > 1) {
		nl_msg_dump(msg, stdout);
	}

	(void)msg;

	int *ret = (int*)(arg);
	*ret = 0;
	return NL_STOP;
}

// libnl callback
static int no_seq_check(struct nl_msg *msg, void *arg)
{
	INFO("%s\n", __func__);

	if (survd_debug > 1) {
		nl_msg_dump(msg, stdout);
	}

	(void)msg;
	(void)arg;

	return NL_OK;
}

// libnl callback for the Scan Event received
static int on_scan_event_valid_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s %p %p\n", __func__, (void *)msg, arg);

	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)arg;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);

//	hex_dump("msg", (const unsigned char *)msg, 128);

	if (survd_debug > 1) {
		nl_msg_dump(msg,stdout);
	}

	struct nlmsghdr *hdr = nlmsg_hdr(msg);

	int datalen = nlmsg_datalen(hdr);
	DBG("datalen=%d attrlen=%d\n", datalen, nlmsg_attrlen(hdr,0));

	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

	struct genlmsghdr *gnlh = (struct genlmsghdr*)nlmsg_data(nlmsg_hdr(msg));
	int err = nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);
	if (err) {
		// TODO
		XASSERT(0,err);
	}


	DBG("%s cmd=%d %s\n", __func__, (int)(gnlh->cmd), to_string_nl80211_commands((enum nl80211_commands)gnlh->cmd));

	// debug dump
	for (int i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			const char *name = to_string_nl80211_attrs((enum nl80211_attrs)i);
			DBG("%s i=%d %s at %p type=%d len=%d\n", __func__, 
				i, name, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
		}
	}

	int ifidx = -1;
	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		ifidx = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
		DBG("%s ifindex=%d\n", __func__, ifidx);
	}

	// if new scan results,
	// send the get scan results message
	if ( (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS || gnlh->cmd == NL80211_CMD_SCHED_SCAN_RESULTS) && ifidx != -1) {
		struct netlink_socket_bundle* scan_sock_bun = (struct netlink_socket_bundle*)bun->more_args;
		XASSERT(scan_sock_bun->cookie == BUNDLE_COOKIE, scan_sock_bun->cookie);

		// https://github.com/linuxlizard/duwt/issues/3  don't start another fetch while a fetch is already running
		if (scan_sock_bun->busy) {
			WARN("%s not starting a new NL80211_CMD_GET_SCAN while a get already running\n", __func__);
		}
		else {
			struct nl_msg* new_msg = nlmsg_alloc();
			XASSERT(new_msg,0);

			void* p = genlmsg_put(new_msg, NL_AUTO_PORT, NL_AUTO_SEQ, 
								scan_sock_bun->nl80211_id, 
								0, 
								NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
			XASSERT(p,0);
			nla_put_u32(new_msg, NL80211_ATTR_IFINDEX, ifidx);
			int ret = nl_send_auto(scan_sock_bun->nl_sock, new_msg);
			if (ret < 0) {
				// TODO
				XASSERT(0,ret);
			}
			if (survd_debug > 1) {
				nl_msg_dump(new_msg, stdout);
			}
			DBG("%s sent NL80211_CMD_GET_SCAN bytes=%d\n", __func__, ret);
			scan_sock_bun->busy = true;
		}
	}

	return NL_SKIP;
}

enum MHD_Result capture_keys (void *arg, enum MHD_ValueKind kind, 
					const char *key, const char *value)
{
	// MHD callback
	printf ("%s %d %s=%s\n", __func__, kind, key, value);
	(void)arg;
	(void)kind;
	(void)key;
	(void)value;
#if 0
	if (kind == MHD_GET_ARGUMENT_KIND) {
		KeyValueList* kv_list = static_cast<KeyValueList*>(arg);
		kv_list->emplace_back(key, value);
	}
	else if (kind == MHD_HEADER_KIND) {
		KeyValueList* kv_list = static_cast<KeyValueList*>(arg);
		kv_list->emplace_back(key, value);
	}
#endif
	return MHD_YES;
}

// responsible for /api/survey 
void get_survey_response(const BSSMap* bss_map, struct MHD_Response **p_response, int* status)
{
	struct MHD_Response* response  = NULL;

	json_t* jarray = json_array();

#if 0
	for (auto iter : *bss_map) {
		struct BSS* bss = iter.second;
		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

		json_t* jbss;

		int err = bss_to_json(bss, &jbss, 0);
		// TODO error checking
		(void)err;

		err = json_array_append_new(jarray, jbss);
		// TODO error checking
		(void)err;
	}
#endif

	char* s = json_dumps(jarray, 0);
	if (!s) {
		ERR("%s json dump failed\n", __func__);
		json_decref(jarray);
		return;
	}

	json_decref(jarray);

	response = MHD_create_response_from_buffer(
					strlen(s),
					(void *)s, 
					MHD_RESPMEM_MUST_FREE);
	if (!response) {
		PTR_FREE(s);
		return;
	}

	int ret = MHD_add_response_header(response, 
			"Content-Type", "application/json"
			);
	if (ret != MHD_YES) {
		ERR("%s add response failed ret=%d\n", __func__, ret);
		MHD_destroy_response(response);
		response = NULL;
	}

	if (response) {
		PTR_ASSIGN(*p_response, response);
		*status = MHD_HTTP_OK;
	}
}

// responsible for /api/bssid
void get_bssid_response(const char* bssid_str, ssize_t bssid_len, const BSSMap* bss_map, struct MHD_Response **p_response, int* status)
{
	*p_response = NULL;
	*status = MHD_HTTP_INTERNAL_SERVER_ERROR;

	if (!bssid_str || !*bssid_str || bssid_len < 12) {
		return;
	}

	macaddr bssid;
	int ret = bss_str_to_bss(bssid_str, bssid_len, bssid);
	if (ret<0 ) {
		DBG("%s failed parse bssid=%s\n", __func__, bssid_str);
		return;
	}

	struct BSS* bss = NULL;
//	uint64_t key = BSSID_U64(bssid);
//	try {
//		bss = bss_map->at(key);
//	}
//	catch (std::out_of_range& err) {
//		INFO("%s %s not found\n", __func__, bssid_str);
//	}

	struct MHD_Response* response  = NULL;

	if (bss) {
		json_t* jbss;
		int err = bss_to_json(bss, &jbss, 0);
		// TODO error checking
		(void)err;

		char* s = json_dumps(jbss, 0);
		// TODO error checking

		json_decref(jbss);

		response = MHD_create_response_from_buffer(
						strlen(s),
						(void *)s, 
						MHD_RESPMEM_MUST_FREE);
		if (!response) {
			return;
		}

		ret = MHD_add_response_header(response, 
				"Content-Type", "application/json"
				);
		if (ret != MHD_YES) {
			ERR("%s add response failed ret=%d\n", __func__, ret);
			MHD_destroy_response(response);
			response = NULL;
		}
		*status = MHD_HTTP_OK;
	}
	else {
		// 404
		response = MHD_create_response_from_buffer(
						strlen(notfound),
						(void *)notfound, 
						MHD_RESPMEM_PERSISTENT);
		*status = MHD_HTTP_NOT_FOUND;
	}

	if (response) {
		PTR_ASSIGN(*p_response, response);
	}
}

// starting point for /api 
void get_api_response(const char* url, ssize_t url_len, const BSSMap* bss_map, struct MHD_Response **p_response, int* status)
{
	// MHD callback

	*p_response = NULL;

	if (url_len < 6) {
		return;
	}

	if (strncmp(url, "survey", 6) == 0) {
		get_survey_response(bss_map, p_response, status);
	}
	else if (strncmp(url, "bssid/", 6) == 0) {
		get_bssid_response(url+6, url_len-6, bss_map, p_response, status);
	}

	return ;
}

static size_t get_ext(const char *filename, char ext[], size_t ext_size)
{
	// search backwards looking for last '.'
	// then copy from the . to end of string into ext[]
	size_t filename_len = strlen(filename);
	const char *ptr = filename + filename_len - 1;

	size_t count = 0;
	while (ptr != filename && *ptr != '.') {
		count++;
		ptr--;
	}

	if (*ptr != '.') {
		// no extension
		INFO("%s no extension found for file=%s\n", __func__, filename);
		return 0;
	}

	if (count >= ext_size) {
		INFO("%s extension len=%zu too long for file=%s\n", __func__, count, filename);
		return 0;
	}

	// skip the '.'
	ptr++; 

	strncpy(ext, ptr, ext_size);
	DBG("%s filename=%s found ext=%s\n", __func__, filename, ext);
	return count;
}

// http callback
struct MHD_Response* get_file_response(const char* url)
{
	struct MHD_Response *response = NULL;
	static char *pwd = NULL;
	static size_t pwd_len = 0;

	DBG("%s url=%s\n", __func__, url);

	if (!pwd) {
		pwd = get_current_dir_name();
		if (!pwd) {
			ERR("%s get_current_dir_nameout of memory\n", __func__);
			return NULL;
		}

		pwd_len = strlen(pwd);
	}

	char path[PATH_MAX+1];
	char path_cleaned[PATH_MAX+1];

	int len = snprintf(path, sizeof(path), "%s/%s", pwd, url);
	if (len == sizeof(path)) {
		ERR("%s path too long\n", __func__);
		return NULL;
	}

	if (!realpath(path, path_cleaned) ) {
		ERR("%s path=%s realpath() err=%s\n", __func__, path, strerror(errno));
		return NULL;
	}

	// require realpath to be under current path (no ../.. shennanigans)
	if ( strncmp(pwd, path_cleaned, pwd_len) != 0) {
		ERR("%s path=%s out of range\n", __func__, path_cleaned);
		return NULL;
	}

	struct stat statbuf;
	memset( &statbuf, 0, sizeof(struct stat));
	int err = stat(path_cleaned, &statbuf);
	if (err != 0) {
		ERR("%s path=%s stat failed err=%s\n", __func__, path_cleaned, strerror(errno));
		return NULL;
	}
	if (!((statbuf.st_mode & S_IFMT) & S_IFREG)) {
		ERR("%s path=%s not a file\n", __func__, path_cleaned);
		return NULL;
	}

	int fd = open(path_cleaned, 0);
	if (fd < 0) { 
		ERR("%s %s open failed err=%s\n", __func__, path_cleaned, strerror(errno));
		return NULL;
	}

	response = MHD_create_response_from_fd(statbuf.st_size, fd);
	if (!response) {
		close(fd);
		return NULL;
	}

	// look for a MIME type
	char ext[16];
	size_t ext_len = get_ext(path_cleaned, ext, sizeof(ext));
	if (ext_len != 0) {
		ENTRY entry, *found;
		entry.key = ext;
		entry.data = NULL;
		int ret = hsearch_r(entry, FIND, &found, &mimetypes);
		if (ret) {
			DBG("%s ext=%s mimetype=%s\n", __func__, ext, (char*)found->data);
			enum MHD_Result mhd_ret = MHD_add_response_header(response, 
					"Content-Type",
					(char*)found->data);
			if (mhd_ret != MHD_YES) {
				ERR("%s add response failed ret=%d\n", __func__, ret);
				MHD_destroy_response(response);
				response = NULL;
			}
		}
	}

	return response;
}

enum MHD_Result answer_to_connection (void *arg, 
						struct MHD_Connection *connection,
						const char *url,
						const char *method, 
						const char *version,
						const char *upload_data,
						size_t *upload_data_size, 
						void **con_cls)
{
	// MHD callback
	// TODO finish C++ -> C conversion
	(void)arg;
	(void)connection;
	(void)url;
	(void)method;
	(void)version;
	(void)upload_data;
	(void)upload_data_size;
	(void)con_cls;

	BSSMap* bss_map = (BSSMap*)arg;

	printf ("%s method=%s request for url=%s using version=%s\n", __func__, method, url, version);

	// tinkering with getting stuff from the HTTP connection
	const union MHD_ConnectionInfo* conn_info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
	char* s = inet_ntoa(((struct sockaddr_in*)conn_info->client_addr)->sin_addr);
	printf("client=%s\n", s);

#if 0
	// get the HTTP header args
	KeyValueList headers;
	std::string origin;
	MHD_get_connection_values (connection, MHD_HEADER_KIND, &capture_keys, &headers);
	for (auto& kv : headers) {
		printf("%s header %s=%s\n", __func__, kv.first, kv.second);
		if ( !strncmp(kv.first, "Origin", 6) && strnlen(kv.second,64) < 64) {
			origin = kv.second;
		}
	}

	// get the URL arguments
	KeyValueList args;
	MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, &capture_keys, &args);
	for (auto& kv : args) {
		printf("%s arg %s=%s\n", __func__, kv.first, kv.second);
	}
#endif
	struct MHD_Response *response;
	int status = MHD_HTTP_OK;

	// note signed size
	ssize_t url_len = strlen(url);

	if (strncmp(url, "/api/", 5) == 0) {
		get_api_response(url+5, url_len-5, bss_map, &response, &status);
		// CORS
//		if (origin.length()) {
//			int ret = MHD_add_response_header(response, 
//					"Access-Control-Allow-Origin",
//					origin.c_str());
//			XASSERT(ret==MHD_YES, ret);
//		}
	}
	else {
		response = get_file_response(url);
	}

	// if all else fails, return a simple default page
	if (!response) {
		bss_map_dump(bss_map);

		char p[64];
		const char* msg = "<html><body>Found %zu BSSID</body></html>\n";

		size_t page_len = snprintf(p, sizeof(p), msg, bss_map->count);

		response = MHD_create_response_from_buffer(
						page_len,
						p, 
						MHD_RESPMEM_MUST_COPY);
		// TODO check for error
	}

	enum MHD_Result ret = MHD_queue_response (connection, status, response);
	// TODO check return

	MHD_destroy_response (response);

	return ret;
}


static enum MHD_Result on_client_connect (void *cls,
								const struct sockaddr *addr,
								socklen_t addrlen)
{
	// MHD callback
	(void)cls;
	(void)addrlen;

	char* s = inet_ntoa(((const struct sockaddr_in*)addr)->sin_addr);
	printf("%s client=%s\n", __func__, s);

	return MHD_YES;
}

int setup_scan_netlink_socket(struct netlink_socket_bundle* bun)
{
	// this is the netlink socket we'll use to get scan results
	memset(bun, 0, sizeof(struct netlink_socket_bundle));
	bun->cookie = BUNDLE_COOKIE;
	bun->name = "scanresults";

	bun->cb = nl_cb_alloc(NL_CB_DEFAULT);

	int err;
	err = nl_cb_set(bun->cb, NL_CB_VALID, NL_CB_CUSTOM, scan_survey_valid_handler, (void*)bun);
	XASSERT(err>=0,err);
	err = nl_cb_err(bun->cb, NL_CB_CUSTOM, error_handler, (void*)bun);
	XASSERT(err>=0,err);
	err = nl_cb_set(bun->cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, (void*)bun);
	XASSERT(err>=0,err);
	err = nl_cb_set(bun->cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, (void*)&bun->cb_err);
	XASSERT(err>=0,err);

	bun->nl_sock = nl_socket_alloc_cb(bun->cb);
	XASSERT(bun->nl_sock, 0);

	err = genl_connect(bun->nl_sock);
	if (err) {
		// TODO
		XASSERT(0,err);
	}

	bun->nl80211_id = genl_ctrl_resolve(bun->nl_sock, NL80211_GENL_NAME);
	XASSERT(bun->nl80211_id >= 0, bun->nl80211_id);

	bun->sock_fd = nl_socket_get_fd(bun->nl_sock);
	XASSERT(bun->sock_fd >= 0, bun->sock_fd);

	return 0;
}

int setup_scan_event_sock(struct netlink_socket_bundle* bun)
{
	int err;

	memset(bun, 0, sizeof(struct netlink_socket_bundle));
	bun->cookie = BUNDLE_COOKIE;
	bun->name = "events";

	/* from iw event.c */
	/* no sequence checking for multicast messages */
	bun->cb = nl_cb_alloc(NL_CB_DEFAULT);
	err = nl_cb_set(bun->cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	XASSERT(err>=0,err);
	err = nl_cb_set(bun->cb, NL_CB_VALID, NL_CB_CUSTOM, on_scan_event_valid_handler, (void*)bun);
	XASSERT(err>=0,err);

	bun->nl_sock = nl_socket_alloc_cb(bun->cb);
	err = genl_connect(bun->nl_sock);
	DBG("genl_connect err=%d\n", err);
	if (err) {
		// TODO
		XASSERT(0,err);
	}

	bun->nl80211_id = genl_ctrl_resolve(bun->nl_sock, NL80211_GENL_NAME);
	XASSERT(bun->nl80211_id >= 0, bun->nl80211_id);

	int mcid = get_multicast_id(bun->nl_sock, "nl80211", "scan");
	DBG("mcid=%d\n", mcid);
	err = nl_socket_add_membership(bun->nl_sock, mcid);
	XASSERT(err >=0, err);

	bun->sock_fd = nl_socket_get_fd(bun->nl_sock);
	XASSERT(bun->sock_fd >= 0, bun->sock_fd);

	return 0;
}

static int netlink_read(struct netlink_socket_bundle* bun)
{
	DBG("%s on %s\n", __func__, bun->name);
	do {
		int err = nl_recvmsgs(bun->nl_sock, bun->cb);
		DBG("%s err=%d cb_err=%d\n", __func__, err, bun->cb_err);
		if (err < 0) {
			// TODO
			ERR("%s on %s err=%d: %s\n", __func__, bun->name, err, nl_geterror(err));
			break;
		}
	} while (bun->cb_err > 0);

	return 0;
}

int main(int argc, char* argv[])
{
	struct args args;

	int err = args_parse(argc, argv, &args);
	if (err != 0) {
		return EXIT_FAILURE;
	}

	if (args.debug > 0) {
		log_set_level(LOG_LEVEL_DEBUG);
	}
	struct MHD_Daemon *daemon = NULL;

	BSSMap bss_map = {
		.count = 0,
		.root = NULL
	};

	memset(&mimetypes, 0, sizeof(struct hsearch_data));
	err = hcreate_r(1024, &mimetypes);
	if (!err) {
		return EXIT_FAILURE;
	}

	err = mimetype_parse("/etc/mime.types", &mimetypes);

	daemon = MHD_start_daemon ( MHD_NO_FLAG, PORT, 
			&on_client_connect, NULL,
			&answer_to_connection, (void*)&bss_map, 
			MHD_OPTION_END);
	XASSERT( daemon, 0);

	struct netlink_socket_bundle scan_event_watcher;
	struct netlink_socket_bundle scan_results_watcher;
	memset( &scan_event_watcher, 0, sizeof(struct netlink_socket_bundle));
	memset( &scan_results_watcher, 0, sizeof(struct netlink_socket_bundle));

	if (!args.load_dump_file) {
		err = setup_scan_event_sock(&scan_event_watcher);
		if (err) {
			// TODO
			XASSERT(0,err);
		}

		err = setup_scan_netlink_socket(&scan_results_watcher);
		if (err) {
			// TODO
			XASSERT(0,err);
		}

		// attach the scan results watcher to the scan event watcher so we can call
		// GET_SCAN_RESULTS from the watcher's libnl callback
		scan_event_watcher.more_args = (void*)&scan_results_watcher;

		// attach the bss collection to the get scan results watcher
		scan_results_watcher.more_args = (void*)&bss_map;
	}

	fd_set read_fd_set, write_fd_set, except_fd_set;
	while(1) { 
		MHD_socket max_fd=0;
		FD_ZERO(&read_fd_set);
		FD_ZERO(&write_fd_set);
		FD_ZERO(&except_fd_set);
		err = MHD_get_fdset (daemon,
								&read_fd_set,
								&write_fd_set,
								&except_fd_set,
								&max_fd);
		XASSERT(err==MHD_YES, err);
		printf("max_fd=%d event=%d results=%d\n", max_fd, scan_event_watcher.sock_fd, scan_results_watcher.sock_fd);
		if (scan_event_watcher.sock_fd) {
			FD_SET(scan_event_watcher.sock_fd, &read_fd_set);
		}
		if (scan_results_watcher.sock_fd) {
			FD_SET(scan_results_watcher.sock_fd, &read_fd_set);
		}

		int nfds = max_fd;
		nfds = MAX(scan_event_watcher.sock_fd, nfds);
		nfds = MAX(scan_results_watcher.sock_fd, nfds);

		DBG("calling select nfds=%d\n", nfds);
		err = select(nfds+1, &read_fd_set, &write_fd_set, &except_fd_set, NULL);
		DBG("select err=%d\n", err);

		if (err<0) {
			ERR("select failed err=%d errno=%d \"%s\"\n", err, errno, strerror(errno));
			break;
		}

		if (FD_ISSET(scan_event_watcher.sock_fd, &read_fd_set)) {
			printf("%s event=%d\n", __func__, scan_event_watcher.sock_fd);
			scan_event_watcher.cb_err = 0;
			err = netlink_read(&scan_event_watcher);
			// TODO handle error
			XASSERT(err==0, err);
		}

		if (FD_ISSET(scan_results_watcher.sock_fd, &read_fd_set)) {
			printf("%s result=%d\n", __func__, scan_results_watcher.sock_fd);
			scan_results_watcher.cb_err = 1;
			err = netlink_read(&scan_results_watcher);
			// TODO handle error
			XASSERT(err==0, err);
		}

		err = MHD_run_from_select(daemon, &read_fd_set,
									&write_fd_set,
									&except_fd_set);
		XASSERT(err==MHD_YES, err);
	}

	MHD_stop_daemon (daemon);
	hdestroy_r(&mimetypes);
	return 0;
}


