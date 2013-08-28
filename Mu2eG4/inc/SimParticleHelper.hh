// The job of this class is to translate between G4 track IDs and
// keys of SimParticles produced by the current job.
//
// Andrei Gaponenko, 2013

#ifndef Mu2eG4_inc_SimParticleHelper
#define Mu2eG4_inc_SimParticleHelper

#include "art/Persistency/Common/Ptr.h"

#include "MCDataProducts/inc/SimParticle.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"

class G4Track;
namespace art { class Event; }
namespace art { class ProductID; }

namespace mu2e {
  class SimParticleHelper {
    unsigned particleNumberOffset_;
    art::ProductID simID_;
    const art::Event* event_;

  public:
    SimParticleHelper(unsigned particleNumberOffset,
                      const art::ProductID& simID,
                      const art::Event& event);

    art::Ptr<SimParticle> particlePtr(const G4Track* trk) const;

    art::Ptr<SimParticle> particlePtrFromG4TrackID(int g4TrkID) const;

    SimParticleCollection::key_type particleKeyFromG4TrackID(int g4TrkID) const;

    art::ProductID productID() const { return simID_; }
    const art::EDProductGetter *productGetter() const;
  };
}

#endif/*Mu2eG4_inc_SimParticleHelper*/
