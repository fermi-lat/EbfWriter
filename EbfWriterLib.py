#$Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/EbfWriter/EbfWriterLib.py,v 1.2 2008/11/12 19:34:50 ecephas Exp $
def generate(env, **kw):
    if not kw.get('depsOnly', 0):
        env.Tool('addLibrary', library = ['EbfWriter'])
    env.Tool('CalXtalResponseLib')
    env.Tool('TriggerLib')
    env.Tool('CalUtilLib')
    env.Tool('FluxSvcLib')
    env.Tool('fluxLib')
    env.Tool('EventLib')
    env.Tool('GlastSvcLib')
    env.Tool('astroLib')
def exists(env):
    return 1;
