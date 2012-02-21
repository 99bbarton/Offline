#ifndef MCDataProducts_SimParticle_hh
#define MCDataProducts_SimParticle_hh

//
// Information about particles created by Geant4.
//
// $Id: SimParticle.hh,v 1.9 2012/02/21 21:19:14 onoratog Exp $
// $Author: onoratog $
// $Date: 2012/02/21 21:19:14 $
//
// Original author Rob Kutschke
//
// Notes:
// 1) Internally G4 numbers tracks 1...N.  An earlier version of TrackingAction
//    renumbered them 0...(N-1); this was an artifact of the SimParticleCollection
//    class being a std::vector, which starts at 0. But now SimParticleCollection
//    is a cet::map_vector, so it is no longer necessary to do the renumbering.
//    Therefore the correct test to see if a particle has a parent is
//
// 2) The trackId, parentIds and daughterIds are all of the correct type to be
//    used to find the corresponding information in SimParticleCollection
//
// 3) The trackId, parentIds and daughterIds are all redundant now that the Ptr
//    versions are available.  We will get rid of them as soon as we check
//    backwards compatibility.

#include "CLHEP/Vector/LorentzVector.h"
#include "CLHEP/Vector/ThreeVector.h"
#include "MCDataProducts/inc/GenParticle.hh"
#include "MCDataProducts/inc/ProcessCode.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"
#include "MCDataProducts/inc/PDGCode.hh"

#include "art/Persistency/Common/Ptr.h"

#include "cetlib/map_vector.h"

#include <iostream>
#include <vector>

namespace mu2e {

  struct SimParticle {

    typedef cet::map_vector_key key_type;

    // A default c'tor is required for ROOT.
    SimParticle():
      _id(),
      _parentId(0),
      _parentSim(),
      _pdgId(),
      _genIndex(-1),
      _genParticle(),
      _startPosition(),
      _startMomentum(),
      _startGlobalTime(0.),
      _startProperTime(0.),
      _startVolumeIndex(0),
      _startG4Status(),
      _creationCode(),
      _endPosition(),
      _endMomentum(),
      _endGlobalTime(0.),
      _endProperTime(0.),
      _endVolumeIndex(0),
      _endG4Status(),
      _stoppingCode(),
      _preLastStepKE(-1),
      _nSteps(0),
      _daughterSims(),
      _weight(0.),
      _endDefined(false){
    }

    SimParticle( key_type                       aid,
                 art::Ptr<SimParticle> const&   aparentSim,
                 PDGCode::type                  apdgId,
                 art::Ptr<GenParticle> const&   agenParticle,
                 const CLHEP::Hep3Vector&       aposition,
                 const CLHEP::HepLorentzVector& amomentum,
                 double                         astartGlobalTime,
                 double                         astartProperTime,
                 unsigned                       astartVolumeIndex,
                 unsigned                       astartG4Status,
                 ProcessCode                    acreationCode,
                 double                         aweight=1.):
      _id(aid),
      _parentId(0),
      _parentSim(aparentSim),
      _pdgId(apdgId),
      _genIndex(-1),
      _genParticle(agenParticle),
      _startPosition(aposition),
      _startMomentum(amomentum),
      _startGlobalTime(astartGlobalTime),
      _startProperTime(astartProperTime),
      _startVolumeIndex(astartVolumeIndex),
      _startG4Status(astartG4Status),
      _creationCode(acreationCode),
      _endPosition(),
      _endMomentum(),
      _endGlobalTime(),
      _endProperTime(),
      _endVolumeIndex(),
      _endG4Status(),
      _stoppingCode(),
      _preLastStepKE(-1),
      _nSteps(0),
      _daughterSims(),
      _weight(aweight),
      _endDefined(false)
    {}

    // Accept compiler generated d'tor, copy c'tor and assignment operator.

    void addEndInfo( CLHEP::Hep3Vector       aendPosition,
                     CLHEP::HepLorentzVector aendMomentum,
                     double                  aendGlobalTime,
                     double                  aendProperTime,
                     unsigned                aendVolumeIndex,
                     unsigned                aendG4Status,
                     ProcessCode             astoppingCode,
                     double                  preLastStepKE,
                     int                     nSteps){
      _endDefined      = true;
      _endPosition     = aendPosition;
      _endMomentum     = aendMomentum;
      _endGlobalTime   = aendGlobalTime;
      _endProperTime   = aendProperTime;
      _endVolumeIndex  = aendVolumeIndex;
      _endG4Status     = aendG4Status;
      _stoppingCode    = astoppingCode;
      _preLastStepKE   = preLastStepKE;
      _nSteps          = nSteps;
    }

    void addDaughter( art::Ptr<SimParticle> const& p ){
      _daughterSims.push_back(p);
      _daughterIds.push_back( key_type(p.key()) );
    }

    // Some Accessors/Modifier pairs.
    // The modifiers are needed by the event mixing code.

    // Key of this track within the SimParticleCollection.  See notes 1 and 2.
    key_type  id() const {return _id;}
    key_type& id()       { return _id;}

    // The parent of this track; may be null.
    art::Ptr<SimParticle> const& parent() const { return _parentSim; }
    art::Ptr<SimParticle>&       parent()       { return _parentSim; }

    // The genparticle corresponding to this track; may be null.
    art::Ptr<GenParticle> const& genParticle() const { return _genParticle;}
    art::Ptr<GenParticle>&       genParticle()       { return _genParticle;}

    // Most members have only accessors

    // PDG particle ID code.
    PDGCode::type pdgId() const {return _pdgId;}

    // Where was this particle created: in the event generator or in G4?
    bool isSecondary()   const { return _parentSim.isNonnull(); }
    bool isPrimary()     const { return _genParticle.isNonnull(); }

    // Some synonyms for the previous two accessors.
    bool hasParent()     const { return _parentSim.isNonnull(); }
    bool fromGenerator() const { return _genParticle.isNonnull(); }
    bool madeInG4()      const { return _genParticle.isNull();    }

    // Information at the start of the track.
    CLHEP::Hep3Vector const& startPosition()       const { return _startPosition;}
    CLHEP::HepLorentzVector const& startMomentum() const { return _startMomentum;}
    double      startGlobalTime()  const { return _startGlobalTime;}
    double      startProperTime()  const { return _startProperTime;}
    unsigned    startVolumeIndex() const { return _startVolumeIndex;}
    unsigned    startG4Status()    const { return _startG4Status;}
    ProcessCode creationCode()      const { return _creationCode;  }

    // Information at the end of the track.
    CLHEP::Hep3Vector const& endPosition() const { return _endPosition;}
    CLHEP::HepLorentzVector const& endMomentum() const { return _endMomentum;}
    double       endGlobalTime()  const { return _endGlobalTime; }
    double       endProperTime()  const { return _endProperTime; }
    unsigned     endVolumeIndex() const { return _endVolumeIndex;}
    unsigned     endG4Status()    const { return _endG4Status;   }
    ProcessCode  stoppingCode()   const { return _stoppingCode;  }
    double       preLastStepKineticEnergy() const { return _preLastStepKE; }
    int          nSteps()  const { return _nSteps;        }

    // SimParticle indices of daughters of this track.
    std::vector<key_type>               const& daughterIds() const { return _daughterIds;}
    std::vector<art::Ptr<SimParticle> > const& daughters()   const { return _daughterSims; }

    // Weight
    double weight() const { return  _weight;}

    // Is the second half defined?
    bool endDefined() const { return _endDefined;}

    // Modifiers;
    void setDaughterPtrs  ( std::vector<art::Ptr<SimParticle> > const& ptr){
      _daughterSims.clear();
      _daughterSims.reserve(ptr.size());
      _daughterSims.insert( _daughterSims.begin(), ptr.begin(), ptr.end() );
      _daughterIds.clear();
      _daughterIds.reserve(ptr.size());
      for ( size_t i=0; i != ptr.size(); ++i){
        _daughterIds.push_back( key_type( ptr[i].key() ) );
      }
    }

    // Two older accessors that will soon be removed from the interface.

    // Index of the parent of this track; may be null.
    key_type                     parentId() const {
      return ( _parentSim.isNonnull()) ? key_type(_parentSim.key()) : key_type(0);
    }

    // Index into the container of generated tracks;
    // -1 if there is no corresponding generated track.
    int                          generatorIndex() const {
      return ( _genParticle.isNonnull()) ? _genParticle.key() : -1;
    }

  private:
    // G4 ID number of this track and of its parent.
    // See notes 1 and 2.
    key_type _id;
    key_type _parentId;                  // Obsolete and not used.  Will be deleted at a convenient time.
    art::Ptr<SimParticle> _parentSim;

    // PDG particle ID code.  See note 1.
    PDGCode::type _pdgId;

    // Index into the container of generated tracks;
    // -1 if there is no corresponding generated track.
    int                    _genIndex;      // Obsolete and not used.  Will be deleted at a convenient time.
    art::Ptr<GenParticle>  _genParticle;

    // Information at the start of the track.
    CLHEP::Hep3Vector       _startPosition;
    CLHEP::HepLorentzVector _startMomentum;
    double                  _startGlobalTime;
    double                  _startProperTime;
    unsigned                _startVolumeIndex;
    unsigned                _startG4Status;
    ProcessCode             _creationCode;

    // Information at the end of the track.
    CLHEP::Hep3Vector       _endPosition;
    CLHEP::HepLorentzVector _endMomentum;
    double                  _endGlobalTime;
    double                  _endProperTime;
    unsigned                _endVolumeIndex;
    unsigned                _endG4Status;
    ProcessCode             _stoppingCode;
    double                  _preLastStepKE;
    int                     _nSteps;

    // SimParticle IDs of daughters of this track.
    std::vector<key_type>  _daughterIds;
    std::vector<art::Ptr<SimParticle> > _daughterSims;

    // Weight
    double _weight;

    // Is the second half defined?
    bool _endDefined;

  };

}

#endif /* MCDataProducts_SimParticle_hh */
