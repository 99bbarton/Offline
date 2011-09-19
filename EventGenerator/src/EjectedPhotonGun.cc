//
//
// Simulate the photons coming from the stopping target when muons are captured
// by an Al nucleus.  
// //
// $Id: EjectedPhotonGun.cc,v 1.4 2011/09/19 17:32:44 onoratog Exp $
// $Author: onoratog $
// $Date: 2011/09/19 17:32:44 $
//
// Original author Gianni Onorato
//
//

// C++ includes.
#include <iostream>

// Framework includes
#include "art/Framework/Core/Run.h"
#include "art/Framework/Core/TFileDirectory.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e includes
#include "ConditionsService/inc/AcceleratorParams.hh"
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "ConditionsService/inc/DAQParams.hh"
#include "ConditionsService/inc/ParticleDataTable.hh"
#include "EventGenerator/inc/EjectedPhotonGun.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "MCDataProducts/inc/PDGCode.hh"
#include "Mu2eUtilities/inc/SimpleConfig.hh"
#include "Mu2eUtilities/inc/safeSqrt.hh"
#include "TargetGeom/inc/Target.hh"
#include "TargetGeom/inc/zBinningForFoils.hh"

// Other external includes.
#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Units/PhysicalConstants.h"

//ROOT Includes
#include "TH1D.h"
#include "TH2D.h"
#include "TMath.h"

using namespace std;

namespace mu2e {

  EjectedPhotonGun::EjectedPhotonGun( art::Run& run, const SimpleConfig& config ):

    // Base class.
    GeneratorBase(),

    // Configurable parameters
    _mean(config.getDouble("ejectedPhotonGun.mean",1.)),
    _elow(config.getDouble("ejectedPhotonGun.elow",0.)),
    _ehi(config.getDouble("ejectedPhotonGun.ehi",7.)),
    _czmin(config.getDouble("ejectedPhotonGun.czmin",  -1.)),
    _czmax(config.getDouble("ejectedPhotonGun.czmax",  1.)),
    _phimin(config.getDouble("ejectedPhotonGun.phimin", 0. )),
    _phimax(config.getDouble("ejectedPhotonGun.phimax", CLHEP::twopi )),
    _doHistograms(config.getBool("ejectedPhotonGun.doHistograms",true)),
    _PStoDSDelay(config.getBool("ejectedPhotonGun.PStoDSDelay", false)),
    _pPulseDelay(config.getBool("ejectedPhotonGun.pPulseDelay", false)),
    // Initialize random number distributions; getEngine comes from the base class.
    _randPoissonQ( getEngine(), std::abs(_mean) ),
    _randomUnitSphere ( getEngine(), _czmin, _czmax, _phimin, _phimax ),
    _flatmomentum ( getEngine() ),
    _STfname(config.getString("FoilParticleGenerator.STfilename","ExampleDataFiles/StoppedMuons/stoppedMuons_02.txt")),
    _nToSkip (config.getInt("ejectedPhotonGun.nToSkip",0)),

    // Histogram pointers
    _hMultiplicity(0),
    _hKE(0),
    _hKEZoom(0),
    _hMomentumMeV(0),
    _hradius(0),
    _hzPosition(0),
    _hcz(0),
    _hphi(0),
    _htime(0),
    _hmudelay(0),
    _hpulsedelay(0),
    _hyx(0),
    _hrz(0){


    // About the ConditionsService:
    // The argument to the constructor is ignored for now.  It will be a
    // data base key.  There is a second argument that I have let take its
    // default value of "current"; it will be used to specify a version number.
    ConditionsHandle<AcceleratorParams> accPar("ignored");
    ConditionsHandle<DAQParams>         daqPar("ignored");
    ConditionsHandle<ParticleDataTable> pdt("ignored");

   
    // Default values for the start and end of the live window.
    // Can be overriden by the run-time config; see below.
    _tmin = daqPar->t0;
    _tmax = accPar->deBuncherPeriod;

    _tmin = config.getDouble("ejectedPhotonGun.tmin",  _tmin );
    _tmax = config.getDouble("ejectedPhotonGun.tmax",  _tmax );

    // Book histograms.
    if ( _doHistograms ){
      GeomHandle<Target> target;
      Binning bins2 = zBinningForFoils(*target,3);

      art::ServiceHandle<art::TFileService> tfs;
      art::TFileDirectory tfdir  = tfs->mkdir( "EjectedPhotonGun" );
      _hMultiplicity = tfdir.make<TH1D>( "hMultiplicity", "Photon Multiplicity",                     20,     0,     20  );
      _hKE           = tfdir.make<TH1D>( "hKE",           "Photon Energy;(MeV)",                     50, _elow,   _ehi  );
      _hMomentumMeV  = tfdir.make<TH1D>( "hMomentumMeV",  "Photon Momentum;(MeV)",                   50, _elow,   _ehi  );
      _hKEZoom       = tfdir.make<TH1D>( "hEZoom",        "Photon Energy (zoom);(MeV)",             200, _elow,   _ehi  );
      _hradius       = tfdir.make<TH1D>( "hradius",       "Photon Radius (Tracker Coord);(mm)",     150,     0.,   150. );
      _hzPosition    = tfdir.make<TH1D>( "hzPosition",    "Photon z Position (Tracker Coord);(mm)", 200, -6600., -5600. );
      _hcz           = tfdir.make<TH1D>( "hcz",           "Photon cos(theta)",                      100,    -1.,     1. );
      _hphi          = tfdir.make<TH1D>( "hphi",          "Photon azimuth;(radians)",               100,  -M_PI,  M_PI  );
      _htime         = tfdir.make<TH1D>( "htime",         "Photon time;(ns)",                       210,  -200.,  3000. );
      _hmudelay      = tfdir.make<TH1D>( "hmudelay",      "Production delay due to muons arriving at ST;(ns)", 300, 0., 2000. );
      _hpulsedelay   = tfdir.make<TH1D>( "hpdelay",       "Production delay due to the proton pulse;(ns)",      60, 0.,  300. );
      _hyx           = tfdir.make<TH2D>( "hxyPos",         "Ejected photons (x,y) at Production;(mm);(mm)",
                                         60,  -120., 120., 60, -120., 120. );
      _hrz           = tfdir.make<TH2D>( "hrzPos",         "Ejected photonsd (z,r) at Production;(mm);(mm)",
                                         bins2.nbins(), bins2.low(), bins2.high(), 60, 0., 120. );

    }

    _fGenerator = auto_ptr<FoilParticleGenerator>(new FoilParticleGenerator( getEngine(), _tmin, _tmax,
                                                                             FoilParticleGenerator::muonFileInputFoil,
                                                                             FoilParticleGenerator::muonFileInputPos,
                                                                             FoilParticleGenerator::negExp,
                                                                             _PStoDSDelay,
                                                                             _pPulseDelay,
									     _STfname,
                                                                             _nToSkip));
  }

  EjectedPhotonGun::~EjectedPhotonGun(){
  }

  void EjectedPhotonGun::generate( GenParticleCollection& genParts ){

    // Choose the number of protons to generate this event.
    long n = _mean < 0 ? static_cast<long>(-_mean): _randPoissonQ.fire();
      if ( _doHistograms ) {
        _hMultiplicity->Fill(n);
      }

      //Loop over particles to generate

      for (int i=0; i<n; ++i) {

        //Pick up position and momentum
        CLHEP::Hep3Vector pos(0,0,0);
        double time;
        _fGenerator->generatePositionAndTime(pos, time);

        //Pick up energy
        double e = _elow + _flatmomentum.fire() * ( _ehi - _elow );

        //Pick up momentum vector

        _p = e;
        CLHEP::Hep3Vector p3 = _randomUnitSphere.fire(_p);

        //Set Four-momentum
        CLHEP::HepLorentzVector mom(0,0,0,0);
        mom.setPx( p3.x() );
        mom.setPy( p3.y() );
        mom.setPz( p3.z() );
        mom.setE( e );

        // Add the particle to  the list.
        genParts.push_back( GenParticle(PDGCode::gamma, GenId::ejectedPhotonGun, pos, mom, time));

        // Fill histograms.
        if ( _doHistograms) {
          double r(pos.perp());
          _hKE         ->Fill( e );
          _hKEZoom     ->Fill( e );
          _hMomentumMeV->Fill( _p );
          _hradius     ->Fill( r );
          _hzPosition  ->Fill( pos.z() );
          _hcz         ->Fill( mom.cosTheta() );
          _hphi        ->Fill( mom.phi() );
          _htime       ->Fill( time );
          _hmudelay    ->Fill(_fGenerator->muDelay());
          _hpulsedelay ->Fill(_fGenerator->pulseDelay());
          _hyx         ->Fill( pos.x(), pos.y() );
          _hrz         ->Fill( pos.z(), r );
        }
      } // end of loop on particles

  } // end generate

} // namespace mu2e
