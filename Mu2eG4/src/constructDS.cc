//
// Free function to create DS. (Detector Solenoid)
//
// $Id: constructDS.cc,v 1.21 2014/01/09 18:15:21 ejbarnes Exp $
// $Author: ejbarnes $
// $Date: 2014/01/09 18:15:21 $
//
// Original author KLG based on Mu2eWorld constructDS
//

// Mu2e includes.
#include "BeamlineGeom/inc/Beamline.hh"
#include "BeamlineGeom/inc/StraightSection.hh"
#include "DetectorSolenoidGeom/inc/DetectorSolenoid.hh"
#include "DetectorSolenoidGeom/inc/DetectorSolenoidShielding.hh"
#include "G4Helper/inc/G4Helper.hh"
#include "G4Helper/inc/VolumeInfo.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "GeomPrimitives/inc/PolyconsParams.hh"
#include "MBSGeom/inc/MBS.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Mu2eG4/inc/constructDS.hh"
#include "Mu2eG4/inc/nestTubs.hh"
#include "Mu2eG4/inc/nestPolycone.hh"
#include "Mu2eG4/inc/finishNesting.hh"
#include "Mu2eG4/inc/MaterialFinder.hh"

// G4 includes
#include "G4ThreeVector.hh"
#include "G4Material.hh"
#include "G4Color.hh"
#include "G4Polycone.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"

// CLHEP includes
#include "CLHEP/Vector/ThreeVector.h"

using namespace std;

namespace mu2e {

  void constructDS( const VolumeInfo& parent,
                    SimpleConfig const & _config
                    ){
    MaterialFinder materialFinder(_config);
    // Flags
    int const verbosityLevel = _config.getInt("ds.verbosityLevel",0);
    bool dsVisible           = _config.getBool("ds.visible",true);
    bool dsSolid             = _config.getBool("ds.solid",true);
    bool dsCoilVisible       = _config.getBool("dsCoil.visible",true);
    bool dsCoilSolid         = _config.getBool("dsCoil.solid",true);
    bool dsSupportVisible    = _config.getBool("dsSupport.visible",true);
    bool dsSupportSolid      = _config.getBool("dsSupport.solid",true);
    bool dsThShieldVisible   = _config.getBool("dsThShield.visible",true);
    bool dsThShieldSolid     = _config.getBool("dsThShield.solid",true);
    bool dsVacuumVisible     = _config.getBool("dsVacuum.visible",true);
    bool dsVacuumSolid       = _config.getBool("dsVacuum.solid",true);
    bool dsShieldVisible     = _config.getBool("dsShielding.visible",true);
    bool dsShieldSolid       = _config.getBool("dsShielding.solid",true);
    bool forceAuxEdgeVisible = _config.getBool("g4.forceAuxEdgeVisible",false);
    bool doSurfaceCheck      = _config.getBool("g4.doSurfaceCheck",false);
    bool const placePV       = true;

    // Fetch parent (hall) position
    G4ThreeVector _hallOriginInMu2e = parent.centerInMu2e();

    // Fetch DS geom. object
    GeomHandle<DetectorSolenoid> ds;
    CLHEP::Hep3Vector const & dsP ( ds->position() );

    // DS cryostat
    G4Material*   dsCryoMaterial = findMaterialOrThrow( ds->material() );

    // - inner cryo shell
    G4ThreeVector dsInnerCryoPosition( dsP.x(), dsP.y(), dsP.z());
    TubsParams    dsInnerCryoParams  ( ds->rIn1(), ds->rIn2(), ds->halfLength() );
    nestTubs( "DSInnerCryoShell",
              dsInnerCryoParams,
              dsCryoMaterial,
              0,
              dsInnerCryoPosition-_hallOriginInMu2e,
              parent,
              0,
              dsVisible,
              G4Color::Magenta(),
              dsSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    // - outer cryo shell
    G4ThreeVector dsOuterCryoPosition( dsP.x(), dsP.y(), dsP.z());
    TubsParams    dsOuterCryoParams  ( ds->rOut1(), ds->rOut2(), ds->halfLength() );
    nestTubs( "DSOuterCryoShell",
              dsOuterCryoParams,
              dsCryoMaterial,
              0,
              dsOuterCryoPosition-_hallOriginInMu2e,
              parent,
              0,
              dsVisible,
              G4Color::Magenta(),
              dsSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    // - end walls
    TubsParams    dsEndWallParams    ( ds->rIn2(), ds->rOut1(), ds->endWallHalfLength() );
    G4ThreeVector dsUpEndWallPosition( dsP.x(), dsP.y(), 
                                       dsP.z() - ds->halfLength() + ds->endWallHalfLength());
    nestTubs( "DSUpEndWallShell",
              dsEndWallParams,
              dsCryoMaterial,
              0,
              dsUpEndWallPosition-_hallOriginInMu2e,
              parent,
              0,
              dsVisible,
              G4Color::Magenta(),
              dsSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    G4ThreeVector dsDownEndWallPosition( dsP.x(), dsP.y(), 
                                         dsP.z() + ds->halfLength() - ds->endWallHalfLength());
    nestTubs( "DSDownEndWallShell",
              dsEndWallParams,
              dsCryoMaterial,
              0,
              dsDownEndWallPosition-_hallOriginInMu2e,
              parent,
              0,
              dsVisible,
              G4Color::Magenta(),
              dsSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    // - upstream face
    GeomHandle<Beamline> beamg;
    const StraightSection * ts5out = beamg->getTS().getTSCryo<StraightSection>( TransportSolenoid::TSRegion::TS5,
                                                                                TransportSolenoid::TSRadialPart::OUT );

    double dsFrontZ0 = dsP.z() - ds->halfLength() + ds->frontHalfLength();
    TubsParams    dsFrontParams  ( ts5out->rOut(), dsInnerCryoParams.innerRadius(), ds->frontHalfLength() );
    G4ThreeVector dsFrontPosition( dsP.x(), dsP.y(), dsFrontZ0);

    nestTubs( "DSFront",
              dsFrontParams,
              dsCryoMaterial,
              0,
              dsFrontPosition-_hallOriginInMu2e,
              parent,
              0,
              dsVisible,
              G4Color::Blue(),
              dsSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    // DS thermal shield
    G4Material*   dsThShieldMaterial = findMaterialOrThrow( ds->shield_material() );

    // - inner shield shell
    G4ThreeVector dsInnerThShieldPosition( dsP.x(), dsP.y(), dsP.z());
    TubsParams    dsInnerThShieldParams  ( ds->shield_rIn1(), ds->shield_rIn2(), ds->shield_halfLength() );
    nestTubs( "DSInnerThShieldShell",
              dsInnerThShieldParams,
              dsThShieldMaterial,
              0,
              dsInnerThShieldPosition-_hallOriginInMu2e,
              parent,
              0,
              dsThShieldVisible,
              G4Color::Cyan(),
              dsThShieldSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    // - outer shield shell
    G4ThreeVector dsOuterThShieldPosition( dsP.x(), dsP.y(), dsP.z());
    TubsParams    dsOuterThShieldParams  ( ds->shield_rOut1(), ds->shield_rOut2(), ds->shield_halfLength() );
    nestTubs( "DSOuterThShieldShell",
              dsOuterThShieldParams,
              dsThShieldMaterial,
              0,
              dsOuterThShieldPosition-_hallOriginInMu2e,
              parent,
              0,
              dsThShieldVisible,
              G4Color::Cyan(),
              dsThShieldSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    // SKIPPING END WALLS OF THERMAL SHIELD
    // DUE TO INCONSISTENCY IN DRAWINGS!

    // DS coils
    G4Material* dsCoilMaterial = findMaterialOrThrow( ds->coil_material() );
    
    for ( int i(0); i < ds->nCoils() ; i++ ) {
      TubsParams coilParams( ds->coil_rIn(), 
                             ds->coil_rOut().at(i),
                             ds->coil_zLength().at(i)*0.5 );
      G4ThreeVector coilPosition( dsP.x(), dsP.y(), 
                                  ds->coil_zPosition().at(i) + coilParams.zHalfLength()); 

      ostringstream coilname;
      coilname << "DSCoil_" << i+1;

      nestTubs( coilname.str(),
                coilParams,
                dsCoilMaterial,
                0,
                coilPosition-_hallOriginInMu2e,
                parent,
                0,
                dsCoilVisible,
                G4Color::Green(),
                dsCoilSolid,
                forceAuxEdgeVisible,
                placePV,
                doSurfaceCheck
                );
    }

    // DS coils support system
    G4Material*   dsSupportMaterial = findMaterialOrThrow( ds->support_material() );
    G4ThreeVector dsSupportPosition( dsP.x(), dsP.y(), dsP.z());
    TubsParams    dsSupportParams  ( ds->support_rIn(), ds->support_rOut(), ds->support_halfLength() );
    nestTubs( "DSCoilSupport",
              dsSupportParams,
              dsSupportMaterial,
              0,
              dsSupportPosition-_hallOriginInMu2e,
              parent,
              0,
              dsSupportVisible,
              G4Color::Blue(),
              dsSupportSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    // DS vacuum volumes
    G4Material* vacuumMaterial = findMaterialOrThrow( ds->vacuumMaterial() );
    TubsParams ds1VacParams    ( ts5out->rOut(), ds->rIn1(), ds->vac_halfLengthDs1()   );
    TubsParams ds2VacParams    ( 0.            , ds->rIn1(), ds->vac_halfLengthDs2()   );

    // Compute/set positions of vacuum volumes in Mu2e coordinates.
    // - DS position is fixed by TS torus radius, and half lengths of 
    //   front face, DS1, and TS5
    double ds1Z0     = dsFrontZ0 + ds->frontHalfLength() + ds->vac_halfLengthDs1();
    double ds2Z0     = ds->vac_zLocDs23Split() - ds->vac_halfLengthDs2();
    double ds2HalfLength     = _config.getDouble("ds2.halfLength");
    
    bool piondegVisible = _config.getBool("piondeg.visible",true);
    bool piondegSolid   = _config.getBool("piondeg.solid",true);
 
    

    if ( verbosityLevel > 0 ) {
      cout << __func__ << " DS2 vacuum extent: " 
           << " [ " << ds2Z0 - ds->vac_halfLengthDs2() << " , " 
           << ds->vac_zLocDs23Split() << " ] " << endl;
    }

    G4ThreeVector ds1Position( dsP.x(), dsP.y(), ds1Z0 );
    G4ThreeVector ds2Position( dsP.x(), dsP.y(), ds2Z0 );

    nestTubs( "DS1Vacuum",
              ds1VacParams,
              vacuumMaterial,
              0,
              ds1Position-_hallOriginInMu2e,
              parent,
              0,
              dsVacuumVisible,
              G4Colour::Green(),
              dsVacuumSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    VolumeInfo ds2VacInfo = 
      nestTubs( "DS2Vacuum",
              ds2VacParams,
              vacuumMaterial,
              0,
              ds2Position-_hallOriginInMu2e,
              parent,
              0,
              dsVacuumVisible,
              G4Colour::Yellow(),
              dsVacuumSolid,
              forceAuxEdgeVisible,
              placePV,
              doSurfaceCheck
              );

    // Polycone geometry allows for MBS to extend beyond solenoid
    // physical boundaries
    
    GeomHandle<DetectorSolenoidShielding> dss;

    // Define polycone parameters
    vector<double> tmp_zPlanesDs3 = { 
      ds->vac_zLocDs23Split(), 
      dss->getVPSPmain()->zBegin(),
      dss->getVPSPCryoSeal()->zBegin(),
      dss->getVPSPCryoSeal()->zEnd(),
      dss->getVPSPCryoSeal()->zEnd(),
      dss->getVPSPendSeal()->zBegin(), 
      dss->getVPSPendSeal()->zBegin(),
      dss->getVPSPendFlange()->zEnd(),
      dss->getVPSPendFlange()->zEnd(),
      dss->getIFBmain()->zEnd(),
      dss->getIFBendPlug()->zBegin(),
      dss->getIFBendPlug()->zEnd() };

    vector<double> tmp_rOuterDs3  = {
      ds->rIn1(),
      dss->getVPSPmain()->innerRadius(),
      dss->getVPSPCryoSeal()->outerRadius(),
      dss->getVPSPCryoSeal()->outerRadius(),
      dss->getVPSPmain()->outerRadius(),
      dss->getVPSPmain()->outerRadius(),
      dss->getVPSPendSeal()->outerRadius(),
      dss->getVPSPendFlange()->outerRadius(),
      dss->getVPSPendFlange()->innerRadius(),
      dss->getIFBmain()->outerRadius(),
      dss->getIFBendPlug()->outerRadius(),
      dss->getIFBendPlug()->outerRadius()
    };

    assert( tmp_zPlanesDs3.size() == tmp_rOuterDs3.size() );

    vector<double> tmp_rInnerDs3 ( tmp_rOuterDs3.size(), 0. );

    PolyconsParams    ds3PolyParams    ( tmp_zPlanesDs3, tmp_rInnerDs3, tmp_rOuterDs3 );
    CLHEP::Hep3Vector ds3positionInMu2e( dsP.x(), dsP.y(), 0.);
    
    VolumeInfo dsShieldParent = nestPolycone( "DS3Vacuum",
                                            ds3PolyParams,
                                            vacuumMaterial,
                                            0,
                                            ds3positionInMu2e - parent.centerInMu2e(),
                                            parent,
                                            0,
                                            dsVacuumVisible,
                                            G4Colour::Yellow(),
                                            dsVacuumSolid,
                                            forceAuxEdgeVisible,
                                            placePV,
                                            doSurfaceCheck
                                            );

    // Construct shielding downstream of DS
    for ( const auto & shield : dss->getTubes() ) {

      nestTubs( shield->getName(),
                shield->getTubsParams(),
                findMaterialOrThrow(shield->materialName()),
                0,
                shield->originInMu2e()-dsShieldParent.centerInMu2e(),
                dsShieldParent,
                0,
                dsShieldVisible,
                G4Colour::Blue(),
                dsShieldSolid,
                forceAuxEdgeVisible,
                placePV,
                doSurfaceCheck
                );

    }
    
    bool addPionDegrader  = _config.getBool("piondegrader.build",false);
    double piondegXoffset        = _config.getDouble("piondeg.xoffset");
    double piondegZoffset        = _config.getDouble("piondeg.zoffset");
    double piondegHalfLength     = _config.getDouble("piondeg.halfLength");
    double piondegRadius = _config.getDouble("piondeg.radius");
    double piondegParams[5]  = { 0.0,  piondegRadius, piondegHalfLength, 0.0, CLHEP::twopi };

    if( addPionDegrader ) {
      std::cout<<" pion degrader position:\t"
	       <<" x offset: "<<piondegXoffset
	       <<" z position: "<<-ds2HalfLength + piondegZoffset + piondegHalfLength
	       <<" Pion degrader halflength:"<<piondegHalfLength
	       <<std::endl;



      G4Material* piondegMaterial  = materialFinder.get("piondeg.materialName");
      VolumeInfo piondegInfo = nestTubs( "PiondegAbs",
					 piondegParams,
					 piondegMaterial,
					 0,
					 G4ThreeVector(piondegXoffset,0.,-ds2HalfLength + piondegZoffset + piondegHalfLength),
					 ds2VacInfo,
					 0,
					 piondegVisible,
					 G4Color::Blue(),
					 piondegSolid,
					 forceAuxEdgeVisible,
					 placePV,
					 doSurfaceCheck
					 );
      
      if ( verbosityLevel > 0) {
      cout << __func__ << 
	" PiondegAbs Z offset in Mu2e       : " << -ds2HalfLength + piondegZoffset + piondegHalfLength << endl;
      cout << __func__ << 
	" piondegZoffset                    : " << piondegZoffset    << endl;
       cout << __func__ << 
	" piondegHalfLength                : " << piondegHalfLength << endl;
      }
	//std::cout<<"piondegInfo\t"<<piondegParams<<std::endl;

 }
  } // end of Mu2eWorld::constructDS;

}
