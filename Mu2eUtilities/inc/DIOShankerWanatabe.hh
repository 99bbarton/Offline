#ifndef DIOSHANKERWANATABE_HH
#define DIOSHANKERWANATABE_HH
//
// Generate a momentum for the DIO electrons, using Wanatabe
// data, merged to Shanker's formula near the endpoint
//
// $Id: DIOShankerWanatabe.hh,v 1.5 2011/05/17 15:36:01 greenc Exp $
// $Author: greenc $
// $Date: 2011/05/17 15:36:01 $
//
// 

// C++ incldues
#include <vector>

// Framework includes
#include "art/Framework/Core/RandomNumberGeneratorService.h"

// Mu2e includes
#include "Mu2eUtilities/inc/DIOBase.hh"

//CLHEP includes
#include "CLHEP/Random/RandGeneral.h"

namespace mu2e {

  class DIOShankerWanatabe: public DIOBase {
    
  public:

    DIOShankerWanatabe(int atomicZ, double emin, double emax, double spectRes,
                       art::RandomNumberGeneratorService::base_engine_t & engine);
    
    ~DIOShankerWanatabe();
    
    double fire();

  private:

    int _Znum;

    double _emin, _emax, _res;

    int calculateNBins();

    int _nBinsSpectrum;

    CLHEP::RandGeneral _randEnergy;

    std::vector<double> shWaSpectrum();

  };

} // end of namespace mu2e

#endif

