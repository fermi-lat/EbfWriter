# -*- python -*-
# $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/EbfWriter/SConscript,v 1.3 2008/11/12 19:29:31 ecephas Exp $
# Authors: Brian Winer <winer@mps.ohio-state.edu> 
# Version: EbfWriter-01-07-02
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




