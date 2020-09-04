# setup.py  python nl80211 library build
#
#	This library is free software; you can redistribute it and/or
#	modify it under the terms of the GNU Lesser General Public
#	License as published by the Free Software Foundation version 2.1
#	of the License.
#
# Copyright (c) 2019-2020 David Poole <dpoole@cradlepoint.com>
#

from distutils.core import setup, Extension

module1 = Extension('_iw',
            sources = ['scan.c', 'iw.c', 'ie.c', 'event.c', 'util.c',
                       'linux_wireless_control.c', 
                       'linux_netlink_control.c',
                       'wifi_ht_channels.c',
                      ],
            include_dirs = ['/usr/include/libnl3',],
            libraries = [ 'nl-3', 'nl-genl-3', ],
            extra_compile_args = [ '-Wextra', '-Wshadow', '-Wundef' ],
        )

setup (name = 'PackageName',
       version = '1.0',
       description = 'This is the iw package',
       ext_modules = [module1])
