from distutils.core import setup, Extension

module1 = Extension('iw',
				sources = ['scan.c', 'iw.c', 'linux_wireless_control.c', 
						'linux_netlink_control.c',
						'wifi_ht_channels.c',
						],
				include_dirs = ['/usr/include/libnl3',],
				libraries = [ 'nl-3', 'nl-genl-3', ],
			)

setup (name = 'PackageName',
       version = '1.0',
       description = 'This is the iw package',
       ext_modules = [module1])
