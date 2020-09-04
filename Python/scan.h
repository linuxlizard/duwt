/*
 * scan.h  Handle nl80211 scanning
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2019-2020 David Poole <dpoole@cradlepoint.com>
 */

#ifndef SCAN_H
#define SCAN_H

#define SCAN_REQ_MAX_SSID_COUNT      8  // Max number of SSID's supported in a scan request; a value of '0' indicates "all"

#define SCAN_REQ_MAX_SSID_LEN       32  // Max length of an SSID supported in a scan request

#define SCAN_REQ_MAX_FREQ_COUNT     16  // Max number of frequencies supported in a scan request; a value of '0' indicates "all"

PyObject* request_scan(PyObject *self, PyObject *args);
PyObject* get_scan(PyObject *self, PyObject *args);

#endif
