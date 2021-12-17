from distutils.core import setup, Extension
import numpy

setup(name = 'bitset', version = '1.0',  \
   ext_modules = [Extension('bitset', ['bitset.cpp'], include_dirs=[numpy.get_include()], extra_compile_args=['-O3'], language='c++')])