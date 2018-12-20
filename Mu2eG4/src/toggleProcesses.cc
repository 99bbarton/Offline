//
// Class Description:
//
// Function that handles the switching on and off of G4 processes.  This
// is handled through the configuration files and includes the following
// commands:
//
// g4.noDecay - turns off decays of specified particles
// muMinusConversionAtRest.do - turns on the at rest G4 process
// MuonMinusConversionAtRest and turns off MuonMinusCaptureAtRest
//
// $Id: toggleProcesses.cc,v 1.9 2013/03/26 19:39:33 genser Exp $
// $Author: genser $
// $Date: 2013/03/26 19:39:33 $
//
//-----------------------------------------------------------------------------


// C++ includes.
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// Geant4 includes
#include "G4ParticleTable.hh"
#include "G4VRestProcess.hh"
#include "G4ProcessManager.hh"

#include "G4EmParameters.hh"

#include "G4MuonMinus.hh"
#include "G4MuonMinusCapture.hh"
#include "G4MuMinusCapturePrecompound.hh"

#include "G4PhysicsListHelper.hh"
#include "G4GammaConversionToMuons.hh"
#include "G4AnnihiToMuPair.hh"
#include "G4eeToHadrons.hh"

// Framework includes
#include "cetlib_except/exception.h"

// Mu2e includes
#include "Mu2eG4/inc/Mu2eRecorderProcess.hh"
#include "fhiclcpp/ParameterSet.h"

using namespace std;

namespace mu2e{

  //================================================================
  void switchDecayOff(const std::vector<int>& plist) {

    G4ParticleTable *theParticleTable = G4ParticleTable::GetParticleTable();
    for( size_t i=0; i<plist.size(); ++i ) {
      int pdg = plist[i];
      G4ParticleDefinition* particle = theParticleTable->FindParticle(pdg);
      if( particle==nullptr ) {
        cout << "SwitchDecayOff: cannot find particle pdgId=" << pdg << endl;
      } else {
        G4ProcessManager* pmanager = particle->GetProcessManager();
        G4ProcessVector * pVector  = pmanager->GetProcessList();
        G4VProcess *decayProcess = nullptr;
        for( G4int j=0; j<pmanager->GetProcessListLength(); ++j ) {
          if( (*pVector)[j]->GetProcessName() == "Decay" ) {
            decayProcess = (*pVector)[j];
            break;
          }
        }
        if( decayProcess==nullptr ) {
          cout << "SwitchDecayOff: cannot find decay process for particle pdgId=" << pdg
               << " (" << particle->GetParticleName() << ")" << endl;
        } else {
          pmanager->RemoveProcess(decayProcess);
          cout << "SwitchDecayOff: decay process is removed for particle pdgId=" << pdg
               << " (" << particle->GetParticleName() << ")" << endl;
        }
        cout << "SwitchDecayOff: list of processes defined for particle pdgId=" << pdg
             << " (" << particle->GetParticleName() << "):" << endl;
        for( G4int j=0; j<pmanager->GetProcessListLength(); ++j )
          cout << (*pVector)[j]->GetProcessName() << endl;
      }
    }

  }

  void switchCaptureDModel(std::string cDModel) {

    // change muMinusCaptureAtRest deexcitation model to muMinusNuclearCapture
    // this is limited to muon minus only

    if (std::string("muMinusNuclearCapture")!=cDModel) {
      return;
    }

    G4ParticleDefinition* particle = G4MuonMinus::MuonMinus();
    if( particle==0 ) {
      cout << "SwitchDecayOff: cannot find MuonMinus " << endl;
    } else {
      G4ProcessManager* pmanager = particle->GetProcessManager();
      G4ProcessVector * pVector  = pmanager->GetProcessList();
      G4VProcess* muCapProcess = nullptr;
      for( G4int j=0; j<pmanager->GetProcessListLength(); ++j ) {
        if( (*pVector)[j]->GetProcessName() == "muMinusCaptureAtRest" ) {
          muCapProcess = (*pVector)[j];
          break;
        }
      }
      if( muCapProcess==0 ) {
        cout << __func__ << " : cannot find muMinusCaptureAtRest process for "
             << particle->GetParticleName() << endl;
      } else {
        pmanager->RemoveProcess(muCapProcess);
        cout << __func__ << " : muMinusCaptureAtRest process is removed for " 
             << particle->GetParticleName() << endl;
        G4MuonMinusCapture* muProcess = new G4MuonMinusCapture(new G4MuMinusCapturePrecompound());
        G4PhysicsListHelper* ph = G4PhysicsListHelper::GetPhysicsListHelper();
        ph->RegisterProcess(muProcess, particle);
        //        pmanager->AddRestProcess( muProcess );
        cout << __func__ << " : added muMinusCaptureAtRest with muMinusNuclearCapture for " 
             << particle->GetParticleName() << endl;
        }
      cout  << __func__ << " : list of processes defined for " 
            << particle->GetParticleName() << " :" << endl;
      for( G4int j=0; j<pmanager->GetProcessListLength(); ++j ) {
        cout << (*pVector)[j]->GetProcessName() << endl;
      }
    }
  }

  void switchCaptureDModel(const fhicl::ParameterSet& pset) { 
    std::string cDModel= pset.get<std::string>("physics.captureDModel");
    switchCaptureDModel(cDModel);
  }

  void switchDecayOff(const fhicl::ParameterSet& pset) {
    std::vector<int> plist = pset.get<std::vector<int> >("physics.noDecay");
    switchDecayOff(plist);
  }


  //================================================================
  void addUserProcesses(const std::vector<std::string>& plist) {

    // search for process names, we assume process implies specific particle, for now

    // for ( const auto& elem : plist) {
    //   std::cout << __func__ << " " << elem << std::endl;
    // }

    G4PhysicsListHelper* ph = G4PhysicsListHelper::GetPhysicsListHelper();

    G4ParticleDefinition* gamma = G4Gamma::Gamma();
    G4ParticleDefinition* positron = G4Positron::Positron();

    // consider replaceing implementation with one using MCDataProducts/inc/ProcessCode

    std::vector<std::string>::const_iterator ipos;

    ipos = std::find(plist.begin(), plist.end(), std::string("GammaToMuPair"));
    bool gmumu = ( ipos != plist.end() );

    ipos = std::find(plist.begin(), plist.end(), std::string("AnnihiToMuPair"));
    bool pmumu = ( ipos != plist.end() );

    ipos = std::find(plist.begin(), plist.end(), std::string("ee2hadr"));
    bool phad = ( ipos != plist.end() );

    // processes are owned by process managers
    if(gmumu) {
      std::cout << __func__ << " adding GammaToMuPair process to gamma" << std::endl;
      G4GammaConversionToMuons* theGammaToMuMu = new G4GammaConversionToMuons();
      ph->RegisterProcess(theGammaToMuMu, gamma);
    }
    if(pmumu) {
      std::cout << __func__ << " adding AnnihiToMuPair process to positron" << std::endl;
      G4AnnihiToMuPair* thePosiToMuMu = new G4AnnihiToMuPair();
      ph->RegisterProcess(thePosiToMuMu, positron);
    }
    if(phad) {
      std::cout << __func__ << " adding ee2hadr process to positron" << std::endl;
      G4eeToHadrons* thePosiToHadrons = new G4eeToHadrons();
      ph->RegisterProcess(thePosiToHadrons, positron);
    }

    // special process to look at the track before post step interaction
    // fixme: make it a function guarded by pset flag and access to the stepping verbosity level
    Mu2eRecorderProcess* rmp = new Mu2eRecorderProcess();
    //    rmp->SetVerboseLevel(1);
    G4ParticleTable* ptable = G4ParticleTable::GetParticleTable();
    G4ParticleTable::G4PTblDicIterator* iter = ptable->GetIterator();
    iter->reset();
    while( (*iter)() ){
      G4ParticleDefinition* particle = iter->value();
      G4ProcessManager* pmanager     = particle->GetProcessManager();
      // The process manager takes ownership of the process
      // ph->RegisterProcess(rmp, proton); // RegisterProcess only works for known geant4 processes
      pmanager->AddContinuousProcess(rmp);
      pmanager->SetProcessOrderingToLast(rmp, idxAlongStep);
    }
  }

  void addUserProcesses(const fhicl::ParameterSet& pset) {
    std::vector<std::string> plist = pset.get<std::vector<std::string> >("physics.addProcesses");
    addUserProcesses(plist);
  }

  void setMuHadLateralDisplacement(const fhicl::ParameterSet& pset) {
    if (pset.get<bool>("physics.setMuHadLateralDisplacement",false)) {
      G4EmParameters* params = G4EmParameters::Instance();
      params->SetMuHadLateralDisplacement(true);
    }
  }



  //================================================================
} // end namespace mu2e
