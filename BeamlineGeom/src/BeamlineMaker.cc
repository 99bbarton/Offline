//
// Construct and return an Beamline.
//
//
// $Id: BeamlineMaker.cc,v 1.20 2013/08/22 14:31:33 knoepfel Exp $
// $Author: knoepfel $
// $Date: 2013/08/22 14:31:33 $
//
// Original author Peter Shanahan
//                 Kyle Knoepfel (significant updates)
//

// C++ includes
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e includes
#include "BeamlineGeom/inc/Beamline.hh"
#include "BeamlineGeom/inc/BeamlineMaker.hh"
#include "BeamlineGeom/inc/TransportSolenoid.hh"
#include "BeamlineGeom/inc/Collimator_TS1.hh"
#include "BeamlineGeom/inc/Collimator_TS3.hh"
#include "BeamlineGeom/inc/Collimator_TS5.hh"
#include "GeomPrimitives/inc/Torus.hh"
#include "GeomPrimitives/inc/Tube.hh"
#include "ConfigTools/inc/SimpleConfig.hh"

// CLHEP
#include "CLHEP/Units/SystemOfUnits.h"

#ifndef __CINT__

namespace mu2e {

  std::unique_ptr<Beamline> BeamlineMaker::make(const SimpleConfig& c) {
    std::unique_ptr<Beamline> res ( new Beamline() );
    BuildBeamline     (c, res.get() );
    BuildTSCryostat   (c, res.get() );
    BuildTSCoils      (c, res.get() );
    BuildTSCollimators(c, &res->_ts );
    BuildTSVacua      (c, &res->_ts );
    BuildTSPolyLining (c, &res->_ts );
    BuildPbarWindow   (c, &res->_ts );
    return res;
  }

  void BeamlineMaker::BuildBeamline(const SimpleConfig& c, Beamline* bl) {
    bl->_solenoidOffset = c.getDouble("mu2e.solenoidOffset");
  }

  void BeamlineMaker::BuildTSCryostat(const SimpleConfig& c, Beamline* bl ){

    TransportSolenoid & ts = bl->_ts;
    ts._rTorus = c.getDouble("ts.rTorus",0.);
    ts._rVac   = c.getDouble("ts.rVac",0.); 
    ts._material = c.getString("ts.materialName");
    ts._insideMaterial = c.getString("ts.insideMaterialName");

    // - end wall parameters
    ts._rIn_endWallU1 = c.getDouble("ts.tsUendWall1.rIn",c.getDouble("ts.ts1in.rOut") ); 
    ts._rIn_endWallU2 = c.getDouble("ts.tsUendWall2.rIn",0.); 
    ts._rIn_endWallD  = c.getDouble("ts.tsDendWall.rIn", c.getDouble("ts.ts5in.rOut") );  

    ts._rOut_endWallU1 = c.getDouble("ts.tsUendWall1.rOut",c.getDouble("ts.ts1out.rIn") ); 
    ts._rOut_endWallU2 = c.getDouble("ts.tsUendWall2.rOut",0.); 
    ts._rOut_endWallD  = c.getDouble("ts.tsDendWall.rOut", c.getDouble("ts.ts5out.rIn") );  

    ts._halfLength_endWallU1 = c.getDouble("ts.tsUendWall1.halfLength"); 
    ts._halfLength_endWallU2 = c.getDouble("ts.tsUendWall2.halfLength"); 
    ts._halfLength_endWallD  = c.getDouble("ts.tsDendWall.halfLength" );  

    double ts1HalfLength = c.getDouble("ts.ts1.halfLength");
    double ts3HalfLength = bl->solenoidOffset() - ts.torusRadius();
    double ts5HalfLength = c.getDouble("ts.ts5.halfLength");

    double ts1zOffset    = (-ts.torusRadius()-ts1HalfLength );
    double ts5zOffset    = ( ts.torusRadius()+ts5HalfLength );

    // Typedefs for easier use
    typedef TransportSolenoid::TSRegion::enum_type     tsReg_enum;
    typedef TransportSolenoid::TSRadialPart::enum_type tsRad_enum;

    // Bookkeeping index 
    // - note that this index mapping is unique as straight section
    // and torus section numbering are always incremented by 2, not 1
    auto MapIndex = [](tsReg_enum iTS, tsRad_enum iRAD) -> unsigned {
      return static_cast<unsigned>(iTS) + static_cast<unsigned>(iRAD); 
    };

    // Cryo map parameters - straight sections
   
    const std::map<unsigned,Tube> straightSectionParams = {
      // Inner straight sections
      { MapIndex(tsReg_enum::TS1,tsRad_enum::IN ), Tube(ts.innerRadius(),
                                                        c.getDouble("ts.ts1in.rOut"),
                                                        ts1HalfLength,
                                                        CLHEP::Hep3Vector( bl->solenoidOffset(),0.0,ts1zOffset) ) },                                       
      { MapIndex(tsReg_enum::TS3,tsRad_enum::IN ), Tube(ts.innerRadius(),
                                                        c.getDouble("ts.ts3in.rOut"),
                                                        ts3HalfLength,
                                                        CLHEP::Hep3Vector(),
                                                        CLHEP::HepRotation(CLHEP::HepRotationY(90.0*CLHEP::degree)) ) },
      { MapIndex(tsReg_enum::TS5,tsRad_enum::IN ), Tube(ts.innerRadius(),
                                                        c.getDouble("ts.ts5in.rOut"),
                                                        ts5HalfLength,
                                                        CLHEP::Hep3Vector(-bl->solenoidOffset(),0.0,ts5zOffset) ) },
      // Outer straight sections
      { MapIndex(tsReg_enum::TS1,tsRad_enum::OUT), Tube(c.getDouble("ts.ts1out.rIn"),                                                   
                                                        c.getDouble("ts.ts1out.rOut"),                                                  
                                                        ts1HalfLength - ts.endWallU2_halfLength(),             
                                                        CLHEP::Hep3Vector( bl->solenoidOffset(),0.0,ts1zOffset-ts.endWallU2_halfLength() ) ) },
      { MapIndex(tsReg_enum::TS3,tsRad_enum::OUT), Tube(c.getDouble("ts.ts3out.rIn"),                            
                                                        c.getDouble("ts.ts3out.rOut"),                           
                                                        ts3HalfLength,                                              
                                                        CLHEP::Hep3Vector(),                                        
                                                        CLHEP::HepRotation(CLHEP::HepRotationY(90.0*CLHEP::degree)) ) },
      { MapIndex(tsReg_enum::TS5,tsRad_enum::OUT), Tube(c.getDouble("ts.ts5out.rIn"),                        
                                                        c.getDouble("ts.ts5out.rOut"),                       
                                                        ts5HalfLength,               
                                                        CLHEP::Hep3Vector(-bl->solenoidOffset(),0.0,ts5zOffset) ) },
    };


    // Cryo map parameters - torus sections

    const std::map<unsigned,Torus> torusSectionParams = {
      // Inner torus sections
      { MapIndex(tsReg_enum::TS2,tsRad_enum::IN ), Torus( ts.torusRadius(),                                                         
                                                          ts.innerRadius(),                                                         
                                                          c.getDouble("ts.ts2in.rOut"),                                          
                                                          1.5*CLHEP::pi, CLHEP::halfpi,                                             
                                                          CLHEP::Hep3Vector( ts3HalfLength, 0.,-ts.torusRadius() ),
                                                          CLHEP::HepRotation(CLHEP::HepRotationX(90.0*CLHEP::degree))               ) },     
      { MapIndex(tsReg_enum::TS4,tsRad_enum::IN ), Torus( ts.torusRadius(),                                                         
                                                          ts.innerRadius(),                                                         
                                                          c.getDouble("ts.ts4in.rOut"),                                          
                                                          CLHEP::halfpi, CLHEP::halfpi,                                             
                                                          CLHEP::Hep3Vector( -ts3HalfLength, 0.,ts.torusRadius() ),
                                                          CLHEP::HepRotation(CLHEP::HepRotationX(90.0*CLHEP::degree))               ) },
      // Outer torus sections
      { MapIndex(tsReg_enum::TS2,tsRad_enum::OUT), Torus( ts.torusRadius(),                                                         
                                                          c.getDouble("ts.ts2out.rIn"),                                          
                                                          c.getDouble("ts.ts2out.rOut"),                                         
                                                          1.5*CLHEP::pi, CLHEP::halfpi,                                             
                                                          CLHEP::Hep3Vector( ts3HalfLength, 0.,-ts.torusRadius() ),
                                                          CLHEP::HepRotation(CLHEP::HepRotationX(90.0*CLHEP::degree))               ) },
      { MapIndex(tsReg_enum::TS4,tsRad_enum::OUT), Torus( ts.torusRadius(),                                                         
                                                          c.getDouble("ts.ts4out.rIn"),                                          
                                                          c.getDouble("ts.ts4out.rOut"),                                         
                                                          CLHEP::halfpi, CLHEP::halfpi,                                             
                                                          CLHEP::Hep3Vector( -ts3HalfLength, 0.,ts.torusRadius() ),
                                                          CLHEP::HepRotation(CLHEP::HepRotationX(90.0*CLHEP::degree))               ) },
    };

    // Set cryo map - straight sections
    for ( unsigned iTS = tsReg_enum::TS1 ; iTS <= tsReg_enum::TS5 ; iTS=iTS+2 ) {
      auto its    = (tsReg_enum)iTS;

      for ( unsigned iRAD = tsRad_enum::IN ; iRAD <= tsRad_enum::OUT ; iRAD++ ) {
        auto irad = (tsRad_enum)iRAD;     
        auto straightParam = straightSectionParams.find( MapIndex(its,irad) );

        ts._cryoMap[its][irad] = std::unique_ptr<TSSection>( new StraightSection ( straightParam->second ) );

      } 
    } 

    // Set cryo map - torus sections
    for ( unsigned iTS = tsReg_enum::TS2 ; iTS <= tsReg_enum::TS4 ; iTS=iTS+2 ) {
      auto its = (tsReg_enum)iTS;

      for ( unsigned iRAD = tsRad_enum::IN ; iRAD <= tsRad_enum::OUT ; iRAD++ ) {
        auto irad = (tsRad_enum)iRAD;     
        auto torusParam = torusSectionParams.find( MapIndex(its,irad ) );

        ts._cryoMap[its][irad] = std::unique_ptr<TSSection>( new TorusSection ( torusParam->second ) );

      }
    } 

  }

  void BeamlineMaker::BuildTSCoils (const SimpleConfig& c, Beamline* bl ) {

    TransportSolenoid * ts = &bl->_ts;

    ts->_coilMaterial = c.getString("ts.coils.material");

    // Loop over TS regions
    for ( unsigned iTS = TransportSolenoid::TSRegion::TS1 ; iTS <= TransportSolenoid::TSRegion::TS5 ; iTS++ )
      { 
        auto its = (TransportSolenoid::TSRegion)iTS;

        std::vector<double> tmp_rIn, tmp_rOut, tmp_sLength, tmp_xPos, tmp_zPos, tmp_yRotAngle;

        std::ostringstream prefix; 
        prefix << "ts" << iTS;
        c.getVectorDouble( prefix.str()+".coils.rIn"      , tmp_rIn       , ts->getNCoils(its) );
        c.getVectorDouble( prefix.str()+".coils.rOut"     , tmp_rOut      , ts->getNCoils(its) );
        c.getVectorDouble( prefix.str()+".coils.sLength"  , tmp_sLength   , ts->getNCoils(its) );
        c.getVectorDouble( prefix.str()+".coils.xPos"     , tmp_xPos      , ts->getNCoils(its) );
        c.getVectorDouble( prefix.str()+".coils.zPos"     , tmp_zPos      , ts->getNCoils(its) );
        c.getVectorDouble( prefix.str()+".coils.yRotAngle", tmp_yRotAngle , ts->getNCoils(its) );

        // Loop over coils per TS region
        ts->_coilMap[its].reserve( ts->getNCoils( its ) );

        for ( unsigned i(0) ; i < ts->getNCoils( its ) ; i++ ) {
          ts->_coilMap[its].emplace_back( tmp_xPos.at(i),  // position
                                          ts->getTSCryo(its,TransportSolenoid::TSRadialPart::IN)->getGlobal().y(), 
                                          tmp_zPos.at(i),
                                          tmp_rIn.at(i),   // tube parameters
                                          tmp_rOut.at(i),
                                          tmp_sLength.at(i)*0.5,
                                          CLHEP::HepRotation(CLHEP::HepRotationY(-tmp_yRotAngle.at(i)*CLHEP::degree) ) );
        }

      }

  }

  void BeamlineMaker::BuildTSCollimators (const SimpleConfig& c, TransportSolenoid* ts) {

    // Set collimators
    double coll1HalfLength = c.getDouble("ts.coll1.halfLength");
    double coll3HalfLength = c.getDouble("ts.coll3.halfLength");
    double coll5HalfLength = c.getDouble("ts.coll5.halfLength");
    double coll3Hole       = c.getDouble("ts.coll3.hole");

    CollimatorTS1 & coll1  = ts->_coll1 ;
    CollimatorTS3 & coll31 = ts->_coll31;
    CollimatorTS3 & coll32 = ts->_coll32;
    CollimatorTS5 & coll5  = ts->_coll5 ;

    coll1 .set(coll1HalfLength,CLHEP::Hep3Vector(0.,0.,c.getDouble("ts.coll1.sOffset" ,0.)));
    coll31.set(coll3HalfLength,CLHEP::Hep3Vector(0.,0.,c.getDouble("ts.coll31.sOffset",0.)-coll3HalfLength-coll3Hole/2));
    coll32.set(coll3HalfLength,CLHEP::Hep3Vector(0.,0.,c.getDouble("ts.coll32.sOffset",0.)+coll3HalfLength+coll3Hole/2));
    coll5 .set(coll5HalfLength,CLHEP::Hep3Vector(0.,0.,c.getDouble("ts.coll5.sOffset" ,0.)));

    // TS1
    coll1._rIn1      = c.getDouble("ts.coll1.innerRadius1",0.);
    coll1._rIn2      = c.getDouble("ts.coll1.innerRadius2",0.);
    coll1._rIn3      = c.getDouble("ts.coll1.innerRadius3",0.);
    coll1._material1 = c.getString("ts.coll1.material1Name");
    coll1._material2 = c.getString("ts.coll1.material2Name");

    // TS3
    coll32._hole             = coll31._hole              = c.getDouble("ts.coll3.hole",0.);
    coll32._holeRadius       = coll31._holeRadius        = c.getDouble("ts.coll3.holeRadius",0.);
    coll32._holeHalfHeight   = coll31._holeHalfHeight    = c.getDouble("ts.coll3.holeHalfHeight",0.);
    coll32._holeDisplacement = coll31._holeDisplacement  = c.getDouble("ts.coll3.holeDisplacement",0.);
    coll32._rotationAngle    = coll31._rotationAngle     = c.getDouble("ts.coll3.rotationAngle",0.);
    coll32._material         = coll31._material          = c.getString("ts.coll3.materialName");

    // TS5
    coll5._rIn         = c.getDouble("ts.coll5.innerRadius");
    coll5._rOut        = c.getDouble("ts.coll5.outerRadius");
    coll5._material    = c.getString("ts.coll5.materialName");

  }

  void BeamlineMaker::BuildTSVacua(const SimpleConfig& c, TransportSolenoid* ts ) {

    typedef TransportSolenoid::TSRegion::enum_type     tsReg_enum;
    typedef TransportSolenoid::TSRadialPart::enum_type tsRad_enum;

    const StraightSection * ts1 = ts->getTSCryo<StraightSection>(tsReg_enum::TS1,tsRad_enum::IN);
    const StraightSection * ts3 = ts->getTSCryo<StraightSection>(tsReg_enum::TS3,tsRad_enum::IN);
    const StraightSection * ts5 = ts->getTSCryo<StraightSection>(tsReg_enum::TS5,tsRad_enum::IN);

    // Figure things out for TS1, which is special
    CLHEP::Hep3Vector ts1VacPos = ts1->getGlobal();
    const double zLocCryoEdge   = ts1->getGlobal().z()-ts1->getHalfLength();
    const double zLocCollEdge   = ts->getColl1().getLocal().z()+ts1->getGlobal().z()-ts->getColl1().halfLength();
    const double zVacExtension  = std::abs( zLocCollEdge - zLocCryoEdge );

    const CLHEP::Hep3Vector vacOffset( 0., 0., -zVacExtension*0.5 );
    ts1VacPos = ts1VacPos + vacOffset;
    
    // Adjust offset of coll1 to be wrt TS1Vacuum (eventual parent
    // volume, not wrt TS1 cryostat)
    ts->_coll1.adjustOffset( -vacOffset );

    // Vacuum map parameters - straight sections
    const std::map<unsigned,Tube> straightSectionParams = {
      { tsReg_enum::TS1, Tube(0., ts->innerRadius(), ts1->getHalfLength()+zVacExtension*0.5, ts1VacPos ) },
      { tsReg_enum::TS3, Tube(0., ts->innerRadius(), ts3->getHalfLength(), ts3->getGlobal(), CLHEP::HepRotation(CLHEP::HepRotationY(90.0*CLHEP::degree)) ) },
      { tsReg_enum::TS5, Tube(0., ts->innerRadius(), ts5->getHalfLength(), ts5->getGlobal() ) }
     };

    // Vacuum map parameters - torus sections
    const std::map<unsigned,Torus> torusSectionParams = {
      { tsReg_enum::TS2, Torus( ts->torusRadius(), 0., ts->innerRadius(),                                                         
                                1.5*CLHEP::pi, CLHEP::halfpi,                                             
                                CLHEP::Hep3Vector( ts3->getHalfLength(), 0.,-ts->torusRadius() ),
                                CLHEP::HepRotation(CLHEP::HepRotationX(90.0*CLHEP::degree))               ) },     
      { tsReg_enum::TS4, Torus( ts->torusRadius(), 0., ts->innerRadius(),                                                         
                                CLHEP::halfpi, CLHEP::halfpi,                                             
                                CLHEP::Hep3Vector( -ts3->getHalfLength(), 0.,ts->torusRadius() ),
                                CLHEP::HepRotation(CLHEP::HepRotationX(90.0*CLHEP::degree))               ) }
    }; 

    // Set vacuum map - straight sections
    for ( unsigned iTS = tsReg_enum::TS1 ; iTS <= tsReg_enum::TS5 ; iTS=iTS+2 ) {
      auto its    = (tsReg_enum)iTS;
      auto straightParam = straightSectionParams.find( its );
      ts->_vacuumMap[its] = std::unique_ptr<TSSection>( new StraightSection ( straightParam->second ) );
    } 
    
    // Set cryo map - torus sections
    for ( unsigned iTS = tsReg_enum::TS2 ; iTS <= tsReg_enum::TS4 ; iTS=iTS+2 ) {
      auto its = (tsReg_enum)iTS;
      auto torusParam    = torusSectionParams.find( its );
      ts->_vacuumMap[its] = std::unique_ptr<TSSection>( new TorusSection ( torusParam->second ) );
    } 
    
  }
  
    void BeamlineMaker::BuildTSPolyLining(const SimpleConfig& c, TransportSolenoid* ts ) {

    typedef TransportSolenoid::TSRegion::enum_type     tsReg_enum;
    typedef TransportSolenoid::TSRadialPart::enum_type tsRad_enum;

    TorusSection    const * ts2 = ts->getTSCryo<TorusSection>( tsReg_enum::TS2, tsRad_enum::IN );
    TorusSection    const * ts4 = ts->getTSCryo<TorusSection>( tsReg_enum::TS4, tsRad_enum::IN );

    // Vacuum map parameters - torus sections
    const std::map<unsigned,Torus> torusSectionParams = {
      { tsReg_enum::TS2, Torus( ts->torusRadius(), 
                                c.getDouble("ts.polyliner.rIn",0.), 
                                c.getDouble("ts.polyliner.rOut",ts->innerRadius()),
                                ts2->phiStart()+5*CLHEP::degree,
                                ts2->deltaPhi()-2*5*CLHEP::degree,
                                ts2->getGlobal() ) },
      //                                *ts2->getRotation() ) },
      { tsReg_enum::TS4, Torus( ts->torusRadius(), 
                                c.getDouble("ts.polyliner.rIn",0.), 
                                c.getDouble("ts.polyliner.rOut",ts->innerRadius()),
                                ts4->phiStart()+5*CLHEP::degree,
                                ts4->deltaPhi()-2*5*CLHEP::degree,
                                ts4->getGlobal() ) }
      //                                *ts4->getRotation() ) }
    }; 

    // Set cryo map - torus sections
    for ( unsigned iTS = tsReg_enum::TS2 ; iTS <= tsReg_enum::TS4 ; iTS=iTS+2 ) {
      auto its = (tsReg_enum)iTS;
      auto torusParam    = torusSectionParams.find( its );
      ts->_polyLiningMap[its] = std::unique_ptr<TorusSection>( new TorusSection ( torusParam->second ) );
      ts->_polyLiningMap[its]->setMaterial( c.getString("ts.polyliner.materialName") );
    } 
    
  }

  void BeamlineMaker::BuildPbarWindow(const SimpleConfig& c, TransportSolenoid* ts){

    PbarWindow & pbarWindow ( ts->_pbarWindow );

    pbarWindow._shape    = c.getString("pbar.Type","disk");
    pbarWindow._material = c.getString("pbar.materialName");
    pbarWindow.set( c.getDouble("pbar.halfLength"),
                    CLHEP::Hep3Vector() );
    pbarWindow._rOut     = ts->innerRadius();

    // Parameters for wedge
    pbarWindow._y0       = c.getDouble("pbarwedge.y0",0.);
    pbarWindow._y1       = c.getDouble("pbarwedge.y1",0.);
    pbarWindow._dz0      = c.getDouble("pbarwedge.dz0",0.);
    pbarWindow._dz1      = c.getDouble("pbarwedge.dz1",0.);

  }

} // namespace mu2e

#endif
