#ifndef Mu2eUtilities_SimParticlesWithHits_HH
#define Mu2eUtilities_SimParticlesWithHits_HH
//
// This class makes available a collection of SimParticles that 
// have more than a minimum number of StrawHits with energy deposition
// in the gas above some cut.  The class can also return a
// vector of information about all of the StrawHits on each track
// in the collection.  If a StrawHit contains contributions from
// more than one SimParticle, that StrawHit will appear in the hit lists
// for all of the contributing SimParticles.
//
// This class is not designed to be peristable.
//
// $Id: SimParticlesWithHits.hh,v 1.4 2011/05/17 15:36:01 greenc Exp $
// $Author: greenc $
// $Date: 2011/05/17 15:36:01 $
//
// Original author Rob Kutschke.
//

// C++ includes.
#include <string>
#include <map>

// Mu2e includes.
#include "Mu2eUtilities/inc/SimParticleInfo.hh"
#include "ToyDP/inc/DPIndexVectorCollection.hh"
#include "ToyDP/inc/StepPointMCCollection.hh"
#include "ToyDP/inc/SimParticleCollection.hh"
#include "ToyDP/inc/StrawHitCollection.hh"
#include "ToyDP/inc/StrawHitMCTruthCollection.hh"

namespace art{
  class Event;
}

using namespace std;

namespace mu2e {

  class SimParticlesWithHits{
  public:

    typedef SimParticleCollection::key_type   key_type;
    typedef SimParticleInfo                   mapped_type;
    typedef map<key_type,SimParticleInfo>     map_type;
    typedef map_type::const_iterator          const_iterator;

    // No default c'tor by design.

    SimParticlesWithHits( const art::Event& evt,
                          string const& _g4ModuleLabel,
                          string const& _hitMakerModuleLabel,
                          string const& _trackerStepPoints,
                          double minEnergyDep,
                          size_t minHits );
    
    // Compiler generated code is Ok for:
    //  d'tor, copy c'tor assignment operator.

    // Find info for the requested SimParticle.  Return by pointer and
    // return a null pointer if the information is not in the map.
    mapped_type const* findOrNull( key_type key);

    // Find info for the requested SimParticle; throw if the particle
    // is not in the map.
    mapped_type const& at( key_type key);

    // Synonym for the at() function.
    mapped_type const& operator[]( key_type key){
      return at(key);
    }

    // Iterators over the collection.
    const_iterator begin() const { return _hitsPerTrack.begin(); }
    const_iterator end()   const { return _hitsPerTrack.end(); }
    
    // Number of particles with hits.
    size_t size() const { return _hitsPerTrack.size(); }

    // Access all of the information
    map_type const& getMap() const { return _hitsPerTrack; }

  private:

    // List of hits on each generated track.
    std::map<key_type,SimParticleInfo> _hitsPerTrack;

    // Pointers to some collections within the event; needed
    // by both the c'tor and selfTest().
    DPIndexVectorCollection   const* _hits_mcptr;
    StepPointMCCollection     const* _stepPointsMC;

    // Check that the object was built correctly.
    void selfTest();

  };

} // namespace mu2e

#endif
