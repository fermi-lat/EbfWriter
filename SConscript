# -*- python -*-
# $Header: /nfs/slac/g/glast/ground/cvs/EbfWriter/SConscript,v 1.12 2010/12/16 16:16:55 lsrea Exp $
# Authors: Brian Winer <winer@mps.ohio-state.edu> 
# Version: EbfWriter-01-08-04
import os
Import('baseEnv')
Import('listFiles')
Import('packages')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('addLinkDeps', package='EbfWriter', toBuild='component')
EbfWriter = libEnv.SharedLibrary('EbfWriter',
                                 listFiles(['src/*.cxx','src/Dll/*.cxx']))

progEnv.Tool('EbfWriterLib')

test_EbfWriter = progEnv.GaudiProgram('test_EbfWriter',
                                      listFiles(['src/test/*.cxx']),
                                      test = 1, package='EbfWriter')

progEnv.Tool('registerTargets', package='EbfWriter',
             libraryCxts=[[EbfWriter, libEnv]],
             testAppCxts = [[test_EbfWriter, progEnv]], 
             includes = listFiles(['EbfWriter/*.h']),
             jo = ['src/test/jobOptions.txt','src/test/test_sources.xml])
