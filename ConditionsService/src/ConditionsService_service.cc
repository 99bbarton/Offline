//
// Primitive conditions data service.
// It does not yet do validty checking.
//
// $Id: ConditionsService_service.cc,v 1.14 2013/03/15 15:52:03 kutschke Exp $
// $Author: kutschke $
// $Date: 2013/03/15 15:52:03 $
//
// Original author Rob Kutschke
//

// C++ include files
#include <iostream>
#include <typeinfo>

// Framework include files
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e include files
#include "ConditionsService/inc/ConditionsService.hh"

// Calibration entities.
// Would like to break the coupling to these.
#include "ConditionsService/inc/AcceleratorParams.hh"
#include "ConditionsService/inc/DAQParams.hh"
#include "ConditionsService/inc/CalorimeterCalibrations.hh"
#include "ConditionsService/inc/TrackerCalibrationsI.hh"
#include "ConditionsService/inc/ExtMonFNALConditions.hh"

using namespace std;

namespace mu2e {

  ConditionsService::ConditionsService(fhicl::ParameterSet const& pset,
                                       art::ActivityRegistry&iRegistry) :
    _conditionsFile(       pset.get<std::string>("conditionsfile","conditions.txt")),
    _allowReplacement(     pset.get<bool>       ("allowReplacement",      true)),
    _messageOnReplacement( pset.get<bool>       ("messageOnReplacement",  false)),
    _messageOnDefault(     pset.get<bool>       ("messageOnDefault",      false)),
    _configStatsVerbosity( pset.get<int>        ("configStatsVerbosity",  0)),
    _printConfig(          pset.get<bool>       ("printConfig",           false)),

    // FIXME: should be initialized in beginRun, not c'tor.
    _config(_conditionsFile, _allowReplacement, _messageOnReplacement, _messageOnDefault),
    _entities(),
    _run_count()
  {
    iRegistry.watchPreBeginRun(this, &ConditionsService::preBeginRun);
    iRegistry.watchPostEndJob (this, &ConditionsService::postEndJob   );
  }

  ConditionsService::~ConditionsService(){
  }

  // This template can be defined here because this is a private method which is only
  // used by the code below in the same file.
  template <typename ENTITY>
  void ConditionsService::addEntity(std::unique_ptr<ENTITY> d)
  {
    if(_entities.find(typeid(ENTITY).name())!=_entities.end())
      throw cet::exception("GEOM") << "failed to install conditions entity with type name "
                                   << typeid(ENTITY).name() << "\n";

    ConditionsEntityPtr ptr(d.release());
    _entities[typeid(ENTITY).name()] = ptr;
  }

  void
  ConditionsService::preBeginRun(art::Run const &) {

    if(++_run_count > 1) {
      mf::LogPrint("CONDITIONS") << "This test version does not change geometry on run boundaries.";
      return;
    }

    cout << "Conditions input file is: " << _conditionsFile << "\n";

    if ( _printConfig ){ _config.print(cout, "Conditions: "); }

    checkConsistency();

    // Can we break the coupling to the entities?
    std::unique_ptr<AcceleratorParams>  acctmp(new AcceleratorParams(_config));
    const AcceleratorParams& accp = *acctmp;
    addEntity( acctmp );
    addEntity( std::unique_ptr<DAQParams>          ( new DAQParams          (_config)) );
    addEntity( std::unique_ptr<CalorimeterCalibrations>( new CalorimeterCalibrations(_config)) );
    if (_config.getBool("isITrackerCond",false)) {
            addEntity( std::unique_ptr<TrackerCalibrations>( new TrackerCalibrationsI(_config)) );
    } else {
            addEntity( std::unique_ptr<TrackerCalibrations>( new TrackerCalibrations(_config)) );
    }
    addEntity( std::unique_ptr<ExtMonFNALConditions>( new ExtMonFNALConditions(accp, _config)) );
  }

  // Check that the configuration is self consistent.
  void ConditionsService::checkConsistency(){
  }

  // Called after all modules have completed their end of job.
  void ConditionsService::postEndJob(){
    _config.printAllSummaries ( cout, _configStatsVerbosity, "Conditions: " );
  }

} // end namespace mu2e

DEFINE_ART_SERVICE(mu2e::ConditionsService);
