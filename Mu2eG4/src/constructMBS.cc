//
// Free function to create Muon Beam Stop and some elements of the Cryostat in G4
//
// $Id: constructMBS.cc,v 1.17 2013/07/02 15:57:07 tassiell Exp $
// $Author: tassiell $
// $Date: 2013/07/02 15:57:07 $
//
// Original author KLG
//
// Notes:
//
// The initial implementaion is described in Mu2e Document 1519

// Note the interdependence of the position of the CryoSeal on
// the position of the neutron absorber to avoid overlaps, it should
// probably be more related to the SolenoidCoil once the dimensions of
// those components are reconciled

// clhep includes
#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Units/PhysicalConstants.h"

// Mu2e includes.

#include "Mu2eG4/inc/constructMBS.hh"
#include "DetectorSolenoidGeom/inc/DetectorSolenoid.hh"
#include "G4Helper/inc/VolumeInfo.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "CosmicRayShieldGeom/inc/CosmicRayShield.hh"
#include "CosmicRayShieldGeom/inc/CRSSteelShield.hh"
#include "MBSGeom/inc/MBS.hh"
#include "G4Helper/inc/G4Helper.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Mu2eG4/inc/nestBox.hh"
#include "Mu2eG4/inc/nestTubs.hh"
#include "Mu2eG4/inc/nestPolycone.hh"
#include "Mu2eG4/inc/finishNesting.hh"

// G4 includes
#include "G4Material.hh"
#include "G4Color.hh"
#include "G4VSolid.hh"
#include "G4Tubs.hh"
#include "G4Polycone.hh"
#include "G4Cons.hh"
#include "G4SubtractionSolid.hh"
#include "G4VPhysicalVolume.hh"

using namespace std;

namespace mu2e {

  void constructMBS(SimpleConfig const & _config){

    // BSTS is the main support tube; the other z positions are dependent upon it

    MBS const & mbsgh = *(GeomHandle<MBS>());

    Polycone const & pMBSMParams = *mbsgh.getMBSMPtr();
    Tube const & pBSTSParams     = *mbsgh.getBSTSPtr();
    Tube const & pSPBSSup1Params = *mbsgh.getSPBSSup1Ptr();
    Tube const & pSPBSSup2Params = *mbsgh.getSPBSSup2Ptr();
    Tube const & pSPBSLParams    = *mbsgh.getSPBSLPtr();
    Tube const & pSPBSCParams    = *mbsgh.getSPBSCPtr();
    Tube const & pSPBSRParams    = *mbsgh.getSPBSRPtr();
    Polycone const & pBSTCParams = *mbsgh.getBSTCPtr();
    Polycone const & pBSBSParams = *mbsgh.getBSBSPtr();
    Polycone const & pCLV2Params = *mbsgh.getCLV2Ptr();

    bool const MBSisVisible        = _config.getBool("mbs.visible",true);
    bool const MBSisSolid          = _config.getBool("mbs.solid", false);
    int  const verbosityLevel      = _config.getInt("mbs.verbosityLevel", 0);

    bool const forceAuxEdgeVisible = _config.getBool("g4.forceAuxEdgeVisible",false);
    bool const doSurfaceCheck      = _config.getBool("g4.doSurfaceCheck",false);
    bool const placePV             = true;

    // Access to the G4HelperService.
    G4Helper* _helper = &(*(art::ServiceHandle<G4Helper>()));

    // Fetch DS geometry handle
    GeomHandle<DetectorSolenoid> ds;

    if ( verbosityLevel > 0) {
      cout << __func__ << " verbosityLevel                   : " << verbosityLevel  << endl;
      cout << __func__ << " solenoidOffset                   : " << ds->position().x()  << endl;
    }

     // mother volumes
    VolumeInfo const & detSolDownstreamVacInfo = _helper->locateVolInfo("DS3Vacuum");
    VolumeInfo const & hallInfo =                _helper->locateVolInfo("HallAir");

    if ( verbosityLevel > 0) {
      cout << __func__ << " z-extent of DS3Vacuum portion in DS in Mu2e  : " <<
        ds->vac_zLocDs23Split() << ", " << ds->cryoZMax() << endl;
    }


    // MBSM

    CLHEP::Hep3Vector MBSMOffsetInMu2e = pMBSMParams.originInMu2e();
    // now local offset in mother volume
    CLHEP::Hep3Vector MBSMOffset =  MBSMOffsetInMu2e - detSolDownstreamVacInfo.centerInMu2e(); // - MBSMotherInfo.centerInMu2e();

    if ( verbosityLevel > 0) {
      cout << __func__ << " MBSMotherOffsetInMu2e                 : " << MBSMOffsetInMu2e << endl;
      cout << __func__ << " MBSMotherOffsetInMBS                  : " << MBSMOffset << endl;
    }

    VolumeInfo MBSMotherInfo  = nestPolycone("MBSMother",
                                    pMBSMParams.getPolyconsParams(),
                                    findMaterialOrThrow(pMBSMParams.materialName()),
                                    0,
                                    MBSMOffset,
                                    detSolDownstreamVacInfo,
                                    0,
                                    MBSisVisible,
                                    0,
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );


    if ( verbosityLevel > 0) {
      G4Polycone *tmpMBSMInfo  =  static_cast<G4Polycone*>(MBSMotherInfo.solid);
      double zhl = tmpMBSMInfo->GetCorner(tmpMBSMInfo->GetNumRZCorner()-1).z-tmpMBSMInfo->GetCorner(0).z;
      zhl*=0.5;
      double MBSMOffsetInMu2eZ = MBSMOffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " MBSM         Z extent in Mu2e    : " <<
        MBSMOffsetInMu2eZ - zhl << ", " << MBSMOffsetInMu2eZ + zhl << endl;
    }

    // BSTS see WBS 5.8 for the naming conventions

    CLHEP::Hep3Vector BSTSOffsetInMu2e = pBSTSParams.originInMu2e();

    if ( verbosityLevel > 0) {
      cout << __func__ << " BSTSOffsetInMu2e                 : " << BSTSOffsetInMu2e << endl;
    }

    // now local offset in mother volume
    CLHEP::Hep3Vector BSTSOffset =  BSTSOffsetInMu2e - MBSMOffsetInMu2e;

    // Use BSTS as mother volume for MBS
    VolumeInfo BSTSInfo  = nestTubs("BSTS",
                                    pBSTSParams.getTubsParams(),
                                    findMaterialOrThrow(pBSTSParams.materialName()),
                                    0,
				    BSTSOffset,
				    //G4ThreeVector(0,0,0),
				    //detSolDownstreamVacInfo,
				    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    G4Colour::Gray(),
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );

    if ( verbosityLevel > 0) {
      double zhl         = static_cast<G4Tubs*>(BSTSInfo.solid)->GetZHalfLength();
      double BSTSOffsetInMu2eZ = BSTSOffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " BSTSOffsetZ           in Mu2e    : " <<
        BSTSOffsetInMu2eZ << endl;
      cout << __func__ << " BSTS         Z extent in Mu2e    : " <<
        BSTSOffsetInMu2eZ - zhl << ", " << BSTSOffsetInMu2eZ + zhl << endl;
    }

    // SPBSSup1
    // This one is placed directly into DS3Vacuum;
    // Note that its radius is lager than theone of BSTS

    CLHEP::Hep3Vector SPBSSup1OffsetInMu2e = pSPBSSup1Params.originInMu2e();

    if ( verbosityLevel > 0) {
      cout << __func__ << " SPBSSup1OffsetInMu2e                 : " << SPBSSup1OffsetInMu2e << endl;
    }

    // now local offset in mother volume
    CLHEP::Hep3Vector SPBSSup1Offset = SPBSSup1OffsetInMu2e - MBSMOffsetInMu2e;

    VolumeInfo SPBSSup1Info  = nestTubs("SPBSSup1",
                                    pSPBSSup1Params.getTubsParams(),
                                    findMaterialOrThrow(pSPBSSup1Params.materialName()),
                                    0,
                                    SPBSSup1Offset,
                                    //detSolDownstreamVacInfo,
                                    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    G4Colour::Blue(),
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );

    if ( verbosityLevel > 0) {
      double zhl         = static_cast<G4Tubs*>(SPBSSup1Info.solid)->GetZHalfLength();
      double SPBSSup1OffsetInMu2eZ = SPBSSup1OffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " SPBSSup1         Z extent in Mu2e    : " <<
        SPBSSup1OffsetInMu2eZ - zhl << ", " << SPBSSup1OffsetInMu2eZ + zhl << endl;
    }

    // SPBSSup2
    // This one is placed directly into DS3Vacuum;
    // Note that its radius is lager than theone of BSTS

    CLHEP::Hep3Vector SPBSSup2OffsetInMu2e = pSPBSSup2Params.originInMu2e();

    if ( verbosityLevel > 0) {
      cout << __func__ << " SPBSSup2OffsetInMu2e                 : " << SPBSSup2OffsetInMu2e << endl;
    }

    // now local offset in mother volume
    CLHEP::Hep3Vector SPBSSup2Offset = SPBSSup2OffsetInMu2e - MBSMOffsetInMu2e;

    VolumeInfo SPBSSup2Info  = nestTubs("SPBSSup2",
                                    pSPBSSup2Params.getTubsParams(),
                                    findMaterialOrThrow(pSPBSSup2Params.materialName()),
                                    0,
                                    SPBSSup2Offset,
                                    //detSolDownstreamVacInfo,
                                    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    G4Colour::Blue(),
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );

    if ( verbosityLevel > 0) {
      double zhl         = static_cast<G4Tubs*>(SPBSSup2Info.solid)->GetZHalfLength();
      double SPBSSup2OffsetInMu2eZ = SPBSSup2OffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " SPBSSup2         Z extent in Mu2e    : " <<
        SPBSSup2OffsetInMu2eZ - zhl << ", " << SPBSSup2OffsetInMu2eZ + zhl << endl;
    }

    // SPBSL
    // This one is placed directly into DS3Vacuum;
    // Note that its radius is lager than theone of BSTS

    CLHEP::Hep3Vector SPBSLOffsetInMu2e = pSPBSLParams.originInMu2e();

    if ( verbosityLevel > 0) {
      cout << __func__ << " SPBSLOffsetInMu2e                 : " << SPBSLOffsetInMu2e << endl;
    }

    // now local offset in mother volume
    CLHEP::Hep3Vector SPBSLOffset = SPBSLOffsetInMu2e - MBSMOffsetInMu2e;

    VolumeInfo SPBSLInfo  = nestTubs("SPBSL",
                                    pSPBSLParams.getTubsParams(),
                                    findMaterialOrThrow(pSPBSLParams.materialName()),
                                    0,
                                    SPBSLOffset,
                                    //detSolDownstreamVacInfo,
                                    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    G4Colour::Blue(),
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );

    if ( verbosityLevel > 0) {
      double zhl         = static_cast<G4Tubs*>(SPBSLInfo.solid)->GetZHalfLength();
      double SPBSLOffsetInMu2eZ = SPBSLOffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " SPBSL         Z extent in Mu2e    : " <<
        SPBSLOffsetInMu2eZ - zhl << ", " << SPBSLOffsetInMu2eZ + zhl << endl;
    }

    // SPBSC
    // This one is placed directly into DS3Vacuum;
    // Note that its radius is lager than theone of BSTS

    CLHEP::Hep3Vector SPBSCOffsetInMu2e = pSPBSCParams.originInMu2e();

    if ( verbosityLevel > 0) {
      cout << __func__ << " SPBSCOffsetInMu2e                 : " << SPBSCOffsetInMu2e << endl;
    }

    // now local offset in mother volume
    CLHEP::Hep3Vector SPBSCOffset = SPBSCOffsetInMu2e - MBSMOffsetInMu2e;

    VolumeInfo SPBSCInfo  = nestTubs("SPBSC",
                                    pSPBSCParams.getTubsParams(),
                                    findMaterialOrThrow(pSPBSCParams.materialName()),
                                    0,
                                    SPBSCOffset,
                                    //detSolDownstreamVacInfo,
                                    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    G4Colour::Blue(),
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );

    if ( verbosityLevel > 0) {
      double zhl         = static_cast<G4Tubs*>(SPBSCInfo.solid)->GetZHalfLength();
      double SPBSCOffsetInMu2eZ = SPBSCOffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " SPBSC         Z extent in Mu2e    : " <<
        SPBSCOffsetInMu2eZ - zhl << ", " << SPBSCOffsetInMu2eZ + zhl << endl;
    }

    // SPBSR
    // This one is placed directly into DS3Vacuum; 
    // Note that its radius is lager than theone of BSTS
    
    CLHEP::Hep3Vector SPBSROffsetInMu2e = pSPBSRParams.originInMu2e();

    if ( verbosityLevel > 0) {
      cout << __func__ << " SPBSROffsetInMu2e                 : " << SPBSROffsetInMu2e << endl;
    }

    // now local offset in mother volume
    CLHEP::Hep3Vector SPBSROffset = SPBSROffsetInMu2e - MBSMOffsetInMu2e;

    VolumeInfo SPBSRInfo  = nestTubs("SPBSR",
                                    pSPBSRParams.getTubsParams(),
                                    findMaterialOrThrow(pSPBSRParams.materialName()),
                                    0,
                                    SPBSROffset,
                                    //detSolDownstreamVacInfo,
                                    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    G4Colour::Blue(),
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );

    if ( verbosityLevel > 0) {
      double zhl         = static_cast<G4Tubs*>(SPBSRInfo.solid)->GetZHalfLength();
      double SPBSROffsetInMu2eZ = SPBSROffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " SPBSR         Z extent in Mu2e    : " <<
        SPBSROffsetInMu2eZ - zhl << ", " << SPBSROffsetInMu2eZ + zhl << endl;
    }

    // BSTC

    CLHEP::Hep3Vector BSTCOffsetInMu2e = pBSTCParams.originInMu2e();
    // now local offset in mother volume
    CLHEP::Hep3Vector BSTCOffset =  BSTCOffsetInMu2e - MBSMOffsetInMu2e; // - MBSMotherInfo.centerInMu2e();
    //BSTCOffset.setZ(0);

    if ( verbosityLevel > 0) {
      cout << __func__ << " BSTCOffsetInMu2e                 : " << BSTCOffsetInMu2e << endl;
      cout << __func__ << " BSTCOffsetInMBS                  : " << BSTCOffset << endl;
    }

    G4Colour  orange  (.75, .55, .0);
    VolumeInfo BSTCInfo  = nestPolycone("BSTC",
                                    pBSTCParams.getPolyconsParams(),
                                    findMaterialOrThrow(pBSTCParams.materialName()),
                                    0,
                                    BSTCOffset,
                                    //detSolDownstreamVacInfo,
                                    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    orange,
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );


    if ( verbosityLevel > 0) {
      G4Polycone *tmpBSTCInfo  =  static_cast<G4Polycone*>(BSTCInfo.solid);
      double zhl = tmpBSTCInfo->GetCorner(tmpBSTCInfo->GetNumRZCorner()-1).z-tmpBSTCInfo->GetCorner(0).z;
      zhl*=0.5;
      double BSTCOffsetInMu2eZ = BSTCOffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " BSTC         Z extent in Mu2e    : " <<
        BSTCOffsetInMu2eZ - zhl << ", " << BSTCOffsetInMu2eZ + zhl << endl;
    }

    // BSBS

    CLHEP::Hep3Vector BSBSOffsetInMu2e = pBSBSParams.originInMu2e();

    // now local offset in mother volume
    CLHEP::Hep3Vector BSBSOffset =  BSBSOffsetInMu2e - MBSMOffsetInMu2e; //-MBSMotherInfo.centerInMu2e();
    //BSBSOffset.setZ(0.0);

    if ( verbosityLevel > 0) {
      cout << __func__ << " BSBSOffsetInMu2e                 : " << BSBSOffsetInMu2e << endl;
      cout << __func__ << " BSBSOffsetInMBS                  : " << BSBSOffset << endl;
    }

    VolumeInfo BSBSInfo  = nestPolycone("BSBS",
                                    pBSBSParams.getPolyconsParams(),
                                    findMaterialOrThrow(pBSBSParams.materialName()),
                                    0,
                                    BSBSOffset,
                                    //detSolDownstreamVacInfo,
                                    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    G4Colour::Yellow(),
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );

    if ( verbosityLevel > 0) {
      G4Polycone *tmpBSBSInfo  =  static_cast<G4Polycone*>(BSBSInfo.solid);
      double zhl = tmpBSBSInfo->GetCorner(tmpBSBSInfo->GetNumRZCorner()-1).z-tmpBSBSInfo->GetCorner(0).z;
      zhl*=0.5;
      double BSBSOffsetInMu2eZ = BSBSOffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " BSBS         Z extent in Mu2e    : " <<
        BSBSOffsetInMu2eZ - zhl << ", " << BSBSOffsetInMu2eZ + zhl << endl;
    }

    // CLV2

    CLHEP::Hep3Vector CLV2OffsetInMu2e = pCLV2Params.originInMu2e();

    // now local offset in mother volume
    CLHEP::Hep3Vector CLV2Offset = CLV2OffsetInMu2e - MBSMOffsetInMu2e;//- MBSMotherInfo.centerInMu2e();
    //CLV2Offset.setZ(0.0);

    if ( verbosityLevel > 0) {
      cout << __func__ << " CLV2OffsetInMu2e                 : " << CLV2OffsetInMu2e << endl;
      cout << __func__ << " CLV2OffsetInMBS                  : " << CLV2Offset << endl;
    }

    VolumeInfo CLV2Info  = nestPolycone("CLV2",
                                    pCLV2Params.getPolyconsParams(),
                                    findMaterialOrThrow(pCLV2Params.materialName()),
                                    0,
                                    CLV2Offset,
                                    //detSolDownstreamVacInfo,
                                    MBSMotherInfo,
                                    0,
                                    MBSisVisible,
                                    orange,
                                    MBSisSolid,
                                    forceAuxEdgeVisible,
                                    placePV,
                                    doSurfaceCheck
                                    );

    if ( verbosityLevel > 0) {
      G4Polycone *tmpCLV2Info  =  static_cast<G4Polycone*>(CLV2Info.solid);
      double zhl = tmpCLV2Info->GetCorner(tmpCLV2Info->GetNumRZCorner()-1).z-tmpCLV2Info->GetCorner(0).z;
      zhl*=0.5;
      double CLV2OffsetInMu2eZ = CLV2OffsetInMu2e[CLHEP::Hep3Vector::Z];
      cout << __func__ << " CLV2         Z extent in Mu2e    : " <<
        CLV2OffsetInMu2eZ - zhl << ", " << CLV2OffsetInMu2eZ + zhl << endl;
    }

    // constructing endplug closing the cryostat and enclosing the MBS

    // the hollow disk aka CryoSeal

    bool const hasCryoSeal = _config.getBool("mbs.hasCryoSeal", true);

    if (hasCryoSeal) {

      double const CryoSealInnerRadius = _config.getDouble("mbs.CryoSealInnerRadius");
      double const CryoSealOuterRadius = _config.getDouble("mbs.CryoSealOuterRadius");
      double const CryoSealHLength     = _config.getDouble("mbs.CryoSealHLength");

      TubsParams CryoSealParams ( CryoSealInnerRadius,
                                  CryoSealOuterRadius,
                                  CryoSealHLength);

      double CryoSealZ           = _config.getDouble("mbs.CryoSealZ");

      CLHEP::Hep3Vector CryoSealOffsetInMu2e = CLHEP::Hep3Vector(ds->position().x(),0.,CryoSealZ);

      CLHEP::Hep3Vector CryoSealOffset = CryoSealOffsetInMu2e - hallInfo.centerInMu2e();

      string const CryoSealMaterialName  = _config.getString("mbs.CryoSealMaterialName");

      VolumeInfo CryoSealInfo  = nestTubs("DSCryoSeal",
                                          CryoSealParams,
                                          findMaterialOrThrow(CryoSealMaterialName),
                                          0,
                                          CryoSealOffset,
                                          hallInfo,
                                          0,
                                          MBSisVisible,
                                          G4Colour::Green(),
                                          MBSisSolid,
                                          forceAuxEdgeVisible,
                                          placePV,
                                          doSurfaceCheck
                                          );


      if ( verbosityLevel > 0) {
        double zhl         = static_cast<G4Tubs*>(CryoSealInfo.solid)->GetZHalfLength();
        double CryoSealOffsetInMu2eZ = CryoSealOffsetInMu2e[CLHEP::Hep3Vector::Z];
        cout << __func__ << " CryoSeal     Z extent in Mu2e    : " <<
          CryoSealOffsetInMu2eZ - zhl << ", " << CryoSealOffsetInMu2eZ + zhl << endl;
      }

    }

    bool const hasEndPlug = _config.getBool("mbs.hasEndPlug", true);

    // now the endplug itself, the hollow part first

    if (hasEndPlug) {

      double const EndPlugTubeInnerRadius = _config.getDouble("mbs.EndPlugTubeInnerRadius");
      double const EndPlugTubeOuterRadius = _config.getDouble("mbs.EndPlugTubeOuterRadius");
      double const EndPlugTubeHLength     = _config.getDouble("mbs.EndPlugTubeHLength");

      TubsParams EndPlugTubeParams ( EndPlugTubeInnerRadius,
                                     EndPlugTubeOuterRadius,
                                     EndPlugTubeHLength );

      double const EndPlugTubeZ           = _config.getDouble("mbs.EndPlugTubeZ");

      if ( verbosityLevel > 0) {

        double EndPlugTubeDSZ = EndPlugTubeZ + EndPlugTubeHLength;
        double EndPlugTubeUSZ = EndPlugTubeZ - EndPlugTubeHLength;
        cout << __func__ << " EndPlugTubeDSZ : " << EndPlugTubeDSZ << endl;
        cout << __func__ << " EndPlugTubeUSZ : " << EndPlugTubeUSZ << endl;
        cout << __func__ << " EndPlugTubeZ   : " << EndPlugTubeZ << endl;

      }

      CLHEP::Hep3Vector EndPlugTubeOffsetInMu2e = CLHEP::Hep3Vector(ds->position().x(),0.,EndPlugTubeZ);

      CLHEP::Hep3Vector EndPlugTubeOffset = EndPlugTubeOffsetInMu2e - hallInfo.centerInMu2e();

      string const EndPlugTubeMaterialName  = _config.getString("mbs.EndPlugMaterialName");

      VolumeInfo EndPlugTubeInfo  = nestTubs("DSEndPlugTube",
                                             EndPlugTubeParams,
                                             findMaterialOrThrow(EndPlugTubeMaterialName),
                                             0,
                                             EndPlugTubeOffset,
                                             hallInfo,
                                             0,
                                             MBSisVisible,
                                             G4Colour::Gray(),
                                             MBSisSolid,
                                             forceAuxEdgeVisible,
                                             placePV,
                                             doSurfaceCheck
                                             );


      if ( verbosityLevel > 0) {
        double zhl         = static_cast<G4Tubs*>(EndPlugTubeInfo.solid)->GetZHalfLength();
        double EndPlugTubeOffsetInMu2eZ = EndPlugTubeOffsetInMu2e[CLHEP::Hep3Vector::Z];
        cout << __func__ << " EndPlugTube     Z extent in Mu2e    : " <<
          EndPlugTubeOffsetInMu2eZ - zhl << ", " << EndPlugTubeOffsetInMu2eZ + zhl << endl;
      }

      // the end plug end disk

      double const EndPlugDiskInnerRadius  = _config.getDouble("mbs.EndPlugDiskInnerRadius");
      double const EndPlugDiskOuterRadius  = _config.getDouble("mbs.EndPlugDiskOuterRadius");
      double const EndPlugDiskHLength      = _config.getDouble("mbs.EndPlugDiskHLength");

      TubsParams EndPlugDiskParams ( EndPlugDiskInnerRadius,
                                     EndPlugDiskOuterRadius,
                                     EndPlugDiskHLength );

      double EndPlugDiskZ = EndPlugTubeZ + EndPlugTubeHLength + EndPlugDiskHLength;

      if ( verbosityLevel > 0) {

        cout << __func__ << " EndPlugDiskZ  : " << EndPlugDiskZ << endl;

      }

      CLHEP::Hep3Vector EndPlugDiskOffsetInMu2e = CLHEP::Hep3Vector(ds->position().x(),0.,EndPlugDiskZ);

      CLHEP::Hep3Vector EndPlugDiskOffset  = EndPlugDiskOffsetInMu2e - hallInfo.centerInMu2e();

      string const EndPlugDiskMaterialName = _config.getString("mbs.EndPlugMaterialName");

      VolumeInfo EndPlugDiskInfo  = nestTubs("DSEndPlugDisk",
                                             EndPlugDiskParams,
                                             findMaterialOrThrow(EndPlugDiskMaterialName),
                                             0,
                                             EndPlugDiskOffset,
                                             hallInfo,
                                             0,
                                             MBSisVisible,
                                             G4Colour::Gray(),
                                             MBSisSolid,
                                             forceAuxEdgeVisible,
                                             placePV,
                                             doSurfaceCheck
                                             );


      if ( verbosityLevel > 0) {
        double zhl         = static_cast<G4Tubs*>(EndPlugDiskInfo.solid)->GetZHalfLength();
        double EndPlugDiskOffsetInMu2eZ = EndPlugDiskOffsetInMu2e[CLHEP::Hep3Vector::Z];
        cout << __func__ << " EndPlugDisk  Z extent in Mu2e    : " <<
          EndPlugDiskOffsetInMu2eZ - zhl << ", " << EndPlugDiskOffsetInMu2eZ + zhl << endl;
      }

    }

  } // end of constructMBS;

}
