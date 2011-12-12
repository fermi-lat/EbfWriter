// $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/EbfWriter/src/EbfWriter.cxx,v 1.28 2011/05/20 15:01:26 heather Exp $

/*
 * HISTORY
 *
 * DATE         WHO    WHAT
 * 5/13/05  winer      Changes to bring the Ebf file in line with the current
 *                     hardware configuration. In addition, provide a means to
 *                     generate test vectors for the Front-End Simulator System.
 * 5/28/03  russell    Added what I deemed to be a kludge to take care of the 
 *                     fact that the algorithm gets called make after all the
 *                     events have been read. If any of the DIGI pointers are
 *                     NULL, I take this as bad, and just return -1. I think
 *                     getting called back when there are no more events to
 *                     process is a bug.
 * 6/11/03  navid      Renamed this package to EbfWriter
 */
           

#include <stdio.h>

// Gaudi system includes
#include "GaudiKernel/Algorithm.h"

#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/IParticlePropertySvc.h"
#include "GaudiKernel/ParticleProperty.h"

// TDS class declarations: input data, and McParticle tree
#include "Event/TopLevel/MCEvent.h"
#include "Event/MonteCarlo/McParticle.h"
#include "Event/MonteCarlo/McIntegratingHit.h"
#include "Event/MonteCarlo/McPositionHit.h"
#include "Event/MonteCarlo/Exposure.h"
#include "Event/TopLevel/Event.h"
#include "Event/TopLevel/EventModel.h"
#include "Event/Recon/CalRecon/CalCluster.h"

#include "LdfEvent/Gem.h"

#include "Event/Digi/TkrDigi.h"
#include "Event/Digi/CalDigi.h"
#include "Event/Digi/AcdDigi.h"
#include "Trigger/TriRowBits.h"
#include "EbfWriter/Ebf.h"

#include "EbfAcdData.h"
#include "EbfCalData.h"
#include "EbfTkrData.h"
#include "EbfGemData.h"
#include "EbfGemCounters.h"
#include "EbfOutput.h"
#include "FESOutput.h"

#include "astro/PointingTransform.h"
#include "facilities/Util.h"

// Cal Simulation info
#include "CalXtalResponse/ICalTrigTool.h"

#include <fstream>
#include <cassert>
#include <map>
using namespace CLHEP;
/** 
 * @class EbfWriter
 * @brief An algorithm to convert the digi data to ebf
 * 
 * $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/EbfWriter/src/EbfWriter.cxx,v 1.28 2011/05/20 15:01:26 heather Exp $
*/
class EbfWriter : public Algorithm 
{
public:
    EbfWriter(const std::string& name, ISvcLocator* pSvcLocator);

    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize();
    StatusCode getMcEvent(double obsEn);
    StatusCode getPointingInfo();
    
private:

// Control Output File for EBF
    EbfOutput m_output;

// Output Routines for FES Format
    FESOutput f_output;

    EbfGemCounters  m_latcounters;
    
    /// parameter to store the maximum size of an event
    /// this should be fairly static
    int         m_maxEvtSize;
    int         m_TkrEncodeFlag;
    int         m_CalEncodeFlag;
    int         m_FesFiles;
    int         m_FesVersion;
    bool        m_WriteEbf;
    bool        m_LdfFormat;
    bool        m_storeOnTds;
    bool        m_reqGemTrig;

    int m_sourceId;
    int m_sequence;
    int m_countEbfEvents;

     std::map<std::string, double> getCelestialCoords(const Hep3Vector glastDir);

    // Monte Carlo that is stored in the 8 word header of the Ebf File 
    struct  McInfo
    {
       unsigned int                 mcId;
       unsigned short int           rsvd;
       unsigned char             acdTile;
       unsigned char          sourceType;
       unsigned short int          absRA;
       unsigned short int         absDec;
       unsigned short int       relTheta;
       unsigned short int         relPhi;
       unsigned short int   actualEnergy;
       unsigned short int observedEnergy;
    } m_McInfo;

    // Time of the previous event
    double PreEvtTime;
    
    // File name fo the EbfFile
    std::string m_FileName;
    std::string m_FileDesc;

    // to decode the particle charge
    IParticlePropertySvc* m_ppsvc;

    /// use to calculate full cal trigger response 
    ICalTrigTool *m_calTrigTool;

};



//static const AlgFactory<EbfWriter>  Factory;
//const IAlgFactory& EbfWriterFactory = Factory;
DECLARE_ALGORITHM_FACTORY(EbfWriter);



EbfWriter::EbfWriter(const std::string& name, ISvcLocator* pSvcLocator) :
  Algorithm(name, pSvcLocator),
  m_calTrigTool(0)
{
    // declare properties with setProperties calls
    declareProperty("MaxEvtSize",m_maxEvtSize=0x10000);
    declareProperty("FileName"  ,m_FileName="");
    declareProperty("FileDesc"  ,m_FileDesc="Generated by GLEAM");
    declareProperty("WriteEbf"  ,m_WriteEbf=false);
    declareProperty("FESFiles"  ,m_FesFiles=0);
    declareProperty("FESVersion",m_FesVersion=0);
    declareProperty("TkrEncode" ,m_TkrEncodeFlag=-1);
    declareProperty("CalEncode" ,m_CalEncodeFlag=-1);
    declareProperty("LdfFormat" ,m_LdfFormat=false);
    declareProperty("StoreOnTds",m_storeOnTds=true);
    declareProperty("ReqGemTrig",m_reqGemTrig=true);
 
    return;
}



//! set parameters and attach to various perhaps useful services.
StatusCode EbfWriter::initialize()
{

    StatusCode  sc = StatusCode::SUCCESS;
    MsgStream      log(msgSvc(), name());

    log << MSG::INFO << "initialize" << endreq;
    
    // Use the Job options service to set the Algorithm's parameters
    setProperties();
    
    if( serviceLocator() ) {
        if( service("ParticlePropertySvc", m_ppsvc, true).isFailure() ) {
            log << MSG::ERROR << "Service [ParticlePropertySvc] not found" << endreq;
        }
    } else {
        return StatusCode::FAILURE;
    }

    m_output.setPrint (log.level () <= MSG::DEBUG);
    m_output.setLdfFormat(m_LdfFormat);
    
    // first expand the filename if env var 
    std::string filename(m_FileName); 
    facilities::Util::expandEnvVar(&filename);

    m_output.open(filename.c_str(), m_maxEvtSize);

//     m_input.open (m_FileName.c_str(), m_maxEvtSize);

    
    /* Set up the EBF time stuff */
    m_latcounters.initialize ((unsigned int)2*20000000, // T0 = 2 seconds
                              (unsigned int)0,   // Initialize dead time value
                              (double)20000000., // LAT clock frequency
                              (double).00001,    // Amp of variation,
                                                 // 1 part in 10**-5
                              (double)90 * 60.); // Period of the clock
                                                 // variation, in secs


    /* Make FES Files */    
    f_output.setVersion(m_FesVersion);
    if(m_FesFiles>0) f_output.open(filename.c_str(),m_FileDesc.c_str());

    PreEvtTime = 0.;
    m_countEbfEvents =0;

//    printf("EbfWriter: Initialization done\n");

    sc = toolSvc()->retrieveTool("CalTrigTool",
                                 "CalTrigTool", 
                                 m_calTrigTool,
                                 0); // could be shared
    if (sc.isFailure() ) {
      log << MSG::ERROR << "  Unable to create CalTrigTool" << endreq;
      return sc;
    }
        
   return sc;
}



StatusCode EbfWriter::execute()
{
    //
    // Procedure and Method:  
    //     Process one event, extracting all the colections from the TDS, and
    //     passing them (if they exist) to individual functions
    //

    MsgStream  log (msgSvc(), name());
    MSG::Level level = log.level ();
    int        print = level <= MSG::INFO;
    StatusCode    sc = StatusCode::SUCCESS;    
    
    DataObject *pnode=0;
    if(eventSvc()->retrieveObject("/Event/Filter",pnode).isFailure())
      eventSvc()->registerObject("/Event/Filter",new DataObject);

// Taking input from GLEAM (i.e. digis)
    EbfTkrData tkr;
    EbfCalData cal; 
    EbfGemData gem;
    EbfAcdData acd;
        
//     printf("In EbfWriter Execute: ReadFile=%i\n",m_ReadFile);   
        
    //
    // Grab event header to pass event info
    SmartDataPtr<Event::EventHeader>header(eventSvc(), 
                                           EventModel::EventHeader);

    //
    // TKR
    // ---
    //
    SmartDataPtr<Event::TkrDigiCol> 
      tkrDigiCol(eventSvc(), EventModel::Digi::TkrDigiCol);
    
    // If no tracker data... 
    if (!tkrDigiCol) return StatusCode::FAILURE;
    
    //        TriRowBitsTds::TriRowBits *rowbits= new TriRowBitsTds::TriRowBits;
    //        eventSvc()->retrieveObject("/Event/TriRowBits", rowbits);
    SmartDataPtr<TriRowBitsTds::TriRowBits> rowbits(eventSvc(), "/Event/TriRowBits"); 
    
    if(m_TkrEncodeFlag<0) {
      tkr.fill  (tkrDigiCol,rowbits);
    } else {
      tkr.fillEncode(m_TkrEncodeFlag,header->event());    
    }
    if(level <= MSG::DEBUG) tkr.print();
    
    //
    // CAL
    // ---
    //
    SmartDataPtr<Event::CalDigiCol> 
      calDigiCol(eventSvc(), EventModel::Digi::CalDigiCol);  
    // If no CAL data... 
    if (!calDigiCol) return StatusCode::FAILURE;
    
               
    if(m_CalEncodeFlag<0) {
      if (cal.fill(calDigiCol, *m_calTrigTool).isFailure())
        return StatusCode::FAILURE;
    } else {
      cal.fillEncode    (m_CalEncodeFlag, header->event());
    }  
    if(level <= MSG::DEBUG) cal.print();
    
    //
    // ACD
    // ---
    //
    SmartDataPtr<Event::AcdDigiCol> 
      acdDigiCol(eventSvc(), EventModel::Digi::AcdDigiCol);
    
    // If no ACD data... 
    if (!acdDigiCol) return StatusCode::FAILURE;
    
    
    acd.fill  (acdDigiCol);
    if(level <= MSG::DEBUG) acd.print();
    
    //
    // GEM
    // ---
    // 
    // Retrieve the LdfEvent::Gem.h object from the TDS
    SmartDataPtr<LdfEvent::Gem> gemTds(eventSvc(), "/Event/Gem");
    if(!gemTds) 
    {
        log << MSG::INFO << "Could not retrieve the Gem object from the TDS" << endreq; 
        return StatusCode::FAILURE;
    }
    
    gem.fill (gemTds,
              header->event(),
              header->time(),
              &m_latcounters,
              &acd, 
              &tkr,
              &cal);
    if(level <= MSG::DEBUG) gem.print();   
    
    
    // Get the Observed Calorimeter Energy
    // This is the old method...single pedestal etc.
    //        double obsEn = cal.getTotalEnergy();
    
    // This is new way that lets CalRecon deal with all the peds and gains
    double obsEn = 0.;
    SmartDataPtr<Event::CalClusterCol> pCals(eventSvc(),EventModel::CalRecon::CalClusterCol);
    if(pCals) {
      if(!(pCals->empty())){
        Event::CalCluster* calCluster = pCals->front();
        obsEn  = calCluster->getMomParams().getEnergy();
      }
    } else {
      //           log << MSG::WARNING << "No calCluster found: No Obs. will be stored in EBF file header" << endreq; 
    }

        
    // Dump MC Information if it exists
    getMcEvent(obsEn);
    //        getPointingInfo();
    
    
    // Check if another copy of EbfWriter previously stored the EBF data
    // to the TDS.  If so, we want to use the sequence number associated
    // with this initial instance, so we decrement the sequence number
    // by one, since the format call will increment it again
    DataObject *orgEbf;
    StatusCode mySc = eventSvc()->findObject("/Event/Filter/Ebf", orgEbf);
  
 //   SmartDataPtr<EbfWriterTds::Ebf> orgEbf(eventSvc(), "/Event/Filter/Ebf");
    if (mySc.isSuccess() && orgEbf)
      m_output.setNumEvtsOut(dynamic_cast<EbfWriterTds::Ebf*>(orgEbf)->getSequence()-1);
    
    
    //
    // Put the contributor's data into EBF format and write it out 
    //
    unsigned int evtSize = m_output.format (&acd, &cal, &tkr, &gem, (unsigned int *)&m_McInfo, m_reqGemTrig);
    //    if (level <= MSG::DEBUG) 
    //       m_output.print  ();
    
    // Get pointer and size of data
    unsigned int dataSize;
    char *data;
    data = m_output.getData(dataSize);
    
    // Write out the file
    unsigned int length;
    unsigned int *TdsBuffer = 0;
    if(m_storeOnTds) {
        TdsBuffer = (unsigned int *)malloc(m_maxEvtSize*5);
        memset(TdsBuffer,0,m_maxEvtSize*5);
    }
    if(evtSize != 0) {
      data=m_output.write  (m_WriteEbf, length, TdsBuffer);
      m_countEbfEvents++;
    }
    
    // If  we are writing EBF put it into the Tds
    if(evtSize !=0 && m_storeOnTds) {
      
      // register object
      EbfWriterTds::Ebf *newEbf=new EbfWriterTds::Ebf;
      eventSvc()->registerObject("/Event/Filter/Ebf", newEbf);
      
      // Put the object on the Tds
      newEbf->set((char *)TdsBuffer,length);
      newEbf->setSequence(m_output.numEvtsOut());
    }
    
    
    if(m_storeOnTds) free(TdsBuffer);
    //
    // If we are writing LDF format, write a LATDatagram for MC Infomation
    //       if(m_LdfFormat) m_output.writeMC((unsigned int *)&m_McInfo, sizeof(m_McInfo));
    
    
    // Get the delta time in clock ticks since last event.
    double deltaTime = header->time()-PreEvtTime;
    PreEvtTime = header->time();
    int nDeltaTime = m_latcounters.deltaTicks(deltaTime);
    
    // Put the contributor's data into FES format and write it out      
    if(m_FesFiles>0 && evtSize!=0) {
      
      // If this is a gamma event, set a flag to go into FES Files      
      m_McInfo.sourceType == 22 ? f_output.setGammaFlag(true) : f_output.setGammaFlag(false); 
      
      
      f_output.dumpTKR (&tkr,nDeltaTime);
      f_output.dumpCAL (&cal,*m_calTrigTool,nDeltaTime);
      f_output.dumpACD (&acd,nDeltaTime);
      f_output.countTrigger();
      
      f_output.setFirstEvtFlag(false);
    } 
    
    return sc;
}


/// clean up
StatusCode EbfWriter::finalize()
{ 
//    printf("EbfWriter: Finalize\n");
    f_output.close();
    m_output.close();

    MsgStream log(msgSvc(), name());

    log << MSG::INFO << "Wrote "<< m_countEbfEvents<<" Total Events\n" << endreq;
#if 0 // this writes a bunch of stuff that is not clear we want
    f_output.dumpTriggerInfo();
#endif
    return StatusCode::SUCCESS;
}

/// clean up
StatusCode EbfWriter::getPointingInfo()
{

  Event::ExposureCol* elist = 0;
  eventSvc()->retrieveObject("/Event/MC/ExposureCol",(DataObject *&)elist);
  if( elist==0) {
     printf("failed to find pointing info\n");
     return StatusCode::FAILURE; // should not happen, but make sure ok.
  }
  //Event::ExposureCol::iterator curEntry = (*elist).begin();
  const Event::Exposure& exp = **(*elist).begin();

  double time     = exp.intrvalstart();
  double latitude = exp.lat();
  double longitude= exp.lon();
  double altitude = exp.alt();
  double posX     = exp.posX();
  double posY     = exp.posY();
  double posZ     = exp.posZ();
  double RightAsX = exp.RAX();
  double DeclX    = exp.DECX();
  double RightAsZ = exp.RAZ();
  double DeclZ    = exp.DECZ(); 
/*  printf("Attitude Information:  
             Time:     %f
             Latitude: %f
             Longitude:%f
             Altitude: %f
             PosX:     %f
             PosY:     %f
             PosZ:     %f
             RightAsX: %f
             DelX:     %f
             RightAsZ: %f
             DelZ:     %f \n",time,latitude,longitude,altitude,posX,posY,posZ,RightAsX,DeclX,RightAsZ,DeclZ);      
*/
// Some calculated quantities
//   deltaRA = RightAsX - RightAsZ;
//   if(deltaRA > 180.) 
//   deltaDec= DeclX - DelZ;

    return StatusCode::SUCCESS;
}


StatusCode EbfWriter::getMcEvent(double oEn)
{

    SmartDataPtr<Event::MCEvent> mcEvt(eventSvc(), EventModel::MC::Event);
    SmartDataPtr<Event::McParticleCol> mcParticle(eventSvc(), EventModel::MC::McParticleCol);
    
    
    m_McInfo.mcId           = mcEvt->getSequence();
    m_McInfo.rsvd           = 0;
    m_McInfo.acdTile        = 0xAA;
    m_McInfo.absRA          = 0xCCCC;
    m_McInfo.absDec         = 0xDDDD;
    m_McInfo.relTheta       = 0xEEEE;
    m_McInfo.relPhi         = 0xFFFF;
    m_McInfo.observedEnergy = 0x2222;
    
//    printf("Monte Carlo sourceId %i and sequence %i\n",m_sourceId,m_sequence); 


    double MC_Id = 0;
    double MC_Charge = 9999;
    double MC_Energy = 0.;
    double MC_x0 =9999.;
    double MC_y0 =9999.;
    double MC_z0 =9999.;
    double MC_xdir = 9999.;
    double MC_ydir = 9999.;
    double MC_zdir = 9999.;
    
    
    if(mcParticle) {
         Event::McParticleCol::const_iterator pMCPrimary = mcParticle->begin();
        // Skip the first particle... it's for bookkeeping.
        // The second particle is the first real propagating particle.
        // (except for beam test mc!)

        unsigned  int numIncident = (*pMCPrimary)->daughterList().size();
        if(numIncident==0) return StatusCode::SUCCESS;
        
        if(numIncident == 1) {
            Event::McParticle::StdHepId hepid= (*pMCPrimary)->particleProperty();
            MC_Id = (double)hepid;
	   
            ParticleProperty* ppty = m_ppsvc->findByStdHepID( hepid );
            if (ppty) {
                std::string name = ppty->particle(); 
                MC_Charge = ppty->charge();          
            }
            pMCPrimary++;
        }
	
	 int mcPartID = (int)(fabs(MC_Id));
	 if(mcPartID>127) mcPartID = 128;
         m_McInfo.sourceType     = mcPartID;
        
        HepPoint3D Mc_x0;
        // launch point for charged particle; conversion point for neutral
        Mc_x0 = (MC_Charge==0 ? (*pMCPrimary)->finalPosition() : (*pMCPrimary)->initialPosition());
        HepLorentzVector Mc_p0 = (*pMCPrimary)->initialFourMomentum();
        // there's a method v.m(), but it does something tricky if m2<0
        double mass = sqrt(std::max(Mc_p0.m2(),0.0));
        
//        Vector Mc_t0 = Vector(Mc_p0.x(),Mc_p0.y(), Mc_p0.z()).unit();
        
        //Pure MC Tuple Items
        // get monte carlo energy
         unsigned int mcEn   = (unsigned int)std::max(Mc_p0.t() - mass, 0.0);

         // Encode the energy in 16 bits highest 4 bits are powers of two
         // Lowest 12 bits are most sign 12 bits...value from mc generator
         // is mcEn.
         unsigned int mcEnValue = 0;
         int pwr2 = 0;
         while ((mcEn & 0xfffff000) != 0) {
            mcEn >>= 1;
            pwr2++;
         } 
         if(pwr2<15) {
            mcEnValue = (pwr2 << 12) | mcEn;
         } else {
            mcEnValue = 0xffff;
         }
//         printf("mcEn 0x%8.8x  pwr2 %i (0x%1.1x) mcEnValue 0x%8.8x \n",mcEn,pwr2,pwr2,mcEnValue);
         m_McInfo.actualEnergy   = (unsigned short int)mcEnValue;


         // Encode the energy in 16 bits highest 4 bits are powers of two
         // Lowest 12 bits are most sign 12 bits...value of the observed value
         // is obsEn.
         unsigned int obsEn = (unsigned int)oEn;
         unsigned int obsEnValue = 0;
         pwr2 = 0;
         while ((obsEn & 0xfffff000) != 0) {
            obsEn >>= 1;
            pwr2++;
         } 
         if(pwr2<15) {
            obsEnValue = (pwr2 << 12) | obsEn;
         } else {
            obsEnValue = 0xffff;
         }
         m_McInfo.observedEnergy   = (unsigned short int)obsEnValue;
//         printf("Observed Energy %i Gen Energy %i\n",obsEn,mcEn);

        
// Location of generation point in local coordinates        
        MC_x0     = Mc_x0.x();
        MC_y0     = Mc_x0.y();
        MC_z0     = Mc_x0.z();

// Direction vectors        
        MC_xdir   = Mc_p0.x();
        MC_ydir   = Mc_p0.y();
        MC_zdir   = Mc_p0.z();         

// Attempt to convert these to RA and DEC
//        printf("MC Dir x: %f  y: %f  z: %f \n",MC_xdir,MC_ydir,MC_zdir);
        
        Hep3Vector glastDir = Hep3Vector(MC_xdir,MC_ydir,MC_zdir);

// Reverse direction to point back to sky
        glastDir = - glastDir.unit();

// Get the relative Theta and Phi
        double pi    = 3.14159; /* This is silly */
        double theta = glastDir.theta();
        double phi   = glastDir.phi();
        if(phi<0) phi += 2*pi;
   
        m_McInfo.relTheta = (unsigned short int)((theta/(2*pi))*(0x1<<16));
        m_McInfo.relPhi   = (unsigned short int)((phi/(2*pi))*(0x1<<16)); 


// Attempt to convert these to RA and DEC
// Adjust to make values positive definite so we can store as an integer.
    std::map<std::string,double> cel_coords = getCelestialCoords(glastDir);
    double absRA = cel_coords["RA"];
    if(absRA<0.) absRA += 360.0;
    double absDec= cel_coords["DEC"]+90.;
//    printf("RA %f  Dec %f",absRA,absDec);
    m_McInfo.absRA  = (unsigned short int) ((absRA/360.) *(0x1<<16));
    m_McInfo.absDec = (unsigned short int) ((absDec/180.)*(0x1<<16));
//    printf("absRA 0x%4.4x absDec 0x%4.4x\n",m_McInfo.absRA,m_McInfo.absDec);
       

   }
//   printf("Particle ID %i: Charge %f; Energy %i; x,y,z, %f:%f:%f; Dir %e:%e:%e\n",m_McInfo.sourceType,MC_Charge,m_McInfo.actualEnergy,MC_x0,MC_y0,MC_z0,MC_xdir,MC_ydir,MC_zdir);
   return StatusCode::SUCCESS;
}
//------------------------------------------------------------------------------
//  This was taken from meritAlg.cxx.  It was a private method, why
// isn't this sort of thing in a standard library someplace?
//
std::map<std::string, double> EbfWriter::getCelestialCoords(const Hep3Vector glastDir)
{
    using namespace astro;

    std::map<std::string, double> fields;

    //First get the coordinates from the ExposureCol
    Event::ExposureCol* elist = 0;
    eventSvc()->retrieveObject("/Event/MC/ExposureCol",(DataObject *&)elist);
    if( elist==0) return fields; // should not happen, but make sure ok.
    const Event::Exposure& exp = **(*elist).begin();

    // create a transformation object -- first get local directions
    SkyDir zsky( exp.RAZ(), exp.DECZ() );
    SkyDir xsky( exp.RAX(), exp.DECX() );
    // orthogonalize, since interpolation and transformations destory orthogonality (limit is 10E-8)
    Hep3Vector xhat = xsky() -  xsky().dot(zsky()) * zsky() ;
    PointingTransform toSky( zsky, xhat );

    // make zenith (except for oblateness correction) unit vector
    Hep3Vector position( exp.posX(),  exp.posY(),  exp.posZ() );
    SkyDir zenith(position.unit());

    SkyDir sdir = toSky.gDir(glastDir);

    //zenith_theta and earth_azimuth
    double zenith_theta = sdir.difference(zenith); 
    if( fabs(zenith_theta)<1e-8) zenith_theta=0;

    SkyDir north_dir(90,0);
    SkyDir east_dir( north_dir().cross(zenith()) );
    double earth_azimuth=atan2( sdir().dot(east_dir()), sdir().dot(north_dir()) );
    if( earth_azimuth <0) earth_azimuth += 2*M_PI; // to 0-360 deg.
    if( fabs(earth_azimuth)<1e-8) earth_azimuth=0;

    fields["RA"]            = sdir.ra();
    fields["DEC"]           = sdir.dec();
    fields["ZENITH_THETA"]  = zenith_theta*180/M_PI;
    fields["EARTH_AZIMUTH"] = earth_azimuth*180/M_PI;
//    printf("getCelestial: RA %f DEC %f\n",sdir.ra(),sdir.dec());
    
    return fields;
}
