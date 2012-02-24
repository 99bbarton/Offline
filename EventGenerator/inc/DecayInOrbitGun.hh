#ifndef EventGenerator_DecayInOrbitGun_hh
#define EventGenerator_DecayInOrbitGun_hh
//
// Generate some number of DIO electrons.
//
// $Id: DecayInOrbitGun.hh,v 1.27 2012/02/24 20:05:52 onoratog Exp $
// $Author: onoratog $
// $Date: 2012/02/24 20:05:52 $
//
//

// C++ includes
#include <memory>

// Framework includes
#include "art/Framework/Principal/Run.h"

// Mu2e includes
#include "EventGenerator/inc/FoilParticleGenerator.hh"
#include "EventGenerator/inc/GeneratorBase.hh"
#include "Mu2eUtilities/inc/ReadDIOSpectrum.hh"
#include "Mu2eUtilities/inc/RandomUnitSphere.hh"

// CLHEP includes
#include "CLHEP/Random/RandPoissonQ.h"
#include "CLHEP/Random/RandGeneral.h"
#include "CLHEP/Random/RandFlat.h"

// Forward declarations outside of mu2e namespace.
class TH1D;
namespace art {
  class Run;
}

namespace mu2e {

  // Forward declarations
  class SimpleConfig;

  class DecayInOrbitGun: public GeneratorBase{

  public:
    DecayInOrbitGun( art::Run& run, const SimpleConfig& config );
    virtual ~DecayInOrbitGun();

    virtual void generate( GenParticleCollection&  );

  private:

    // Mean number of dio electrons to generate in each event.
    // If positive, use this as the mean of a Poisson distribution.
    // If negative, generate exactly std::abs(mean) every time.
    double _mean;

    // Limits on the energy range to be generated.
    // Number of bins in the binned representation of the energy spectrum.
    double _elow;
    double _ehi;
    int    _nbins;

    // Class object to generate position and time of the particle
    std::auto_ptr<FoilParticleGenerator> _fGenerator;
    std::auto_ptr<ReadDIOSpectrum> _randEnergy;

    // Limits on the generated direction.
    double _czmin;
    double _czmax;
    double _phimin;
    double _phimax;

    bool _pStodSDelay;
    bool _pPulseDelay;
    double _pPulseShift;

    // Activate the folding procedure on generation time. Default is on
    bool _timeFolding;

    // Limits on the generated time.
    double _tmin;
    double _tmax;

    double _p; //particle momentum

    double _mass; //electron mass
    double _mumass; //muon mass needed to pass as paramenter to ReadDIOSpectrum constructor

    // Histogram control.
    bool _doHistograms;

    // Resolution of the energy spectrum (0.1 default)
    double _spectrumResolution;

    // Kind of spectrum to be used
    std::string _energySpectrum;

    // End: parameters that can be configured from the config file.

    // Random number generators.
    CLHEP::RandGeneral _randSimpleEnergy;
    CLHEP::RandFlat _randFlatEnergy;
    CLHEP::RandPoissonQ _randPoissonQ;
    RandomUnitSphere _randomUnitSphere;

    std::string _stFname;

    int _nToSkip;

    // Diagnostic histograms.
    TH1D* _hMultiplicity;
    TH1D* _hEElec;
    TH1D* _hEElecZ;
    TH1D* _hradius;
    TH1D* _hzPosition;
    TH1D* _hcz;
    TH1D* _hphi;
    TH1D* _ht;
    TH1D* _hmudelay;
    TH1D* _hpulsedelay;

    GenId::enum_type _dioGenId;

    //Functions used to calculate energy spectrum of the electron
    std::vector<double> binnedEnergySpectrum();
    double energySpectrum( double e );

  };

} // end namespace mu2e,

#endif /* EventGenerator_DecayInOrbitGun_hh */
