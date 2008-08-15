#$Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/EbfWriter/EbfWriterLib.py,v 1.1 2008/07/09 21:13:39 glastrm Exp $
def generate(env, **kw):
    if not kw.get('depsOnly', 0):
        env.Tool('addLibrary', library = ['EbfWriter'])
    env.Tool('EventLib')
    env.Tool('guiLib')
    env.Tool('identsLib')
    env.Tool('AcdUtilLib')

def exists(env):
    return 1;
