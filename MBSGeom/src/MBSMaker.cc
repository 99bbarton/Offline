//
// Construct and return MBS
//
// $Id: MBSMaker.cc,v 1.4 2013/07/02 15:57:07 tassiell Exp $
// $Author: tassiell $
// $Date: 2013/07/02 15:57:07 $
//
// Original author KLG
//
// Notes
// see mu2e-doc-1351 for naming convetions etc...

// c++ includes
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// clhep includes
#include "CLHEP/Vector/ThreeVector.h"

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// Mu2e includes
#include "MBSGeom/inc/MBSMaker.hh"
#include "MBSGeom/inc/MBS.hh"

#include "ConfigTools/inc/SimpleConfig.hh"

using namespace std;

namespace mu2e {

  // Constructor that gets information from the config file instead of
  // from arguments.
  MBSMaker::MBSMaker(SimpleConfig const & _config,
                     double solenoidOffset)
  {
    // if( ! _config.getBool("hasMBS",false) ) return;

    // create an empty MBS
    _mbs = unique_ptr<MBS>(new MBS());

    // access its object through a reference

    MBS & mbs = *_mbs.get();

    parseConfig(_config);

    // now create the specific components


    mbs._zMax = _MBSCZ+_BSTSZ+_BSTSHLength;
    mbs._rMax = _SPBSLOuterRadius;
    if (mbs._rMax<_SPBSCOuterRadius) { mbs._rMax = _SPBSCOuterRadius; }
    if (mbs._rMax<_SPBSROuterRadius) { mbs._rMax = _SPBSROuterRadius; }
    if (mbs._rMax<_SPBSSup1OuterRadius) { mbs._rMax = _SPBSSup1OuterRadius; }
    if (mbs._rMax<_SPBSSup2OuterRadius) { mbs._rMax = _SPBSSup2OuterRadius; }
    mbs._rMin = *std::min_element(_CLV2InnerRadii.begin(), _CLV2InnerRadii.end());
    mbs._totLength = 2.0*_BSTSHLength;


    CLHEP::Hep3Vector _MBSMOffsetInMu2e  = CLHEP::Hep3Vector(-solenoidOffset,0.,_MBSCZ);
    mbs._originInMu2e = _MBSMOffsetInMu2e;

    std::vector<double> MBSMCornersZ, MBSMCornersInnRadii, MBSMCornersOutRadii;
    MBSMCornersZ.push_back(-_BSTSHLength);
    MBSMCornersInnRadii.push_back(mbs._rMin);
    MBSMCornersOutRadii.push_back(_BSTSOuterRadius);
    MBSMCornersZ.push_back(_SPBSLZ-_SPBSLHLength);
    MBSMCornersInnRadii.push_back(mbs._rMin);
    MBSMCornersOutRadii.push_back(_BSTSOuterRadius);
    MBSMCornersZ.push_back(_SPBSLZ-_SPBSLHLength);
    MBSMCornersInnRadii.push_back(mbs._rMin);
    MBSMCornersOutRadii.push_back(mbs._rMax);
    MBSMCornersZ.push_back(_SPBSRZ+_SPBSRHLength);
    MBSMCornersInnRadii.push_back(mbs._rMin);
    MBSMCornersOutRadii.push_back(mbs._rMax);
    MBSMCornersZ.push_back(_SPBSRZ+_SPBSRHLength);
    MBSMCornersInnRadii.push_back(mbs._rMin);
    MBSMCornersOutRadii.push_back(_BSTSOuterRadius);
    MBSMCornersZ.push_back(_BSTSHLength);
    MBSMCornersInnRadii.push_back(mbs._rMin);
    MBSMCornersOutRadii.push_back(_BSTSOuterRadius);

    mbs._pMBSMParams = std::unique_ptr<Polycone>
      (new Polycone(MBSMCornersZ,
                      MBSMCornersInnRadii,
                      MBSMCornersOutRadii,
                      _MBSMOffsetInMu2e,
                      "DSVacuum"));

    CLHEP::Hep3Vector _BSTSOffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_BSTSZ);

    mbs._pBSTSParams = std::unique_ptr<Tube>
      (new Tube(_BSTSMaterialName,
                _BSTSOffsetInMu2e,
                _BSTSInnerRadius,
                _BSTSOuterRadius,
                _BSTSHLength));

    CLHEP::Hep3Vector _SPBSSup1OffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_SPBSSup1Z);

    mbs._pSPBSSup1Params = std::unique_ptr<Tube>
      (new Tube(_SPBSSup1MaterialName,
                _SPBSSup1OffsetInMu2e,
                _SPBSSup1InnerRadius,
                _SPBSSup1OuterRadius,
                _SPBSSup1HLength));

    CLHEP::Hep3Vector _SPBSSup2OffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_SPBSSup2Z);

    mbs._pSPBSSup2Params = std::unique_ptr<Tube>
      (new Tube(_SPBSSup2MaterialName,
                _SPBSSup2OffsetInMu2e,
                _SPBSSup2InnerRadius,
                _SPBSSup2OuterRadius,
                _SPBSSup2HLength));

    CLHEP::Hep3Vector _SPBSLOffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_SPBSLZ);

    mbs._pSPBSLParams = std::unique_ptr<Tube>
      (new Tube(_SPBSLMaterialName,
                _SPBSLOffsetInMu2e,
                _SPBSLInnerRadius,
                _SPBSLOuterRadius,
                _SPBSLHLength));

    CLHEP::Hep3Vector _SPBSCOffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_SPBSCZ);

    mbs._pSPBSCParams = std::unique_ptr<Tube>
      (new Tube(_SPBSCMaterialName,
                _SPBSCOffsetInMu2e,
                _SPBSCInnerRadius,
                _SPBSCOuterRadius,
                _SPBSCHLength,
                _SPBSCminAngle,
                _SPBSCmaxAngle));

    CLHEP::Hep3Vector _SPBSROffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_SPBSRZ);

    mbs._pSPBSRParams = std::unique_ptr<Tube>
      (new Tube(_SPBSRMaterialName,
                _SPBSROffsetInMu2e,
                _SPBSRInnerRadius,
                _SPBSROuterRadius,
                _SPBSRHLength));

    CLHEP::Hep3Vector _BSTCOffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_BSTCZ);

    int nBSTCSurfs = _BSTCLengths.size();
    if (nBSTCSurfs != (int) _BSTCInnerRadii.size() ||  nBSTCSurfs != (int) _BSTCOuterRadii.size()) {
            throw cet::exception("GEOM")
              << "BSTC has different number of radii and lengths, check the geom config file. \n";
    }
    std::vector<double> BSTCCornersZ, BSTCCornersInnRadii, BSTCCornersOutRadii;
    double BSTCtotLength=0;
    for (std::vector<double>::iterator length_it=_BSTCLengths.begin(); length_it!=_BSTCLengths.end(); ++length_it) {
            BSTCtotLength+=*length_it;
    }
    double tmpBSTCPntZ=/*_BSTCZ*/-0.5*BSTCtotLength;
    for (int isurf=0; isurf<nBSTCSurfs; ++isurf) {
            BSTCCornersZ.push_back(tmpBSTCPntZ);
            BSTCCornersInnRadii.push_back(_BSTCInnerRadii.at(isurf));
            BSTCCornersOutRadii.push_back(_BSTCOuterRadii.at(isurf));
            tmpBSTCPntZ+=_BSTCLengths.at(isurf);
            BSTCCornersZ.push_back(tmpBSTCPntZ);
            BSTCCornersInnRadii.push_back(_BSTCInnerRadii.at(isurf));
            BSTCCornersOutRadii.push_back(_BSTCOuterRadii.at(isurf));
   }

    mbs._pBSTCParams = std::unique_ptr<Polycone>
      (new Polycone(BSTCCornersZ,
                      BSTCCornersInnRadii,
                      BSTCCornersOutRadii,
                      _BSTCOffsetInMu2e,
                      _BSTCMaterialName));

    CLHEP::Hep3Vector _BSBSOffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_BSBSZ);

    int nBSBSSurfs = _BSBSLengths.size();
    if (nBSBSSurfs != (int) _BSBSInnerRadii.size() ||  nBSBSSurfs != (int) _BSBSOuterRadii.size()) {
            throw cet::exception("GEOM")
              << "BSBS has different number of radii and lengths, check the geom config file. \n";
    }
    std::vector<double> BSBSCornersZ, BSBSCornersInnRadii, BSBSCornersOutRadii;
    double BSBStotLength=0;
    for (std::vector<double>::iterator length_it=_BSBSLengths.begin(); length_it!=_BSBSLengths.end(); ++length_it) {
            BSBStotLength+=*length_it;
    }
    double tmpBSBSPntZ=/*_BSBSZ*/-0.5*BSBStotLength;
    for (int isurf=0; isurf<nBSBSSurfs; ++isurf) {
            BSBSCornersZ.push_back(tmpBSBSPntZ);
            BSBSCornersInnRadii.push_back(_BSBSInnerRadii.at(isurf));
            BSBSCornersOutRadii.push_back(_BSBSOuterRadii.at(isurf));
            tmpBSBSPntZ+=_BSBSLengths.at(isurf);
            BSBSCornersZ.push_back(tmpBSBSPntZ);
            BSBSCornersInnRadii.push_back(_BSBSInnerRadii.at(isurf));
            BSBSCornersOutRadii.push_back(_BSBSOuterRadii.at(isurf));
    }

    mbs._pBSBSParams = std::unique_ptr<Polycone>
      (new Polycone(BSBSCornersZ,
                      BSBSCornersInnRadii,
                      BSBSCornersOutRadii,
                      _BSBSOffsetInMu2e,
                      _BSBSMaterialName));


    CLHEP::Hep3Vector _CLV2OffsetInMu2e  = _MBSMOffsetInMu2e + CLHEP::Hep3Vector(0.0,0.,_CLV2Z);

    int nCLV2Surfs = _CLV2Lengths.size();
    if (nCLV2Surfs != (int) _CLV2InnerRadii.size() ||  nCLV2Surfs != (int) _CLV2OuterRadii.size()) {
            throw cet::exception("GEOM")
              << "CLV2 has different number of radii and lengths, check the geom config file. \n";
    }
    std::vector<double> CLV2CornersZ, CLV2CornersInnRadii, CLV2CornersOutRadii;
    double CLV2totLength=0;
    for (std::vector<double>::iterator length_it=_CLV2Lengths.begin(); length_it!=_CLV2Lengths.end(); ++length_it) {
            CLV2totLength+=*length_it;
    }
    double tmpCLV2PntZ=/*_CLV2Z*/-0.5*CLV2totLength;
    for (int isurf=0; isurf<nCLV2Surfs; ++isurf) {
            CLV2CornersZ.push_back(tmpCLV2PntZ);
            CLV2CornersInnRadii.push_back(_CLV2InnerRadii.at(isurf));
            CLV2CornersOutRadii.push_back(_CLV2OuterRadii.at(isurf));
            tmpCLV2PntZ+=_CLV2Lengths.at(isurf);
            CLV2CornersZ.push_back(tmpCLV2PntZ);
            CLV2CornersInnRadii.push_back(_CLV2InnerRadii.at(isurf));
            CLV2CornersOutRadii.push_back(_CLV2OuterRadii.at(isurf));
    }

    mbs._pCLV2Params = std::unique_ptr<Polycone>
      (new Polycone(CLV2CornersZ,
                      CLV2CornersInnRadii,
                      CLV2CornersOutRadii,
                      _CLV2OffsetInMu2e,
                      _CLV2MaterialName));

  }

  void MBSMaker::parseConfig( SimpleConfig const & _config ){

    _verbosityLevel       = _config.getInt("mbs.verbosityLevel");

    _MBSCZ                = _config.getDouble("mbs.MBSCZ");

    _BSTSInnerRadius      = _config.getDouble("mbs.BSTSInnerRadius");
    _BSTSOuterRadius      = _config.getDouble("mbs.BSTSOuterRadius");
    _BSTSHLength          = _config.getDouble("mbs.BSTSHLength");
    _BSTSMaterialName     = _config.getString("mbs.BSTSMaterialName");
    _BSTSZ                = _config.getDouble("mbs.BSTSZrelCntr");

    _SPBSSup1InnerRadius  = _config.getDouble("mbs.SPBSSup1InnerRadius");
    _SPBSSup1OuterRadius  = _config.getDouble("mbs.SPBSSup1OuterRadius");
    _SPBSSup1HLength      = _config.getDouble("mbs.SPBSSup1HLength");
    _SPBSSup1MaterialName = _config.getString("mbs.SPBSSup1MaterialName");
    _SPBSSup1Z            = _config.getDouble("mbs.SPBSSup1ZrelCntr");
    _SPBSSup2InnerRadius  = _config.getDouble("mbs.SPBSSup2InnerRadius");
    _SPBSSup2OuterRadius  = _config.getDouble("mbs.SPBSSup2OuterRadius");
    _SPBSSup2HLength      = _config.getDouble("mbs.SPBSSup2HLength");
    _SPBSSup2MaterialName = _config.getString("mbs.SPBSSup2MaterialName");
    _SPBSSup2Z            = _config.getDouble("mbs.SPBSSup2ZrelCntr");

    _SPBSLInnerRadius     = _config.getDouble("mbs.SPBSLInnerRadius");
    _SPBSLOuterRadius     = _config.getDouble("mbs.SPBSLOuterRadius");
    _SPBSLHLength         = _config.getDouble("mbs.SPBSLHLength");
    _SPBSLMaterialName    = _config.getString("mbs.SPBSLMaterialName");
    _SPBSLZ               = _config.getDouble("mbs.SPBLZrelCntr");
    _SPBSCInnerRadius     = _config.getDouble("mbs.SPBSCInnerRadius");
    _SPBSCOuterRadius     = _config.getDouble("mbs.SPBSCOuterRadius");
    _SPBSCHLength         = _config.getDouble("mbs.SPBSCHLength");
    _SPBSCminAngle        = _config.getDouble("mbs.SPBSCminAngle")*CLHEP::deg;
    _SPBSCmaxAngle        = _config.getDouble("mbs.SPBSCmaxAngle")*CLHEP::deg;
    _SPBSCMaterialName    = _config.getString("mbs.SPBSCMaterialName");
    _SPBSCZ               = _config.getDouble("mbs.SPBCZrelCntr");
    _SPBSRInnerRadius     = _config.getDouble("mbs.SPBSRInnerRadius");
    _SPBSROuterRadius     = _config.getDouble("mbs.SPBSROuterRadius");
    _SPBSRHLength         = _config.getDouble("mbs.SPBSRHLength");
    _SPBSRMaterialName    = _config.getString("mbs.SPBSRMaterialName");
    _SPBSRZ               = _config.getDouble("mbs.SPBRZrelCntr");

    _config.getVectorDouble("mbs.BSTCInnerRadii",_BSTCInnerRadii);
    _config.getVectorDouble("mbs.BSTCOuterRadii",_BSTCOuterRadii);
    _config.getVectorDouble("mbs.BSTCLengths",_BSTCLengths);
    _BSTCMaterialName     = _config.getString("mbs.BSTCMaterialName");
    _BSTCZ                = _config.getDouble("mbs.BSTCZrelCntr");
    _config.getVectorDouble("mbs.BSBSInnerRadii",_BSBSInnerRadii);
    _config.getVectorDouble("mbs.BSBSOuterRadii",_BSBSOuterRadii);
    _config.getVectorDouble("mbs.BSBSLengths",_BSBSLengths);
    _BSBSMaterialName     = _config.getString("mbs.BSBSMaterialName");
    _BSBSZ                = _config.getDouble("mbs.BSBSZrelCntr");
    _config.getVectorDouble("mbs.CLV2InnerRadii",_CLV2InnerRadii);
    _config.getVectorDouble("mbs.CLV2OuterRadii",_CLV2OuterRadii);
    _config.getVectorDouble("mbs.CLV2Lengths",_CLV2Lengths);
    _CLV2MaterialName     = _config.getString("mbs.CLV2MaterialName");
    _CLV2Z                = _config.getDouble("mbs.CLV2ZrelCntr");

  }

} // namespace mu2e
