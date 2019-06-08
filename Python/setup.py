from distutils.core import setup, Extension

module1 = Extension('iw',
                    sources = ['iw.c'])

setup (name = 'PackageName',
       version = '1.0',
       description = 'This is the iw package',
       ext_modules = [module1])
