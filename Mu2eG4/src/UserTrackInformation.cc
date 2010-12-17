//
// Mu2e specific information about one G4 track.
//
// $Id: UserTrackInformation.cc,v 1.1 2010/12/17 22:07:56 kutschke Exp $
// $Author: kutschke $
// $Date: 2010/12/17 22:07:56 $
//
// Original author Rob Kutschke
//

#include <iostream>

// Mu2e includes
#include "Mu2eG4/inc/UserTrackInformation.hh"

using namespace std;

namespace mu2e{

  UserTrackInformation::UserTrackInformation():
    G4VUserTrackInformation("Mu2eTrackInfo"),
    _forcedStop(false),
    _code(){
  }

  UserTrackInformation::~UserTrackInformation(){
  }

  void UserTrackInformation::Print() const{
  }

} // end namespace mu2e

