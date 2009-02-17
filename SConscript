# -*- python -*-
# $Header: /nfs/slac/g/glast/ground/cvs/EbfWriter/SConscript,v 1.5 2009/01/23 00:06:34 ecephas Exp $
# Authors: Brian Winer <winer@mps.ohio-state.edu> 
# Version: EbfWriter-01-08-00
import os
Import('baseEnv')
Import('listFiles')
Import('packages')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('EbfWriterLib', depsOnly = 1)
EbfWriter = libEnv.SharedLibrary('EbfWriter', listFiles(['src/*.cxx','src/Dll/*.cxx']))

progEnv.Tool('EbfWriterLib')
test_EbfWriter = progEnv.GaudiProgram('test_EbfWriter', listFiles(['src/test/*.cxx']), test = 1)

if baseEnv['PLATFORM'] != 'win32':
	progEnv.Tool('registerObjects', package = 'EbfWriter', libraries = [EbfWriter], testApps = [test_EbfWriter], 
	includes = listFiles(['EbfWriter/*.h']))

if baseEnv['PLATFORM'] == 'win32':
	progEnv.Tool('registerObjects', package = 'EbfWriter', libraries = [EbfWriter], testApps = [test_EbfWriter], 
	includes = listFiles(['EbfWriter/*.h', 'src/*.h']))




