/** 
* @file EbfWriter_load.cxx
* @brief This is needed for forcing the linker to load all components
* of the library.
*
*  $Header: /nfs/slac/g/glast/ground/cvs/EbfWriter/src/Dll/EbfWriter_load.cxx,v 1.6 2002/04/11 23:20:21 usher Exp $
*/

#include "GaudiKernel/DeclareFactoryEntries.h"

// There should be one entry for each component included in 
// the library for this package.
DECLARE_FACTORY_ENTRIES(EbfWriter) {
    DECLARE_ALGORITHM(EbfWriter);
} 
