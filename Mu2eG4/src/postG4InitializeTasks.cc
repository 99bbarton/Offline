//
// Steering routine to G4 call customization routines that must be called
// only after the call to Mu2eG4RunManager::Initialize.
//
// Do not put G4 code in this steering routine.
//
// $Id: postG4InitializeTasks.cc,v 1.4 2012/07/15 22:06:17 kutschke Exp $
// $Author: kutschke $
// $Date: 2012/07/15 22:06:17 $
//

#include "Mu2eG4/inc/postG4InitializeTasks.hh"

#include "fhiclcpp/ParameterSet.h"

#include "Mu2eG4/inc/customizeChargedPionDecay.hh"
#include "Mu2eG4/inc/toggleProcesses.hh"
#include "Mu2eG4/inc/setMinimumRangeCut.hh"
#include "Mu2eG4/inc/setParticleCut.hh"

#include "G4VUserPhysicsList.hh"

namespace mu2e{

  void postG4InitializeTasks(const fhicl::ParameterSet& pset, G4VUserPhysicsList* pL) {

    // G4 does not include pi+ -> e+ nu + cc. Fix that in one of several ways.
    customizeChargedPionDecay(pset);

    // Switch off the decay of some particles
    switchDecayOff(pset);

    // Set the range cut for e+,e-,gamma (& proton but see below)
    setMinimumRangeCut(pset, pL);

    // set the proton production cut which is special
    std::string pName("proton");
    setParticleCut(pset, pName, pL);

    // add processes if requested
    addUserProcesses(pset);

    // swap Bertini Cascade with Precompound model in G4MuonMinusCapture
    switchCaptureDModel(pset);

    // use more accurate boundary crossing algorithm for muons and hadrons
    setMuHadLateralDisplacement(pset);

  }

}  // end namespace mu2e
