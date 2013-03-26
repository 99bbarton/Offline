//
// Build a dictionary.
//
// $Id: classes.h,v 1.25 2013/03/26 23:28:23 kutschke Exp $
// $Author: kutschke $
// $Date: 2013/03/26 23:28:23 $
//
// Original author Rob Kutschke
//

#define ENABLE_MU2E_GENREFLEX_HACKS

#include <vector>

#include "art/Persistency/Common/Wrapper.h"
#include "art/Persistency/Common/Assns.h"

#include "RecoDataProducts/inc/StrawHitCollection.hh"
#include "RecoDataProducts/inc/StereoHitCollection.hh"
#include "RecoDataProducts/inc/StrawHitPositionCollection.hh"
#include "RecoDataProducts/inc/StrawHitFlagCollection.hh"
#include "RecoDataProducts/inc/StrawClusterCollection.hh"
#include "RecoDataProducts/inc/CaloHitCollection.hh"
#include "RecoDataProducts/inc/CaloCrystalHitCollection.hh"
#include "RecoDataProducts/inc/KalRepPayloadCollection.hh"
#include "RecoDataProducts/inc/KalRepExtensionPayloadCollection.hh"
#include "RecoDataProducts/inc/ExtMonUCITofHitCollection.hh"
#include "RecoDataProducts/inc/SubEventCollection.hh"
#include "RecoDataProducts/inc/TrackerHitTimeClusterCollection.hh"
#include "RecoDataProducts/inc/SctrSttnClusterGroupCollection.hh"
#include "RecoDataProducts/inc/ZRotStrawHitMapCollection.hh"
#include "RecoDataProducts/inc/TrackerHitByID.hh"
#include "RecoDataProducts/inc/CaloClusterCollection.hh"
#include "RecoDataProducts/inc/TrackSeedCollection.hh"
#include "RecoDataProducts/inc/TrkExtTrajCollection.hh"
#include "RecoDataProducts/inc/ExtMonFNALRawHit.hh"
#include "RecoDataProducts/inc/ExtMonFNALRawHitCollection.hh"
#include "RecoDataProducts/inc/ExtMonFNALRawCluster.hh"
#include "RecoDataProducts/inc/ExtMonFNALRawClusterCollection.hh"
#include "RecoDataProducts/inc/ExtMonFNALRecoCluster.hh"
#include "RecoDataProducts/inc/ExtMonFNALRecoClusterCollection.hh"

#include "RecoDataProducts/inc/ExtMonFNALTrkParam.hh"
#include "RecoDataProducts/inc/ExtMonFNALTrkFitQuality.hh"
#include "RecoDataProducts/inc/ExtMonFNALTrkClusterResiduals.hh"
#include "RecoDataProducts/inc/ExtMonFNALTrkFit.hh"
#include "RecoDataProducts/inc/ExtMonFNALTrkFitCollection.hh"

// Cannot use the typedefs in here - not sure why.
template class art::Ptr<mu2e::CaloHit>;
template class std::vector<art::Ptr<mu2e::CaloHit> >;
template class art::Ptr<mu2e::ExtMonUCITofHit>;
template class std::vector<art::Ptr<mu2e::ExtMonUCITofHit> >;
template class art::Ptr<mu2e::StrawHit>;
template class std::vector<art::Ptr<mu2e::StrawHit> >;
template class art::Ptr<mu2e::StereoHit>;
template class std::vector<art::Ptr<mu2e::StereoHit> >;
template class art::Ptr<mu2e::StrawHitPosition>;
template class std::vector<art::Ptr<mu2e::StrawHitPosition> >;
template class art::Ptr<mu2e::StrawHitFlag>;
template class std::vector<art::Ptr<mu2e::StrawHitFlag> >;
template class art::Ptr<mu2e::TrackerHitTimeCluster>;
template class std::vector<art::Ptr<mu2e::TrackerHitTimeCluster> >;
template class std::multimap<unsigned long int, art::Ptr<mu2e::StrawHit> >;
template class art::Ptr<mu2e::CaloCrystalHit>;
template class std::vector<art::Ptr<mu2e::CaloCrystalHit> >;
template class art::Ptr<mu2e::CaloCluster>;
template class std::vector<art::Ptr<mu2e::CaloCluster> >;
template class art::Ptr<mu2e::TrackSeed>;
template class std::vector<art::Ptr<mu2e::TrackSeed> >;
template class art::Ptr<mu2e::TrkExtTrajPoint>;
template class std::vector<mu2e::TrkExtTrajPoint>;
template class art::Ptr<mu2e::TrkExtTraj>;
template class std::vector<mu2e::TrkExtTraj>;
template class std::vector<std::pair<unsigned int, unsigned int> >;

template class art::Wrapper<mu2e::StrawHitCollection>;
template class art::Wrapper<mu2e::StereoHitCollection>;
template class art::Wrapper<mu2e::StrawHitPositionCollection>;
template class art::Wrapper<mu2e::StrawHitFlagCollection>;
template class art::Wrapper<mu2e::StrawClusterCollection>;
template class art::Wrapper<mu2e::CaloHitCollection>;
template class art::Wrapper<mu2e::CaloCrystalHitCollection>;
template class art::Wrapper<mu2e::CaloClusterCollection>;
template class art::Wrapper<mu2e::KalRepPayloadCollection>;
template class art::Wrapper<mu2e::KalRepExtensionPayloadCollection>;


template class art::Wrapper<mu2e::ExtMonUCITofHitCollection>;
template class art::Wrapper<mu2e::SubEventCollection>;
template class art::Wrapper<mu2e::TrackerHitTimeClusterCollection>;
template class art::Wrapper<mu2e::SctrSttnClusterGroupCollection>;
template class art::Wrapper<mu2e::ZRotStrawHitMapCollection>;
template class art::Wrapper<mu2e::TrackerHitByID>;
template class art::Wrapper<mu2e::TrackSeedCollection>;
template class art::Wrapper<mu2e::TrkExtTrajCollection>;

template class std::vector<mu2e::ExtMonFNALRawHit>;
template class art::Wrapper<mu2e::ExtMonFNALRawHitCollection>;

template class art::Ptr<mu2e::ExtMonFNALRawHit>;
template class art::PtrVector<mu2e::ExtMonFNALRawHit>;

template class art::Ptr<mu2e::ExtMonFNALRawCluster>;
template class std::vector<mu2e::ExtMonFNALRawCluster>;
template class art::Wrapper<mu2e::ExtMonFNALRawClusterCollection>;

template class std::vector<mu2e::ExtMonFNALRecoCluster>;
template class std::vector<std::vector<mu2e::ExtMonFNALRecoCluster> >;
template class art::Wrapper<mu2e::ExtMonFNALRecoClusterCollection>;

template class std::vector<mu2e::ExtMonFNALTrkParam>;

template class art::Ptr<mu2e::ExtMonFNALRecoCluster>;
template class art::Ptr<mu2e::ExtMonFNALTrkParam>;
template class std::pair<art::Ptr<mu2e::ExtMonFNALRecoCluster>, art::Ptr<mu2e::ExtMonFNALTrkParam> >;
template class std::pair<art::Ptr<mu2e::ExtMonFNALTrkParam>, art::Ptr<mu2e::ExtMonFNALRecoCluster> >;

template class std::vector<mu2e::ExtMonFNALTrkClusterResiduals>;
template class std::vector<mu2e::ExtMonFNALTrkFit>;
template class art::Wrapper<mu2e::ExtMonFNALTrkFitCollection>;

#undef ENABLE_MU2E_GENREFLEX_HACKS
