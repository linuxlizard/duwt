#ifndef IE_H
#define IE_H

// From IEEE802.11-2016:
//
// This revision is based on IEEE Std 802.11-2012, into which the following amendments have been
// incorporated:
// — IEEE Std 802.11aeTM-2012: Prioritization of Management Frames (Amendment 1)
// — IEEE Std 802.11aaTM-2012: MAC Enhancements for Robust Audio Video Streaming (Amendment 2)
// — IEEE Std 802.11adTM-2012: Enhancements for Very High Throughput in the 60 GHz Band (Amendment 3)
// — IEEE Std 802.11acTM-2013: Enhancements for Very High Throughput for Operation in Bands below 6 GHz (Amendment 4)
// — IEEE Std 802.11afTM-2013: Television White Spaces (TVWS) Operation (Amendment 5)

// From IEEE802.11-2012:
//
// <quote>
// The original standard was published in 1999 and reaffirmed in 2003. A revision was published in 2007,
// which incorporated into the 1999 edition the following amendments: IEEE Std 802.11aTM-1999,
// IEEE Std 802.11bTM-1999, IEEE Std 802.11b-1999/Corrigendum 1-2001, IEEE Std 802.11dTM-2001, IEEE
// Std 802.11gTM-2003, IEEE Std 802.11hTM-2003, IEEE Std 802.11iTM-2004, IEEE Std 802.11jTM-2004 and
// IEEE Std 802.11eTM-2005.
//
// The current revision, IEEE Std 802.11-2012, incorporates the following amendments into the 2007 revision:
// — IEEE Std 802.11kTM-2008: Radio Resource Measurement of Wireless LANs (Amendment 1)
// — IEEE Std 802.11rTM-2008: Fast Basic Service Set (BSS) Transition (Amendment 2)
// — IEEE Std 802.11yTM-2008: 3650–3700 MHz Operation in USA (Amendment 3)
// — IEEE Std 802.11wTM-2009: Protected Management Frames (Amendment 4)
// — IEEE Std 802.11nTM-2009: Enhancements for Higher Throughput (Amendment 5)
// — IEEE Std 802.11pTM-2010: Wireless Access in Vehicular Environments (Amendment 6)
// — IEEE Std 802.11zTM-2010: Extensions to Direct-Link Setup (DLS) (Amendment 7)
// — IEEE Std 802.11vTM-2011: IEEE 802.11 Wireless Network Management (Amendment 8)
// — IEEE Std 802.11uTM-2011: Interworking with External Networks (Amendment 9)
// — IEEE Std 802.11sTM-2011: Mesh Networking (Amendment 10)
//
// As a result of publishing this revision, all of the previously published amendments and revisions are now
// retired.
// </quote>

// https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/
#include <unicode/utypes.h>
#include <unicode/utext.h>
#include <unicode/utf8.h>

typedef enum {
	IE_SSID = 0,
	IE_MESH_ID = 114,
	IE_EXTENDED_CAPABILITIES = 127,
	IE_VENDOR = 221,
		
} IE_ID;

extern const uint8_t ms_oui[3];
extern const uint8_t ieee80211_oui[3];
extern const uint8_t wfa_oui[3];

#define IE_COOKIE 0xdc4904e3
#define IE_SPECIFIC_COOKIE 0x65cb47f0
#define IE_LIST_COOKIE 0xcb72f5ff

// general "base class" of all IE
struct IE 
{
	uint32_t cookie;
	IE_ID id;
	size_t len;
	// raw bytes of the IE; does not include id+len
	uint8_t* buf;

	// constructor
//	void (*create)(uint8_t id, uint8_t len, uint8_t* buf);

	// destructor
	void (*free)(struct IE* self);

	// blob of specific IE info
	void* specific;
};


// base is a pointer to the struct IE that contains us
#define IE_SPECIFIC_FIELDS\
	uint32_t cookie;\
	struct IE* base;

// standard says 32 octets but Unicode seriously muddies the water
#define SSID_MAX_LEN 32

#define IE_CAST(ieptr, type)\
	((type*)ie->specific)

struct IE_SSID
{
	IE_SPECIFIC_FIELDS

	// http://userguide.icu-project.org/strings
	// http://userguide.icu-project.org/strings/utf-8
	UChar ssid[SSID_MAX_LEN*2];
	int32_t ssid_len;
};

struct IE_Extended_Capabilities
{
	IE_SPECIFIC_FIELDS
};

#define IE_VENDOR_OUI_LEN 5
struct IE_Vendor
{
	IE_SPECIFIC_FIELDS

	uint8_t oui[IE_VENDOR_OUI_LEN];
};

struct IE_List
{
	uint32_t cookie;
	// array of pointers
	struct IE** ieptrlist;
	size_t max;
	size_t count;
};

struct IE* ie_new(uint8_t id, uint8_t len, const uint8_t* buf);
void ie_delete(struct IE** pie);

#define IE_LIST_BLANK\
	{ .cookie = 0, .max = 0, .count = 0 }

int ie_list_init(struct IE_List* list);
void ie_list_release(struct IE_List* list);
int ie_list_move_back(struct IE_List* list, struct IE** pie);
void ie_list_peek(const char *label, struct IE_List* list);
const struct IE* ie_list_find_id(struct IE_List* list, IE_ID id);

int decode_ie_buf( const uint8_t* buf, size_t len, struct IE_List* ielist);


#endif

