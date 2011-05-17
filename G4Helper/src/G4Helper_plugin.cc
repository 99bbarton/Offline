//
// G4Helper plugin.
//
// $Id: G4Helper_plugin.cc,v 1.2 2011/05/17 15:36:00 greenc Exp $
// $Author: greenc $ 
// $Date: 2011/05/17 15:36:00 $
//
// Original author Rob Kutschke
//

// Framework includes
#include "FWCore/ServiceRegistry/interface/ServiceMaker.h"

// Mu2e includes
#include "G4Helper/inc/G4Helper.hh"

using namespace std;

namespace mu2e {

  G4Helper::G4Helper(fhicl::ParameterSet const& iPS, 
                     art::ActivityRegistry&iRegistry){
  }

  G4Helper::~G4Helper(){
  }

  // Return the volume info mapped to the given key, throw if the key does not exist.
  VolumeInfo& G4Helper::locateVolInfo( const std::string key){
    std::map<std::string,VolumeInfo>::iterator i = _volumeInfoList.find(key);
    if ( i == _volumeInfoList.end() ){
      throw cet::exception("GEOM")
        << "G4Helper::locateVolInfo cannot find the volume named: "
        << key 
        << "\n";
    }
    return i->second;
  } // end of G4Helper::locateVolInfo

  // If the key already exists, throw. Otherwise add the (key, value) pair
  // to the map.
  void G4Helper::addVolInfo( const VolumeInfo& info ){
    std::map<std::string,VolumeInfo>::iterator i = _volumeInfoList.find(info.name);
    if ( i != _volumeInfoList.end() ){
      throw cet::exception("GEOM")
        << "locateVolInfo already has the key: "
        << info.name
        << "\n";
    }
    _volumeInfoList[info.name] = info;
  } // end of G4Helper::addVolInfo

} // end namespace mu2e

using mu2e::G4Helper;
DEFINE_ART_SERVICE(G4Helper);
