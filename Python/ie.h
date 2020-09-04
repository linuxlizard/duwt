/*
 * ie.h  Parse Information Elements (IE) in Wi-Fi packets.
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2019-2020 David Poole <dpoole@cradlepoint.com>
 */

#ifndef IE_H
#define IE_H

const char* ie_get_name(uint8_t id);

int ie_decode(uint8_t id, uint8_t len, uint8_t* buf, PyObject* dest_dict);

#endif

