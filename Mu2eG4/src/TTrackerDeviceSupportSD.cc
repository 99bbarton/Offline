//
// Define a sensitive detector for TTrackerDeviceSupport
//
// $Id: TTrackerDeviceSupportSD.cc,v 1.4 2012/07/15 22:06:17 kutschke Exp $
// $Author: kutschke $
// $Date: 2012/07/15 22:06:17 $
//
// Original author KLG
//

#include <cstdio>

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// Mu2e includes
#include "Mu2eG4/inc/TTrackerDeviceSupportSD.hh"
#include "Mu2eG4/inc/PhysicsProcessInfo.hh"
#include "ConfigTools/inc/SimpleConfig.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/WorldG4.hh"
#include "TTrackerGeom/inc/TTracker.hh"

// G4 includes
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4ios.hh"

using namespace std;

namespace mu2e {

  TTrackerDeviceSupportSD::TTrackerDeviceSupportSD(G4String name, SimpleConfig const & config ):
    Mu2eSensitiveDetector(name,config)
  {

    SetVerboseLevel(config.getInt("ttracker.verbosityLevel",0));

   art::ServiceHandle<GeometryService> geom;

    if ( !geom->hasElement<TTracker>() ) {
      throw cet::exception("GEOM")
        << "Expected T Tracker but did not find it.\n";
    } 
    else {
      _TrackerVersion = config.getInt("TTrackerVersion",3);
      if ( _TrackerVersion != 3) {
        throw cet::exception("TTrackerDeviceSupportSD")
          << "Expected TTrackerVersion of 3 but found " << _TrackerVersion <<endl;
      }
    }
  }

  G4bool TTrackerDeviceSupportSD::ProcessHits(G4Step* aStep,G4TouchableHistory*){

    _currentSize += 1;

    if ( _sizeLimit>0 && _currentSize>_sizeLimit ) {
      if( (_currentSize - _sizeLimit)==1 ) {
        mf::LogWarning("G4") << "Maximum number of particles reached in " 
                             << SensitiveDetectorName
                             << ": "
                             << _currentSize << endl;
      }
      return false;
    }

    // Which process caused this step to end?
    G4String const& pname  = aStep->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
    ProcessCode endCode(_processInfo->findAndCount(pname));

    G4int sdcn = 0;

    G4TouchableHandle const & touchableHandle = aStep->GetPreStepPoint()->GetTouchableHandle();

    G4Event const* event = G4RunManager::GetRunManager()->GetCurrentEvent();


    // we take here the numbering from TTrackerDeviceEnvelope
    // (not TTrackerDeviceSupport which is always 0)

    G4int en = event->GetEventID();
    G4int ti = aStep->GetTrack()->GetTrackID();
    G4int cn = touchableHandle->GetCopyNumber(1);

    //    G4TouchableHistory* theTouchable =
    //  (G4TouchableHistory*)( aStep->GetPreStepPoint()->GetTouchable() );

    if (verboseLevel > 1) {

      cout << "TTrackerDeviceSupportSD::" << __func__ << " Debugging history depth " <<
        setw(4) << touchableHandle->GetHistoryDepth() << endl;

      cout << "TTrackerDeviceSupportSD::" << __func__ << " Debugging copy n 0 1 2 3 4 " <<
        setw(4) << touchableHandle->GetCopyNumber(0) <<
        setw(4) << touchableHandle->GetCopyNumber(1) <<
        setw(4) << touchableHandle->GetCopyNumber(2) <<
        setw(4) << touchableHandle->GetCopyNumber(3) <<
        setw(4) << touchableHandle->GetCopyNumber(4) << endl;

      cout << "TTrackerDeviceSupportSD::" << __func__ << " Debugging PV Name Mother Name " <<
        touchableHandle->GetVolume(0)->GetName() << " " <<
        touchableHandle->GetVolume(1)->GetName() << " " <<
        touchableHandle->GetVolume(2)->GetName() << " " <<
        touchableHandle->GetVolume(3)->GetName() << " " <<
        touchableHandle->GetVolume(4)->GetName() << endl;

      cout << "TTrackerDeviceSupportSD::" << __func__ 
           << " Debugging hit info event track copyn replican: " <<
        setw(4) << en << " " <<
        setw(4) << ti << " " <<
        setw(4) << cn << endl;

      cout << "TTrackerDeviceSupportSD::" << __func__ << " Debugging _TrackerVersion: " << 
        _TrackerVersion << endl;

    }
    // device support is placed in a device envelope; device name should be unique though
    // therefore _TrackerVersion is probably not needed
    //

    if ( _TrackerVersion == 3) {
      sdcn = cn;
    }

    // Add the hit to the framework collection.
    // The point's coordinates are saved in the mu2e coordinate system.
    _collection->
      push_back(StepPointMC(art::Ptr<SimParticle>
                            ( *_simID,
                              aStep->GetTrack()->GetTrackID(),
                              _event->productGetter(*_simID) ),
                            sdcn,
                            aStep->GetTotalEnergyDeposit(),
                            aStep->GetNonIonizingEnergyDeposit(),
                            aStep->GetPreStepPoint()->GetGlobalTime(),
                            aStep->GetPreStepPoint()->GetProperTime(),
                            aStep->GetPreStepPoint()->GetPosition() - _mu2eOrigin,
                            aStep->GetPreStepPoint()->GetMomentum(),
                            aStep->GetStepLength(),
                            endCode
                            ));

    if (verboseLevel >0) {
      cout << "TTrackerDeviceSupportSD::" << __func__ << " Event " << setw(4) <<
        G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID() <<
        " TTrackerDeviceSupport " << 
        touchableHandle->GetVolume(1)->GetName() << " " <<
        setw(4) << touchableHandle->GetVolume(1)->GetCopyNo() <<
        " hit at: " << aStep->GetPreStepPoint()->GetPosition() - _mu2eOrigin << endl;
    }

    // Debugging tests (empty for now)
    if ( !_debugList.inList() ) return true;
    return true;

  }

} //namespace mu2e
