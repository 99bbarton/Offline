//
// Free function to create the virtual detectors
//
// $Id: constructVirtualDetectors.cc,v 1.3 2011/05/17 15:36:01 greenc Exp $
// $Author: greenc $
// $Date: 2011/05/17 15:36:01 $
//
// Original author KLG based on Mu2eWorld constructVirtualDetectors
//
// C++ includes

#include <iostream>

// Mu2e includes.

#include "Mu2eG4/inc/constructVirtualDetectors.hh"

#include "BeamlineGeom/inc/Beamline.hh"
#include "G4Helper/inc/G4Helper.hh"
#include "G4Helper/inc/VolumeInfo.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "Mu2eG4/inc/MaterialFinder.hh"
#include "Mu2eG4/inc/SensitiveDetectorName.hh"
#include "Mu2eG4/inc/VirtualDetectorSD.hh"
#include "Mu2eG4/inc/nestTubs.hh"
#include "VirtualDetectorGeom/inc/VirtualDetector.hh"

// G4 includes

#include "G4Material.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4Color.hh"


using namespace std;

namespace mu2e {

  // Construct the virtual detectors

  void constructVirtualDetectors( SimpleConfig const * const _config ){

    // Place virtual detectors

    bool vdVisible           = _config->get<bool>("vd.visible",true);
    bool vdSolid             = _config->get<bool>("vd.solid",true);
    bool forceAuxEdgeVisible = _config->get<bool>("g4.forceAuxEdgeVisible",false);
    bool doSurfaceCheck      = _config->get<bool>("g4.doSurfaceCheck",false);
    bool const placePV       = true;
    
    GeomHandle<VirtualDetector> vdg;
    if( vdg->nDet()<=0 ) return;

    GeomHandle<Beamline> beamg;
    double rVac         = CLHEP::mm * beamg->getTS().innerRadius();

    double vdHalfLength = CLHEP::mm * vdg->getHalfLength();

    MaterialFinder materialFinder(*_config);
    G4Material* vacuumMaterial     = materialFinder.get("toyDS.insideMaterialName");

    TubsParams vdParams(0,rVac,vdHalfLength);

    // Virtual Detectors 1 and 2 are placed inside TS1

    G4VSensitiveDetector* vdSD = G4SDManager::GetSDMpointer()->
      FindSensitiveDetector(SensitiveDetectorName::VirtualDetector());

    G4Helper* _helper = &(*(art::ServiceHandle<G4Helper>()));

    for( int id=1; id<=2; ++id) if( vdg->exist(id) ) {
      VolumeInfo const & parent = _helper->locateVolInfo("ToyTS1Vacuum");
      ostringstream name;
      name << "VirtualDetector" << id;
      VolumeInfo vd = nestTubs( name.str(), vdParams, vacuumMaterial, 0,
                                vdg->getLocal(id),
                                parent,
                                id, vdVisible, G4Color::Red(), vdSolid,
                                forceAuxEdgeVisible,
                                placePV,
                                doSurfaceCheck );
      vd.logical->SetSensitiveDetector(vdSD);
    }

    // Virtual Detectors 3-6 are placed inside TS3

    for( int id=3; id<=6; ++id) if( vdg->exist(id) ) {
      VolumeInfo const & parent = _helper->locateVolInfo("ToyTS3Vacuum");
      ostringstream name;
      name << "VirtualDetector" << id;
      VolumeInfo vd = nestTubs( name.str(), vdParams, vacuumMaterial, 0,
                                vdg->getLocal(id),
                                parent,
                                id, vdVisible, G4Color::Red(), vdSolid,
                                forceAuxEdgeVisible,
                                placePV,
                                doSurfaceCheck );
      vd.logical->SetSensitiveDetector(vdSD);
    }

    // Virtual Detectors 7-8 are placed inside TS5

    for( int id=7; id<=8; ++id) if( vdg->exist(id) ) {
      VolumeInfo const & parent = _helper->locateVolInfo("ToyTS5Vacuum");
      ostringstream name;
      name << "VirtualDetector" << id;
      VolumeInfo vd = nestTubs( name.str(), vdParams, vacuumMaterial, 0,
                                vdg->getLocal(id),
                                parent,
                                id, vdVisible, G4Color::Red(), vdSolid,
                                forceAuxEdgeVisible,
                                placePV,
                                doSurfaceCheck );
      vd.logical->SetSensitiveDetector(vdSD);
    }

    // Virtual Detectors 9-10 are placed inside DS2, just before and after stopping target

    // If there is no neutron absorber, virtual detectors 9 and 10 extend to
    // inner wall of DS2 minus 5 mm. If neutron absorber is define, these 
    // detectors extend to neutron absorber minus 5 mm.

    double Ravr = _config->getDouble("toyDS.rIn");
    double deltaR = 0;
    double Z0 = 0;
    double deltaZ = 1.0;

    if ( _config->get<bool>("hasNeutronAbsorber",false) ) {
      double NAIInnerRadius0     = _config->getDouble("neutronabsorber.internalInnerRadius0");
      double NAIInnerRadius1     = _config->getDouble("neutronabsorber.internalInnerRadius1");
      Ravr   = (NAIInnerRadius0+NAIInnerRadius1)/2;
      deltaR = (NAIInnerRadius1-NAIInnerRadius0);
      Z0     = _config->getDouble("neutronabsorber.internalZ01");
      deltaZ = 2.0 *_config->getDouble("neutronabsorber.internalHalfLengthZ01");
    }

    for( int id=9; id<=10; ++id) if( vdg->exist(id) ) {
      
      double zvd = vdg->getGlobal(id).z();
      double rvd = Ravr + deltaR/deltaZ*(zvd-Z0) - 5.0;

      cout << "VD " << id << " : " << zvd << " " << rvd << endl;

      TubsParams vdParamsTarget(0,rvd,vdHalfLength);

      VolumeInfo const & parent = _helper->locateVolInfo("ToyDS2Vacuum");
      ostringstream name;
      name << "VirtualDetector" << id;
      VolumeInfo vd = nestTubs( name.str(), vdParamsTarget, vacuumMaterial, 0,
                                vdg->getLocal(id),
                                parent,
                                id, vdVisible, G4Color::Red(), vdSolid,
                                forceAuxEdgeVisible,
                                placePV,
                                doSurfaceCheck );
      vd.logical->SetSensitiveDetector(vdSD);
    }

  }

}
