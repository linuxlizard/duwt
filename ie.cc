#include <iostream>

#include <sys/socket.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>

#include "ie.hh"

IE::IE(uint8_t id, uint8_t len, uint8_t *buf) 
{
	this->id = id;
	this->len = len;
	this->buf = new uint8_t[len];
	memcpy(this->buf, buf, len);
}

IE::~IE() 
{
	delete [] buf;
}

