//
// Free function to create the virtual detectors
//
// $Id: constructVirtualDetectors.cc,v 1.66 2014/01/06 20:59:01 kutschke Exp $
// $Author: kutschke $
// $Date: 2014/01/06 20:59:01 $
//
// Original author KLG based on Mu2eWorld constructVirtualDetectors

// C++ includes
#include <iostream>
#include <string>

// Mu2e includes.
#include "Mu2eG4/inc/constructVirtualDetectors.hh"

#include "BeamlineGeom/inc/Beamline.hh"
#include "CalorimeterGeom/inc/VaneCalorimeter.hh"
#include "CalorimeterGeom/inc/DiskCalorimeter.hh"
#include "DetectorSolenoidGeom/inc/DetectorSolenoid.hh"
#include "ExternalNeutronShieldingGeom/inc/ExtNeutShieldCendBoxes.hh"
#include "ExtinctionMonitorUCIGeom/inc/ExtMonUCI.hh"
#include "G4Helper/inc/AntiLeakRegistry.hh"
#include "G4Helper/inc/G4Helper.hh"
#include "G4Helper/inc/VolumeInfo.hh"
#include "GeomPrimitives/inc/Tube.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/VirtualDetector.hh"
#include "MCDataProducts/inc/VirtualDetectorId.hh"
#include "MECOStyleProtonAbsorberGeom/inc/MECOStyleProtonAbsorber.hh"
#include "Mu2eBuildingGeom/inc/Mu2eBuilding.hh"
#include "Mu2eG4/inc/SensitiveDetectorName.hh"
#include "Mu2eG4/inc/checkForOverlaps.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Mu2eG4/inc/finishNesting.hh"
#include "Mu2eG4/inc/nestTubs.hh"
#include "Mu2eG4/inc/nestBox.hh"
#include "ProductionSolenoidGeom/inc/PSVacuum.hh"
#include "ProtonBeamDumpGeom/inc/ProtonBeamDump.hh"
#include "ProductionTargetGeom/inc/ProductionTarget.hh"
#include "TTrackerGeom/inc/TTracker.hh"

// G4 includes
#include "G4Material.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4Color.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4SubtractionSolid.hh"
#include "G4IntersectionSolid.hh"

using namespace std;

namespace mu2e {

  // Construct the virtual detectors

  void constructVirtualDetectors( SimpleConfig const & _config ){

    // Place virtual detectors

    int static const verbosityLevel = _config.getInt("vd.verbosityLevel",0);

    bool vdIsVisible         = _config.getBool("vd.visible",true);
    bool vdIsSolid           = _config.getBool("vd.solid",true);
    bool forceAuxEdgeVisible = _config.getBool("g4.forceAuxEdgeVisible",false);
    bool doSurfaceCheck      = _config.getBool("g4.doSurfaceCheck",false);
    bool const placePV       = true;

    AntiLeakRegistry& reg = art::ServiceHandle<G4Helper>()->antiLeakRegistry();
    GeomHandle<VirtualDetector> vdg;
    if( vdg->nDet()<=0 ) return;

    GeomHandle<Mu2eBuilding> building;

    GeomHandle<Beamline> beamg;
    double rVac         = CLHEP::mm * beamg->getTS().innerRadius();

    double vdHalfLength = CLHEP::mm * vdg->getHalfLength();

    GeomHandle<DetectorSolenoid> ds;
    G4Material* vacuumMaterial     = findMaterialOrThrow( ds->vacuumMaterial() );

    TubsParams vdParams(0,rVac,vdHalfLength);

    // Virtual Detectors Coll1_In, COll1_Out are placed inside TS1

    G4VSensitiveDetector* vdSD = G4SDManager::GetSDMpointer()->
      FindSensitiveDetector(SensitiveDetectorName::VirtualDetector());

    G4Helper* _helper = &(*(art::ServiceHandle<G4Helper>()));

    if(verbosityLevel>0) {
      VirtualDetectorId::printAll();
    }

    // FIXME: one should factorize some the code below; the main
    // things which change: parent and offset
    for( int vdId=VirtualDetectorId::Coll1_In;
         vdId<=VirtualDetectorId::Coll1_Out;
         ++vdId) if( vdg->exist(vdId) ) {
        VolumeInfo const & parent = _helper->locateVolInfo("TS1Vacuum");
        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)
               << " at " << vdg->getGlobal(vdId) << endl;
          cout << __func__ << "    VD parameters: " << vdParams << endl;
          cout << __func__ << "    VD rel. posit: " << vdg->getLocal(vdId) << endl;
        }

        VolumeInfo vd = nestTubs( VirtualDetector::volumeName(vdId),
                                  vdParams, vacuumMaterial, 0,
                                  vdg->getLocal(vdId),
                                  parent,
                                  vdId, vdIsVisible, G4Color::Red(), vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false);

        doSurfaceCheck && checkForOverlaps(vd.physical, _config, verbosityLevel>0);

        vd.logical->SetSensitiveDetector(vdSD);
      }

    // Virtual Detectors Coll31_In, Coll31_Out, Coll32_In, Coll32_Out are placed inside TS3

    for( int vdId=VirtualDetectorId::Coll31_In;
         vdId<=VirtualDetectorId::Coll32_Out;
         ++vdId) if( vdg->exist(vdId) ) {
        VolumeInfo const & parent = _helper->locateVolInfo("TS3Vacuum");
        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)
               << " at " << vdg->getGlobal(vdId) << endl;
        }
        VolumeInfo vd = nestTubs( VirtualDetector::volumeName(vdId),
                                  vdParams, vacuumMaterial, 0,
                                  vdg->getLocal(vdId),
                                  parent,
                                  vdId, vdIsVisible, G4Color::Red(), vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false);

        doSurfaceCheck && checkForOverlaps(vd.physical, _config, verbosityLevel>0);

        vd.logical->SetSensitiveDetector(vdSD);
      }

    // Virtual Detectors Coll5_In, Coll5_Out are placed inside TS5

    for( int vdId=VirtualDetectorId::Coll5_In;
         vdId<=VirtualDetectorId::Coll5_Out;
         ++vdId) if( vdg->exist(vdId) ) {
        VolumeInfo const & parent = _helper->locateVolInfo("TS5Vacuum");
        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)
               << " at " << vdg->getGlobal(vdId) <<  " parent: " << parent.centerInMu2e() << endl;
        }
        VolumeInfo vd = nestTubs( VirtualDetector::volumeName(vdId),
                                  vdParams, vacuumMaterial, 0,
                                  vdg->getLocal(vdId),
                                  parent,
                                  vdId, vdIsVisible, G4Color::Red(), vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false);

        doSurfaceCheck && checkForOverlaps(vd.physical, _config, verbosityLevel>0);

        vd.logical->SetSensitiveDetector(vdSD);
      }

    // Virtual Detectors Coll5_OutSurf surrounds the outer cylindrical surface of collimator in TS5

    int vdId = VirtualDetectorId::Coll5_OutSurf;
    if( vdg->exist(vdId) ) {

      if ( verbosityLevel > 0) {
        cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
      }

      // the detector is on the outer surface of the coll5
      // it is thin cylinder, NOT a thin disk

      VolumeInfo const & parent = _helper->locateVolInfo("TS5Vacuum");

      double coll5OuterRadius    = beamg->getTS().getColl5().rIn();
      double coll5HalfLength     = beamg->getTS().getColl5().halfLength();

      TubsParams  vdParamsColl5OutSurf(coll5OuterRadius - 2.*vdHalfLength,
                                       coll5OuterRadius,
                                       coll5HalfLength - 2.*vdHalfLength);

      VolumeInfo vd = nestTubs( VirtualDetector::volumeName(vdId),
                                vdParamsColl5OutSurf, vacuumMaterial, 0,
                                vdg->getLocal(vdId),
                                parent,
                                vdId, vdIsVisible, G4Color::Red(), vdIsSolid,
                                forceAuxEdgeVisible,
                                placePV,
                                false);

      doSurfaceCheck && checkForOverlaps(vd.physical, _config, verbosityLevel>0);

      vd.logical->SetSensitiveDetector(vdSD);

    }

    // Virtual Detectors ST_In, ST_Out are placed inside DS2, just before and after stopping target

    // If there is no neutron absorber, virtual detectors 9 and 10 extend to
    // inner wall of DS2 minus 5 mm.
    // Existence of internal neutron absorber(INA) and/or outer proton absorber(OPA) is checked.
    // Priority on radius determination goes to OPA, and next, INA, and finally DS2.
    // Final radius is the extention to OPA, INA or DS, minus 5 mm.

    if ( !_config.getBool("isDumbbell",false) ){
      double Ravr = ds->rIn1();

      if ( _config.getBool("hasInternalNeutronAbsorber",false) ) {
        Ravr = _config.getDouble("intneutronabs.rIn1");
      }

      bool opaflag = false;
      double opaz0, opaz1, opari0, opari1;
      if ( _config.getBool("hasProtonAbsorber", true) ) {
        GeomHandle<MECOStyleProtonAbsorber> pageom;
        if ( pageom->isAvailable(ProtonAbsorberId::opabs) ) {
          opaflag = true;
          MECOStyleProtonAbsorberPart opa = pageom->part(2);
          opaz0 = opa.center().z()-opa.halfLength();
          opaz1 = opa.center().z()+opa.halfLength();
          opari0 = opa.innerRadiusAtStart();
          opari1 = opa.innerRadiusAtEnd();
        }
      }

      for( int vdId=VirtualDetectorId::ST_In;
           vdId<=VirtualDetectorId::ST_Out;
           ++vdId) if( vdg->exist(vdId) ) {

          if ( verbosityLevel > 0) {
            cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
          }

          double zvd = vdg->getGlobal(vdId).z();
          if (opaflag) {
            Ravr = (opari1 - opari0)/(opaz1 - opaz0) * (zvd - opaz0) + opari0;
          }
          double rvd = Ravr - 5.0;

          if ( verbosityLevel > 0) {
            cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
              " z, r : " << zvd << ", " << rvd << endl;
          }

          TubsParams vdParamsTarget(0.,rvd,vdHalfLength);

          VolumeInfo const & parent = ( _config.getBool("isDumbbell",false) ) ?
            _helper->locateVolInfo("DS3Vacuum") :
            _helper->locateVolInfo("DS2Vacuum"); //DS3Vacuum to move the targets

          if (verbosityLevel >0) {
            cout << __func__ << " " << VirtualDetector::volumeName(vdId) << " Z offset in Mu2e    : " <<
              zvd << endl;
            cout << __func__ << " " << VirtualDetector::volumeName(vdId) << " Z extent in Mu2e    : " <<
              zvd - vdHalfLength << ", " << zvd + vdHalfLength << endl;
          }

          VolumeInfo vd = nestTubs( VirtualDetector::volumeName(vdId),
                                    vdParamsTarget, vacuumMaterial, 0,
                                    vdg->getLocal(vdId),
                                    parent,
                                    vdId,
                                    vdIsVisible,
                                    G4Color::Red(), vdIsSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    false);

          doSurfaceCheck && checkForOverlaps(vd.physical, _config, verbosityLevel>0);

          vd.logical->SetSensitiveDetector(vdSD);
        }
    }

    if ( _config.getBool("hasTTracker",false)  ) {


      // placing virtual detectors in the middle of the ttracker

      // check if ttracker exists and if the number of devices
      // ttracker.numDevices is even is done in VirtualDetectorMaker

      vdId = VirtualDetectorId::TT_Mid;
      if( vdg->exist(vdId) ) {

        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
        }

        // the radius of tracker mother
        TTracker const & ttracker = *(GeomHandle<TTracker>());
        double orvd = ttracker.mother().tubsParams().outerRadius();
        double irvd = ttracker.mother().tubsParams().innerRadius();

        if ( ttracker.getSupportModel() == SupportModel::detailedv0 ) {
          auto const& staves =  ttracker.getSupportStructure().staveBody();
          if ( staves.empty() ){
            throw cet::exception("GEOM")
              << "Cannot create virtual detector " << VirtualDetectorId(vdId).name()
              << " unless staves are defined\n";

          }
          orvd = staves.at(0).tubsParams().innerRadius();
        }


        if ( verbosityLevel > 0) {
          double zvd = vdg->getGlobal(vdId).z();
          cout << __func__  << " " << VirtualDetector::volumeName(vdId) <<
            " z, r : " << zvd << ", " << irvd << " " << orvd << endl;
        }

        TubsParams vdParamsTTracker(irvd,orvd,vdHalfLength);

        VolumeInfo const & parent = _helper->locateVolInfo("TrackerMother");

        CLHEP::Hep3Vector vdPos = vdg->getGlobal(vdId)-parent.centerInMu2e();
        //        cout << "foo: TT_Mid: " << vdPos << " " << vdg->getLocal(vdId) << endl;
        //        cout << "foo: TT_Mid: " << vdParamsTTracker << endl;

        VolumeInfo vd = nestTubs( VirtualDetector::volumeName(vdId),
                                  vdParamsTTracker, vacuumMaterial, 0,
                                  vdPos,
                                  parent,
                                  vdId, vdIsVisible, G4Color::Red(), vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false);

        doSurfaceCheck && checkForOverlaps(vd.physical, _config, verbosityLevel>0);

        vd.logical->SetSensitiveDetector(vdSD);

        vdId = VirtualDetectorId::TT_MidInner;
        if( vdg->exist(vdId) ) {

          if ( verbosityLevel > 0) {
            cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
          }

          // VD TT_MidInner is placed inside the ttracker at the same z position as
          // VD TT_Mid but from radius 0 to the inner radius of the ttracker
          // mother volume. However, its mother volume is DS3Vacuum
          // which has a different offset. We will use the global offset
          // here (!) as DS is not in the geometry service yet

          // we need to take into account the "overlap" with the TT_InSurf

          TubsParams vdParamsTTrackerInner(0.,irvd-2.*vdHalfLength,vdHalfLength);

          VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

          G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();

          if ( verbosityLevel > 0) {
            double zvd = vdg->getGlobal(vdId).z();
            cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
              " z, r : " << zvd  << ", " << irvd << endl;
          }

          //        cout << "foo: TT_MidInner: " << vdLocalOffset         << endl;
          //        cout << "foo: TT_MidInner: " << vdParamsTTrackerInner << endl;

          VolumeInfo vd = nestTubs( VirtualDetector::volumeName(vdId),
                                    vdParamsTTrackerInner, vacuumMaterial, 0,
                                    vdLocalOffset,
                                    parent,
                                    vdId, vdIsVisible, G4Color::Red(), vdIsSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    false);

          doSurfaceCheck && checkForOverlaps(vd.physical, _config, verbosityLevel>0);

          vd.logical->SetSensitiveDetector(vdSD);
        }

      }

      // placing virtual detectors TT_FrontHollow, TT_FrontPA in front
      // of the ttracker (in the proton absorber region); check if
      // ttracker exist is done in VirtualDetectorMaker

      if (    _config.getBool("hasProtonAbsorber",false)
              && !_config.getBool("protonabsorber.isHelical", false)
              && !_config.getBool("protonabsorber.isShorterCone", false)) {

        // This branch is for the case that the proton absorber penetrates this vd

        vdId = VirtualDetectorId::TT_FrontHollow;
        if( vdg->exist(vdId) ) {

          if ( verbosityLevel > 0) {
            cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
          }
          if ( !_config.getBool("hasProtonAbsorber",false) ) {
            throw cet::exception("GEOM")
              << "This virtual detector " << VirtualDetectorId(vdId).name()
              << " can only be placed if proton absorber is present\n";
          }

          // the radius of tracker mother
          TTracker const & ttracker = *(GeomHandle<TTracker>());
          double orvd = ttracker.mother().tubsParams().outerRadius();
          double vdZ  = vdg->getGlobal(vdId).z();

          if ( verbosityLevel > 0) {
            cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
              " z, r : " << vdZ << ", " << orvd << endl;
          }

          // we will create an subtraction solid
          // (we will "subtract" protonAbsorber)
          // and place it (the subtraction solid) in DS3Vacuum

          VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

          G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();

          VolumeInfo vdFullInfo;
          vdFullInfo.name = VirtualDetector::volumeName(vdId) + "_FULL";

          TubsParams  vdParamsTTrackerFrontFull(0.,orvd,vdHalfLength);

          vdFullInfo.solid = new G4Tubs(vdFullInfo.name,
                                        vdParamsTTrackerFrontFull.innerRadius(),
                                        vdParamsTTrackerFrontFull.outerRadius(),
                                        vdParamsTTrackerFrontFull.zHalfLength(),
                                        vdParamsTTrackerFrontFull.phi0(),
                                        vdParamsTTrackerFrontFull.phiMax());

          if ( verbosityLevel > 0) {
            cout << __func__ << " constructing " <<  vdFullInfo.name << endl;
          }

          VolumeInfo const & protonabs2Info = _helper->locateVolInfo("protonabs2");

          VolumeInfo vdHollowInfo;
          if ( verbosityLevel > 0) {
            cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
          }
          vdHollowInfo.name = VirtualDetector::volumeName(vdId);

          // we need to make sure that the vd z is within protonabs2 z
          // in addition the outomatic check if vd is inside ds3Vac is done by G4 itself

          double pabs2Z = protonabs2Info.centerInMu2e()[CLHEP::Hep3Vector::Z];
          double pzhl   = static_cast<G4Cons*>(protonabs2Info.solid)->GetZHalfLength();

          if (verbosityLevel >0) {
            cout << __func__ << " " << protonabs2Info.name << " Z offset in Mu2e    : " <<
              pabs2Z << endl;
            cout << __func__ << " " << protonabs2Info.name << " Z extent in Mu2e    : " <<
              pabs2Z - pzhl  << ", " <<  pabs2Z + pzhl  << endl;
            cout << __func__ << " " << vdFullInfo.name     << " Z offset in Mu2e    : " <<
              vdZ << endl;
            cout << __func__ << " " << vdFullInfo.name     << " Z extent in Mu2e    : " <<
              vdZ - vdHalfLength << ", " << vdZ + vdHalfLength << endl;
          }

          if ( (pabs2Z+pzhl-vdZ-vdHalfLength)<0.0) {
            throw cet::exception("GEOM")
              << "Incorrect positioning of "
              << protonabs2Info.name
              << " and "
              << vdFullInfo.name
              <<"\n";
          }

          // need to find the relative offset of vd & protonAbsorber;
          // we will use the global offsets; they are (in Mu2e)
          // vdg->getGlobal(vdId)
          // protonabs2Info.centerInMu2e()
          // only global offsets can be used for vdet TT_FrontHollow, TT_FrontPA

          if ( verbosityLevel > 0) {
            cout << __func__ << " constructing " <<  vdHollowInfo.name << " name check " << endl;
          }

          vdHollowInfo.solid = new G4SubtractionSolid(vdHollowInfo.name,
                                                      vdFullInfo.solid,
                                                      protonabs2Info.solid,
                                                      0,
                                                      protonabs2Info.centerInMu2e()-
                                                      vdg->getGlobal(vdId));

          vdHollowInfo.centerInParent = vdLocalOffset;
          vdHollowInfo.centerInWorld  = vdHollowInfo.centerInParent + parent.centerInWorld;

          finishNesting(vdHollowInfo,
                        vacuumMaterial,
                        0,
                        vdLocalOffset,
                        parent.logical,
                        vdId,
                        vdIsVisible,
                        G4Color::Red(),
                        vdIsSolid,
                        forceAuxEdgeVisible,
                        placePV,
                        false);

          doSurfaceCheck && checkForOverlaps(vdHollowInfo.physical, _config, verbosityLevel>0);

          if ( verbosityLevel > 0) {

            // both protonabs2 & vd are placed in DS3Vacuum, do they have proper local offsets?

            double theZ  = vdHollowInfo.centerInMu2e()[CLHEP::Hep3Vector::Z];
            double theHL = static_cast<G4Tubs*>(vdFullInfo.solid)->GetZHalfLength();
            cout << __func__ << " " << vdHollowInfo.name <<
              " Z offset in Mu2e    : " <<
              theZ << endl;
            cout << __func__ << " " << vdHollowInfo.name <<
              " Z extent in Mu2e    : " <<
              theZ - theHL << ", " << theZ + theHL << endl;

            cout << __func__ << " " << vdHollowInfo.name <<
              " local input offset in G4                  : " <<
              vdLocalOffset << endl;
            cout << __func__ << " " << vdHollowInfo.name <<
              " local GetTranslation()       offset in G4 : " <<
              vdHollowInfo.physical->GetTranslation() << endl;

            cout << __func__ << " " << protonabs2Info.name <<
              " local GetTranslation()              offset in G4 : " <<
              protonabs2Info.physical->GetTranslation() << endl;

            cout << __func__ << " " << protonabs2Info.name << " " << vdHollowInfo.name <<
              " local GetTranslation() offset diff in G4 : " <<
              vdHollowInfo.physical->GetTranslation() - protonabs2Info.physical->GetTranslation() <<
              endl;
            cout << __func__ <<
              " protonabs2Info.centerInMu2e() - vdg->getGlobal(vdId) offset           : " <<
              protonabs2Info.centerInMu2e()-vdg->getGlobal(vdId) << endl;
          }

          vdHollowInfo.logical->SetSensitiveDetector(vdSD);

          //  now the complementary solid, it has to be placed in protonabs2

          vdId = VirtualDetectorId::TT_FrontPA;
          if (vdg->exist(vdId)) {
            if ( verbosityLevel > 0) {
              cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
            }
            VolumeInfo vdIntersectionInfo;
            vdIntersectionInfo.name = VirtualDetector::volumeName(vdId);

            vdIntersectionInfo.solid = new G4IntersectionSolid(vdIntersectionInfo.name + "_INT",
                                                               vdFullInfo.solid,
                                                               protonabs2Info.solid,
                                                               0,
                                                               protonabs2Info.centerInMu2e()-
                                                               vdg->getGlobal(vdId));

            VolumeInfo const & parent = _helper->locateVolInfo("protonabs2");
            vdLocalOffset = vdg->getGlobal(vdId)-protonabs2Info.centerInMu2e();

            vdIntersectionInfo.centerInParent = vdLocalOffset;
            vdIntersectionInfo.centerInWorld  = vdIntersectionInfo.centerInParent +
              parent.centerInWorld;

            finishNesting(vdIntersectionInfo,
                          vacuumMaterial,
                          0,
                          vdLocalOffset,
                          parent.logical,
                          vdId,
                          vdIsVisible,
                          G4Color::Red(),
                          vdIsSolid,
                          forceAuxEdgeVisible,
                          placePV,
                          false);

            doSurfaceCheck && checkForOverlaps(vdIntersectionInfo.physical, _config, verbosityLevel>0);

            if ( verbosityLevel > 0) {

              // vd is placed in protonabs2

              double theZ  = vdIntersectionInfo.centerInMu2e()[CLHEP::Hep3Vector::Z];
              double theHL = static_cast<G4Tubs*>(vdFullInfo.solid)->GetZHalfLength();
              cout << __func__ << " " << vdIntersectionInfo.name <<
                " Z offset in Mu2e    : " <<
                theZ << endl;
              cout << __func__ << " " << vdIntersectionInfo.name <<
                " Z extent in Mu2e    : " <<
                theZ - theHL << ", " << theZ + theHL << endl;

              cout << __func__ << " " << vdIntersectionInfo.name <<
                " local input offset in G4                  : " <<
                vdLocalOffset << endl;
              cout << __func__ << " " << vdIntersectionInfo.name <<
                " local GetTranslation()       offset in G4 : " <<
                vdIntersectionInfo.physical->GetTranslation() << endl;

              cout << __func__ << " " << protonabs2Info.name <<
                " local GetTranslation()              offset in G4 : " <<
                protonabs2Info.physical->GetTranslation() << endl;

              cout << __func__ << " " << protonabs2Info.name << " " << vdIntersectionInfo.name <<
                " local GetTranslation() offset diff in G4 : " <<
                vdIntersectionInfo.physical->GetTranslation() -
                protonabs2Info.physical->GetTranslation() << endl;
              cout << __func__ <<
                " protonabs2Info.centerInMu2e() - vdg->getGlobal(vdId) offset           : " <<
                protonabs2Info.centerInMu2e()-vdg->getGlobal(vdId) << endl;
            }


            vdIntersectionInfo.logical->SetSensitiveDetector(vdSD);

          }
        }
      }
      else {

        // If there is no proton absorber that penetrates the front VD, then the VD is a simple disk.
        // Keep the same name even though the "hollow" part of the name is not really appropriate ...

        vdId = VirtualDetectorId::TT_FrontHollow;
        if( vdg->exist(vdId) ) {

          if ( verbosityLevel > 0) {
            cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
          }

          // the radius of tracker mother
          TTracker const & ttracker = *(GeomHandle<TTracker>());
          double orvd = ttracker.mother().tubsParams().outerRadius();
          double vdZ  = vdg->getGlobal(vdId).z();

          if ( verbosityLevel > 0) {
            cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
              " z, r : " << vdZ << ", " << orvd << endl;
          }

          VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

          G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();

          TubsParams  vdParamsTTrackerFrontFull(0.,orvd,vdHalfLength);

          //          cout << "foo: TT_Front: " << vdLocalOffset             << endl;
          //          cout << "foo: TT_Front: " << vdParamsTTrackerFrontFull << endl;
          VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                       vdParamsTTrackerFrontFull,
                                       vacuumMaterial,
                                       0,
                                       vdLocalOffset,
                                       parent,
                                       vdId,
                                       vdIsVisible,
                                       G4Color::Red(),
                                       vdIsSolid,
                                       forceAuxEdgeVisible,
                                       placePV,
                                       false);

          doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

          vdInfo.logical->SetSensitiveDetector(vdSD);

        }

      }

      vdId = VirtualDetectorId::TT_Back;
      if( vdg->exist(vdId) ) {

        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
        }
        // the radius of tracker mother
        TTracker const & ttracker = *(GeomHandle<TTracker>());
        double orvd = ttracker.mother().tubsParams().outerRadius();
        double vdZ  = vdg->getGlobal(vdId).z();

        if ( verbosityLevel > 0) {
          cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
            " z, r : " << vdZ << ", " << orvd << endl;
        }

        VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

        G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();

        TubsParams  vdParamsTTrackerBackFull(0.,orvd,vdHalfLength);

        //        cout << "foo: TT_Back: " << vdLocalOffset    << endl;
        //        cout << "foo: TT_Back: " << vdParamsTTrackerBackFull << endl;

        VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                     vdParamsTTrackerBackFull,
                                     vacuumMaterial,
                                     0,
                                     vdLocalOffset,
                                     parent,
                                     vdId,
                                     vdIsVisible,
                                     G4Color::Red(),
                                     vdIsSolid,
                                     forceAuxEdgeVisible,
                                     placePV,
                                     false);

        doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

        vdInfo.logical->SetSensitiveDetector(vdSD);

      }

      vdId = VirtualDetectorId::TT_OutSurf;
      if( vdg->exist(vdId) ) {

        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
        }

        // the radius of tracker mother
        TTracker const & ttracker = *(GeomHandle<TTracker>());
        TubsParams const& motherParams = ttracker.mother().tubsParams();
        double orvd = motherParams.outerRadius();
        double vdZ  = vdg->getGlobal(vdId).z();

        if ( verbosityLevel > 0) {
          cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
            " z, r : " << vdZ << ", " << orvd << endl;
        }

        VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

        G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();

        // the detector is on the outer surface of the ttracker envelope
        // it is thin cylinder, NOT a thin disk
        TubsParams  vdParamsTTrackerOutSurf(orvd,orvd+2.*vdHalfLength,motherParams.zHalfLength());

        //        cout << "foo: TT_OutSurf: " << vdLocalOffset         << endl;
        //        cout << "foo: TT_OutSurf: " << vdParamsTTrackerOutSurf << endl;

        VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                     vdParamsTTrackerOutSurf,
                                     vacuumMaterial,
                                     0,
                                     vdLocalOffset,
                                     parent,
                                     vdId,
                                     vdIsVisible,
                                     G4Color::Red(),
                                     vdIsSolid,
                                     forceAuxEdgeVisible,
                                     placePV,
                                     false);

        doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

        vdInfo.logical->SetSensitiveDetector(vdSD);

      }

      vdId = VirtualDetectorId::TT_InSurf;
      if( vdg->exist(vdId) ) {

        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
        }

        // the radius of tracker mother
        TTracker const & ttracker = *(GeomHandle<TTracker>());
        TubsParams const& motherParams = ttracker.mother().tubsParams();
        double irvd = motherParams.innerRadius();
        double vdZ  = vdg->getGlobal(vdId).z();

        if ( verbosityLevel > 0) {
          cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
            " z, r : " << vdZ << ", " << irvd << endl;
        }

        VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

        G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();

        // the detector is on the inner surface of the ttracker envelope
        // it is thin cylinder, NOT a thin disk
        TubsParams  vdParamsTTrackerInSurf(irvd-2.*vdHalfLength,irvd,motherParams.zHalfLength());

        //        cout << "foo: TT_InSurf: " << vdLocalOffset         << endl;
        //        cout << "foo: TT_InSurf: " << vdParamsTTrackerInSurf << endl;

        VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                     vdParamsTTrackerInSurf,
                                     vacuumMaterial,
                                     0,
                                     vdLocalOffset,
                                     parent,
                                     vdId,
                                     vdIsVisible,
                                     G4Color::Red(),
                                     vdIsSolid,
                                     forceAuxEdgeVisible,
                                     placePV,
                                     false);

        doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

        vdInfo.logical->SetSensitiveDetector(vdSD);

      }

    } // end hasTTracker
    else if ( _config.getBool("hasITracker",false) && _config.getBool("itracker.VirtualDetect",false) ) {

      VolumeInfo const & trckrParent = _helper->locateVolInfo("TrackerMother");

      double tModInRd = ((G4Tubs *)trckrParent.solid)->GetInnerRadius();
      double tModOtRd = ((G4Tubs *)trckrParent.solid)->GetOuterRadius();
      double tModDz   = ((G4Tubs *)trckrParent.solid)->GetDz();


      vdId = VirtualDetectorId::IT_VD_InSurf;
      if( vdg->exist(vdId) ) {

        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
        }

        // the radius of tracker mother
        double irvd = tModInRd; //envelopeParams.innerRadius();
        double vdZ  = vdg->getGlobal(vdId).z();

        if ( verbosityLevel > 0) {
          cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
            " z, r : " << vdZ << ", " << irvd << endl;
        }

        VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

        G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();

        // the detector is on the inner surface of the tracker envelope
        // it is thin cylinder, NOT a thin disk
        TubsParams  vdParamsITrackerInSurf(irvd-2.*vdHalfLength,irvd,tModDz);

        VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                     vdParamsITrackerInSurf,
                                     vacuumMaterial,
                                     0,
                                     vdLocalOffset,
                                     parent,
                                     vdId,
                                     vdIsVisible,
                                     G4Color::Red(),
                                     vdIsSolid,
                                     forceAuxEdgeVisible,
                                     placePV,
                                     false);

        doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

        vdInfo.logical->SetSensitiveDetector(vdSD);
      }

      vdId = VirtualDetectorId::IT_VD_EndCap_Front;
      if( vdg->exist(vdId) ) {

        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
        }

        // the radius of tracker mother
        double irvd = tModInRd; //envelopeParams.innerRadius();
        double vdZ  = vdg->getGlobal(vdId).z();

        if ( verbosityLevel > 0) {
          cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
            " z, r : " << vdZ << ", " << irvd << endl;
        }

        VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

        G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();// -G4ThreeVector(0.0,0.0,-(tModDz+vdHalfLength));

        // the detector is on the inner surface of the tracker envelope
        // it is thin cylinder, NOT a thin disk
        TubsParams  vdParamsITrackerInSurf(tModInRd,tModOtRd,vdHalfLength);

        VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                     vdParamsITrackerInSurf,
                                     vacuumMaterial,
                                     0,
                                     vdLocalOffset,
                                     parent,
                                     vdId,
                                     vdIsVisible,
                                     G4Color::Red(),
                                     vdIsSolid,
                                     forceAuxEdgeVisible,
                                     placePV,
                                     false);

        doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

        vdInfo.logical->SetSensitiveDetector(vdSD);
      }

      vdId = VirtualDetectorId::IT_VD_EndCap_Back;
      if( vdg->exist(vdId) ) {

        if ( verbosityLevel > 0) {
          cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
        }

        // the radius of tracker mother
        double irvd = tModInRd; //envelopeParams.innerRadius();
        double vdZ  = vdg->getGlobal(vdId).z();

        if ( verbosityLevel > 0) {
          cout << __func__ << " " << VirtualDetector::volumeName(vdId) <<
            " z, r : " << vdZ << ", " << irvd << endl;
        }

        VolumeInfo const & parent = _helper->locateVolInfo("DS3Vacuum");

        G4ThreeVector vdLocalOffset = vdg->getGlobal(vdId) - parent.centerInMu2e();// -G4ThreeVector(0.0,0.0,(tModDz+vdHalfLength));

        // the detector is on the inner surface of the tracker envelope
        // it is thin cylinder, NOT a thin disk
        TubsParams  vdParamsITrackerInSurf(tModInRd,tModOtRd,vdHalfLength);

        VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                     vdParamsITrackerInSurf,
                                     vacuumMaterial,
                                     0,
                                     vdLocalOffset,
                                     parent,
                                     vdId,
                                     vdIsVisible,
                                     G4Color::Red(),
                                     vdIsSolid,
                                     forceAuxEdgeVisible,
                                     placePV,
                                     false);

        doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

        vdInfo.logical->SetSensitiveDetector(vdSD);
      }

    } // end hasITracker

    vdId = VirtualDetectorId::EMFC1Entrance;
    if( vdg->exist(vdId) ) {

      if ( verbosityLevel > 0) {
        cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
      }

      VolumeInfo const & parent = _helper->locateVolInfo("HallAir");
      GeomHandle<ProtonBeamDump> dump;

      const double vdYmin = dump->frontShieldingCenterInMu2e().y()
        - dump->frontShieldingHalfSize()[1]
        + building->hallFloorThickness()
        ;
      const double vdYmax = std::min(
                                     dump->frontShieldingCenterInMu2e().y() + dump->frontShieldingHalfSize()[1],
                                     building->hallInsideYmax()
                                     );

      std::vector<double> hlen(3);
      hlen[0] = dump->frontShieldingHalfSize()[0];
      hlen[1] = (vdYmax - vdYmin)/2;
      hlen[2] = vdg->getHalfLength();

      // NB: it's not "shielding" center in Y in case the ceiling height is a limitation
      CLHEP::Hep3Vector shieldingFaceCenterInMu2e( (dump->shieldingFaceXmin()+
                                                    dump->shieldingFaceXmax())/2,

                                                   (vdYmax + vdYmin)/2,

                                                   (dump->shieldingFaceZatXmin()+
                                                    dump->shieldingFaceZatXmax())/2
                                                   );

      CLHEP::Hep3Vector vdOffset(dump->coreRotationInMu2e() * CLHEP::Hep3Vector(0, 0, hlen[2]));


      if ( verbosityLevel > 0) {
        std::cout<<"shieldingFaceCenterInMu2e = "<<shieldingFaceCenterInMu2e
                 <<", parent.centerInMu2e() = "<<parent.centerInMu2e()
                 <<", vdOffset = "<<vdOffset
                 <<std::endl;
      }

      VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdId),
                                  hlen,
                                  vacuumMaterial,
                                  reg.add(dump->coreRotationInMu2e().inverse()),
                                  shieldingFaceCenterInMu2e + vdOffset - parent.centerInMu2e(),
                                  parent,
                                  vdId,
                                  vdIsVisible,
                                  G4Color::Red(),
                                  vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false);

      doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

      vdInfo.logical->SetSensitiveDetector(vdSD);
    }


    // placing virtual detector on the exit (beam dump direction) and inside
    // of PS vacuum,  right before the PS enclosure end plate.
    vdId = VirtualDetectorId::PS_FrontExit;
    if ( vdg->exist(vdId) )
      {
        const VolumeInfo& parent = _helper->locateVolInfo("PSVacuum");

        const Tube& psevac = GeomHandle<PSVacuum>()->vacuum();

        TubsParams vdParams(0., psevac.outerRadius(), vdg->getHalfLength());

        G4ThreeVector vdCenterInParent(0., 0., -psevac.halfLength() + vdg->getHalfLength());

        VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                     vdParams,
                                     vacuumMaterial,
                                     0,
                                     vdCenterInParent,
                                     parent,
                                     vdId,
                                     vdIsVisible,
                                     G4Color::Red(),
                                     vdIsSolid,
                                     forceAuxEdgeVisible,
                                     placePV,
                                     false
                                     );

        doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

        vdInfo.logical->SetSensitiveDetector(vdSD);
      }

    if ( _config.getBool("hasExtMonUCI", false) )
    {
    // placing virtual detector infront of ExtMonUCI removable shielding
    vdId = VirtualDetectorId::EMIEntrance1;
    if( vdg->exist(vdId) ) {
      VolumeInfo const & parent = _helper->locateVolInfo("HallAir");
      GeomHandle<ExtMonUCI::ExtMon> extmon;

      std::vector<double> hlen(3);
      hlen[0] = _config.getDouble("vd.ExtMonUCIEntrance1.HalfLengths", 100.0);
      hlen[1] = _config.getDouble("vd.ExtMonUCIEntrance1.HalfLengths", 100.0);
      hlen[2] = vdg->getHalfLength();

      std::vector<double> pos;
      _config.getVectorDouble("vd.ExtMonUCIEntrance1.Position", pos, 3);
      G4ThreeVector vdLocalOffset = G4ThreeVector( pos[0], pos[1], pos[2] ) - parent.centerInMu2e();

      VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdId),
                                  hlen,
                                  vacuumMaterial,
                                  0,
                                  vdLocalOffset,
                                  parent,
                                  vdId,
                                  vdIsVisible,
                                  G4Color::Red(),
                                  vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false
                                  );

      doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

      vdInfo.logical->SetSensitiveDetector(vdSD);
    }

    // placing virtual detector between ExtMonUCI removable shielding and front shielding
    vdId = VirtualDetectorId::EMIEntrance2;
    if( vdg->exist(vdId) ) {
      VolumeInfo const & parent = _helper->locateVolInfo("ExtMonUCI");
      GeomHandle<ExtMonUCI::ExtMon> extmon;

      std::vector<double> hlen(3);
      hlen[0] = extmon->envelopeParams()[0];
      hlen[1] = extmon->envelopeParams()[1];
      hlen[2] = vdg->getHalfLength();

      VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdId),
                                  hlen,
                                  vacuumMaterial,
                                  0,
                                  vdg->getLocal(vdId),
                                  parent,
                                  vdId,
                                  vdIsVisible,
                                  G4Color::Red(),
                                  vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false
                                  );

      doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

      vdInfo.logical->SetSensitiveDetector(vdSD);
    }

    // placing virtual detector at the entrance and exit collimator0-3
    vdId = VirtualDetectorId::EMIC0Entrance;
    for (int iCol = 0; iCol < 4; iCol++) {
      if( vdg->exist(vdId) ) {
        VolumeInfo const & parent = _helper->locateVolInfo("ExtMonUCI");
        GeomHandle<ExtMonUCI::ExtMon> extmon;

        std::vector<double> hlen(3);
        hlen[0] = extmon->col(iCol)->paramsOuter()[0];
        hlen[1] = extmon->col(iCol)->paramsOuter()[1];
        hlen[2] = vdg->getHalfLength();

        for (int iFrontBack = 0; iFrontBack < 2; iFrontBack++) {
          VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdId),
                                      hlen,
                                      vacuumMaterial,
                                      0,
                                      vdg->getLocal(vdId),
                                      parent,
                                      vdId,
                                      vdIsVisible,
                                      G4Color::Red(),
                                      vdIsSolid,
                                      forceAuxEdgeVisible,
                                      placePV,
                                      false
                                      );

          doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

          vdInfo.logical->SetSensitiveDetector(vdSD);

          vdId++;
        }
      }
    }
    } // hasExtMonUCI

    // An XY plane between the PS and anything ExtMon
    vdId = VirtualDetectorId::ExtMonCommonPlane;
    if( vdg->exist(vdId) ) {
      if ( verbosityLevel > 0) {
        cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
      }

      VolumeInfo const & parent = _helper->locateVolInfo("HallAir");
      GeomHandle<ProtonBeamDump> dump;

      const double requested_z = _config.getDouble("vd.ExtMonCommonPlane.z");

      const double xmin = std::min(building->hallInsideXPSCorner(),
                                   dump->frontShieldingCenterInMu2e()[0]
                                   + (requested_z - dump->frontShieldingCenterInMu2e()[2])*tan(dump->coreRotY())
                                   - dump->frontShieldingHalfSize()[0]/cos(dump->coreRotY())
                                   );

      CLHEP::Hep3Vector centerInMu2e(
                                     (building->hallInsideXmax() + xmin)/2,
                                     (building->hallInsideYmax() + building->hallInsideYmin())/2,
                                     requested_z
                                     );

      std::vector<double> hlen(3);
      hlen[0] = (building->hallInsideXmax() - xmin)/2;
      hlen[1] = (building->hallInsideYmax() - building->hallInsideYmin())/2;
      hlen[2] = vdg->getHalfLength();

      VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdId),
                                  hlen,
                                  vacuumMaterial,
                                  0,
                                  centerInMu2e - parent.centerInMu2e(),
                                  parent,
                                  vdId,
                                  vdIsVisible,
                                  G4Color::Red(),
                                  vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false
                                  );

      doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

      vdInfo.logical->SetSensitiveDetector(vdSD);
    }

    // placing virtual detector at the dump core face
    vdId = VirtualDetectorId::ProtonBeamDumpCoreFace;
    if( vdg->exist(vdId) ) {
      if ( verbosityLevel > 0) {
        cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
      }

      VolumeInfo const & parent = _helper->locateVolInfo("ProtonBeamDumpCore");

      GeomHandle<ProtonBeamDump> dump;

      CLHEP::Hep3Vector centerInCore(0, 0, dump->coreHalfSize()[2] - vdg->getHalfLength());

      std::vector<double> hlen(3);
      hlen[0] = dump->coreHalfSize()[0];
      hlen[1] = dump->coreHalfSize()[1];
      hlen[2] = vdg->getHalfLength();

      VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdId),
                                  hlen,
                                  vacuumMaterial,
                                  0,
                                  centerInCore,
                                  parent,
                                  vdId,
                                  vdIsVisible,
                                  G4Color::Red(),
                                  vdIsSolid,
                                  forceAuxEdgeVisible,
                                  placePV,
                                  false
                                  );

      doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

      vdInfo.logical->SetSensitiveDetector(vdSD);
    }

    // ExtMonFNAL detector VDs - created in constructExtMonFNAL()

    // placing virtual detector at exit of DS neutron shielding
    vdId = VirtualDetectorId::DSNeutronShieldExit;
    if( vdg->exist(vdId) ) {
      if ( verbosityLevel > 0) {
        cout << __func__ << " constructing " << VirtualDetector::volumeName(vdId)  << endl;
      }

      VolumeInfo const & parent = _helper->locateVolInfo("HallAir");

      GeomHandle<ExtNeutShieldCendBoxes> enscendb;

      // Get box with hole
      size_t iHoleBox (0);
      for ( ; iHoleBox < enscendb->materialNames().size() ; iHoleBox++ )
        {
          if ( enscendb->hasHole(iHoleBox) ) break;
        }

      CLHEP::Hep3Vector location = enscendb->holeLocation(iHoleBox);
      location.setZ( location.z() + enscendb->holeHalfLength(0) + vdg->getHalfLength() );

      VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdId),
                                   TubsParams(0., enscendb->holeRadius(0), vdg->getHalfLength()),
                                   vacuumMaterial,
                                   0,
                                   location-parent.centerInMu2e(),
                                   parent,
                                   vdId,
                                   vdIsVisible,
                                   G4Color::Red(),
                                   vdIsSolid,
                                   forceAuxEdgeVisible,
                                   placePV,
                                   false
                                   );

      doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);

      vdInfo.logical->SetSensitiveDetector(vdSD);
    }

    // placing virtual detector around the calorimeter vanes

    if ( _config.getBool("hasVaneCalorimeter",true) )
    {

      int vdIdFront = VirtualDetectorId::EMC_0_FrontIn;
      int vdIdEdge  = VirtualDetectorId::EMC_0_EdgeIn;
      int vdIdSurf  = VirtualDetectorId::EMC_0_SurfIn;

      VaneCalorimeter const& cal = *(GeomHandle<VaneCalorimeter>());

      VolumeInfo const& parent = _helper->locateVolInfo("CalorimeterMother");
      CLHEP::Hep3Vector const& parentInMu2e = parent.centerInMu2e();

      for (int iv = 0; iv < cal.nVane(); iv++)
      {

          const CLHEP::Hep3Vector & size = cal.vane(iv).size();
          G4ThreeVector posVane          = cal.origin() + cal.vane(iv).originLocal();
          double deltax                  = size.x()+2.0*vdg->getHalfLength()+0.02;
          double deltay                  = size.y()+2.0*vdg->getHalfLength()+0.02;

          //Front = small side facing the beam, closest to tracker
          //Edge  = small face closest to z-axis (r_in / r_out)
          //Surf  = largest side (hit by electrons)

          G4ThreeVector posFront1 = posVane - G4ThreeVector(0,0,size.z()+2.0*vdg->getHalfLength()+0.02) - parentInMu2e;
          G4ThreeVector posFront2 = posVane + G4ThreeVector(0,0,size.z()+2.0*vdg->getHalfLength()+0.02) - parentInMu2e;
          G4ThreeVector posEdge1  = posVane + cal.vane(iv).rotation().inverse()*CLHEP::Hep3Vector(0,deltay,0) - parentInMu2e;
          G4ThreeVector posEdge2  = posVane + cal.vane(iv).rotation().inverse()*CLHEP::Hep3Vector(0,-deltay,0) - parentInMu2e;
          G4ThreeVector posSurf1  = posVane + cal.vane(iv).rotation().inverse()*CLHEP::Hep3Vector(deltax,0,0) - parentInMu2e;
          G4ThreeVector posSurf2  = posVane + cal.vane(iv).rotation().inverse()*CLHEP::Hep3Vector(-deltax,0,0) - parentInMu2e;

          double dimVDFront[3] = {size.x(),    size.y(),    vdg->getHalfLength()};
          double dimVDSurf[3]  = {vdg->getHalfLength(), size.y(),    size.z()};
          double dimVDEdge[3]  = {size.x(),    vdg->getHalfLength(), size.z()};


          if( vdg->exist(vdIdFront) )
          {
              VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdIdFront),
                                          dimVDFront,
                                          vacuumMaterial,
                                          &cal.vane(iv).rotation(),
                                          posFront1,
                                          parent,
                                          vdIdFront,
                                          vdIsVisible,
                                          G4Color::Red(),
                                          vdIsSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          false
                                          );
              ++vdIdFront;

              VolumeInfo vdInfo2 = nestBox(VirtualDetector::volumeName(vdIdFront),
                                          dimVDFront,
                                          vacuumMaterial,
                                          &cal.vane(iv).rotation(),
                                          posFront2,
                                          parent,
                                          vdIdFront,
                                          vdIsVisible,
                                          G4Color::Red(),
                                          vdIsSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          false
                                          );
              ++vdIdFront;

              doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);
              doSurfaceCheck && checkForOverlaps(vdInfo2.physical, _config, verbosityLevel>0);
              vdInfo.logical->SetSensitiveDetector(vdSD);
              vdInfo2.logical->SetSensitiveDetector(vdSD);

          }

          if( vdg->exist(vdIdEdge) )
          {
              VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdIdEdge),
                                          dimVDEdge,
                                          vacuumMaterial,
                                          &cal.vane(iv).rotation(),
                                          posEdge1,
                                          parent,
                                          vdIdEdge,
                                          vdIsVisible,
                                          G4Color::Red(),
                                          vdIsSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          false
                                          );
              ++vdIdEdge;

              VolumeInfo vdInfo2 = nestBox(VirtualDetector::volumeName(vdIdEdge),
                                          dimVDEdge,
                                          vacuumMaterial,
                                          &cal.vane(iv).rotation(),
                                          posEdge2,
                                          parent,
                                          vdIdEdge,
                                          vdIsVisible,
                                          G4Color::Red(),
                                          vdIsSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          false
                                          );
              ++vdIdEdge;

              doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);
              doSurfaceCheck && checkForOverlaps(vdInfo2.physical, _config, verbosityLevel>0);
              vdInfo.logical->SetSensitiveDetector(vdSD);
              vdInfo2.logical->SetSensitiveDetector(vdSD);
          }

          if( vdg->exist(vdIdSurf) )
          {
              VolumeInfo vdInfo = nestBox(VirtualDetector::volumeName(vdIdSurf),
                                          dimVDSurf,
                                          vacuumMaterial,
                                          &cal.vane(iv).rotation(),
                                          posSurf1,
                                          parent,
                                          vdIdSurf,
                                          vdIsVisible,//vdIsVisible,
                                          G4Color::Red(),
                                          vdIsSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          false
                                          );

             ++vdIdSurf;

              VolumeInfo vdInfo2 = nestBox(VirtualDetector::volumeName(vdIdSurf),
                                          dimVDSurf,
                                          vacuumMaterial,
                                          &cal.vane(iv).rotation(),
                                          posSurf2,
                                          parent,
                                          vdIdSurf,
                                          vdIsVisible,//vdIsVisible,
                                          G4Color::Red(),
                                          vdIsSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          false
                                          );

              ++vdIdSurf;

              doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);
              doSurfaceCheck && checkForOverlaps(vdInfo2.physical, _config, verbosityLevel>0);
              vdInfo.logical->SetSensitiveDetector(vdSD);
              vdInfo2.logical->SetSensitiveDetector(vdSD);
          }

      }//end vane loop
    }//end if VaneCalorimeter





    if ( _config.getBool("hasDiskCalorimeter",true) ) {

      int vdIdEdge = VirtualDetectorId::EMC_Disk_0_EdgeIn;
      int vdIdSurf = VirtualDetectorId::EMC_Disk_0_SurfIn;

      DiskCalorimeter const& cal = *(GeomHandle<DiskCalorimeter>());
      VolumeInfo const& parent = _helper->locateVolInfo("CalorimeterMother");
      CLHEP::Hep3Vector const& parentInMu2e = parent.centerInMu2e();


      for (size_t id = 0; id < cal.nDisk(); id++)
      {

           const CLHEP::Hep3Vector & size = cal.disk(id).size();
           G4ThreeVector posDisk = cal.origin() + cal.disk(id).originLocal();
           double delta          = 2*vdg->getHalfLength()+0.02;

           TubsParams  vdParamsFront(size[0]-delta,   size[1]+delta,   vdg->getHalfLength());
           TubsParams  vdParamsInner(size[0]-2*delta, size[0]-delta,   size[2]/2.0);
           TubsParams  vdParamsOuter(size[1]+delta,   size[1]+2*delta, size[2]/2.0);

           G4ThreeVector posFront = posDisk - parentInMu2e - G4ThreeVector(0,0,size.z()/2.0+delta);
           G4ThreeVector posBack  = posDisk - parentInMu2e + G4ThreeVector(0,0,size.z()/2.0+delta);
           G4ThreeVector posInner = posDisk - parentInMu2e;

           if( vdg->exist(vdIdSurf) )
           {

               VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdIdSurf),
                                            vdParamsFront,
                                            vacuumMaterial,
                                            0,
                                            posFront,
                                            parent,
                                            vdIdSurf,
                                            vdIsVisible,
                                            G4Color::Red(),
                                            vdIsSolid,
                                            forceAuxEdgeVisible,
                                            placePV,
                                            false);
               ++vdIdSurf;

               VolumeInfo vdInfo2 = nestTubs(VirtualDetector::volumeName(vdIdSurf),
                                            vdParamsFront,
                                            vacuumMaterial,
                                            0,
                                            posBack,
                                            parent,
                                            vdIdSurf,
                                            vdIsVisible,
                                            G4Color::Red(),
                                            vdIsSolid,
                                            forceAuxEdgeVisible,
                                            placePV,
                                            false);
               ++vdIdSurf;

               doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);
               doSurfaceCheck && checkForOverlaps(vdInfo2.physical, _config, verbosityLevel>0);
               vdInfo.logical->SetSensitiveDetector(vdSD);
               vdInfo2.logical->SetSensitiveDetector(vdSD);
           }


           if( vdg->exist(vdIdEdge) )
           {
               VolumeInfo vdInfo = nestTubs(VirtualDetector::volumeName(vdIdEdge),
                                            vdParamsInner,
                                            vacuumMaterial,
                                            0,
                                            posInner,
                                            parent,
                                            vdIdEdge,
                                            vdIsVisible,
                                            G4Color::Red(),
                                            vdIsSolid,
                                            forceAuxEdgeVisible,
                                            placePV,
                                            false);
               ++vdIdEdge;

               VolumeInfo vdInfo2 = nestTubs(VirtualDetector::volumeName(vdIdEdge),
                                            vdParamsOuter,
                                            vacuumMaterial,
                                            0,
                                            posInner,
                                            parent,
                                            vdIdEdge,
                                            vdIsVisible,
                                            G4Color::Red(),
                                            vdIsSolid,
                                            forceAuxEdgeVisible,
                                            placePV,
                                            false);
               ++vdIdEdge;

               doSurfaceCheck && checkForOverlaps(vdInfo.physical, _config, verbosityLevel>0);
               doSurfaceCheck && checkForOverlaps(vdInfo2.physical, _config, verbosityLevel>0);
               vdInfo.logical->SetSensitiveDetector(vdSD);
               vdInfo2.logical->SetSensitiveDetector(vdSD);
           }

      }// end loop on disks
    }//hasDiskCalorimeter

  } // constructVirtualDetectors()
}
