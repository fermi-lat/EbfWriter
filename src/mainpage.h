//$Header: /nfs/slac/g/glast/ground/cvs/EbfWriter/src/mainpage.h,v 1.1.1.1 2003/07/03 18:13:57 golpa Exp $
// (Special "header" just for doxygen)

/** @mainpage  package EbfWriter

This package is a "user hook" allowing development of flight-oriented algrorithms in the 
Glast Gaudi environment.  Since it "uses" the global package Gleam, it depends on all 
packages needed to build the simulation/ reconstruction environment.  This package has 2 functions:

  - Convert the hit information obtained from the TDS into the on-board format to write to TDS

  - Encapsulate flight code, to allow evalutaion of flight algorithms in the off-line enviornment.

  This package demonstrates how to:
  <ul>
    <li> access to all quantities in the Transient Data Store (TDS)</li>
    <li> writing text output to the log file (which can be turned on/off with the gui "printer").
    <li> generating histograms and tuples, which would then be written to a ROOT file. (Need to document this.)
  </ul>

  <hr>
  @section jobOptions src/jobOptions.txt
  @include jobOptions.txt
  <hr>
  @section notes release.notes
  release.notes
  <hr>
  @section requirements requirements
  @include requirements

*/
