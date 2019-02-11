#ifndef TTrackerGeom_TTracker_hh
#define TTrackerGeom_TTracker_hh
//
// Hold all geometry and identifier information about
// a TTracker.  This is intended as a "data only"
// class.
//
// Original author Rob Kutschke
//

#include <vector>
#include <array>
#include <limits>
//#include <iomanip>
//#include <iostream>

#include "cetlib_except/exception.h"

//#include "DataProducts/inc/StrawId.hh"  included via Straw via Tracker (-> Straw)

#include "TTrackerGeom/inc/Manifold.hh"
#include "TTrackerGeom/inc/Support.hh"
#include "TTrackerGeom/inc/SupportModel.hh"
#include "TTrackerGeom/inc/SupportStructure.hh"

#include "TrackerGeom/inc/Plane.hh"
#include "DataProducts/inc/PanelId.hh"
#include "TrackerGeom/inc/StrawDetail.hh"
#include "TrackerGeom/inc/Tracker.hh"
#include "GeomPrimitives/inc/TubsParams.hh"
#include "GeomPrimitives/inc/PlacedTubs.hh"

#include "TTrackerGeom/inc/Station.hh"

namespace mu2e {

  class TTracker: public Tracker{

        friend class TTrackerMaker;

  public:

    // =============== NewTracker Public Objects Start ==============

    // constexpr static int _nttstraws = StrawId::_nplanes *
    //                                   StrawId::_npanels *
    //                                   StrawId::_nstraws; // uncomment after eliminating Tracker

    // constexpr static int _maxRedirect = std::numeric_limits<uint16_t>::max();
    constexpr static uint16_t _maxRedirect =
      ((StrawId::_nplanes -1) << StrawId::_planesft) +
      ((StrawId::_npanels -1) << StrawId::_panelsft) +
      StrawId::_nstraws;

    // =============== NewTracker Public Objects End   ==============

    TTracker(){
      if (StrawId::_nlayers != 2)
        throw cet::exception("GEOM")
          << "Expect configuration with 2 layers per panel\n";
    }  // TODO: insert proper initializer list, starting w/ base class

    // Use compiler-generated copy c'tor, copy assignment, and d'tor

    void fillPointers () const;

    double rOut() const { return _rOut;}
    double z0()   const { return _z0;}
    double zHalfLength() const;

    double strawRadius() const{
      return getStraw(StrawId(0,0,0)).getDetail().outerRadius();
    }

    std::string const& envelopeMaterial() const { return _envelopeMaterial; }

    // Check for legal identifiers. (for what decays to StrawId)
    bool isLegal(const StrawId strid) const{
       return strid.valid();
    }

    // Accessors
    uint16_t nPlanes() const{
      // return StrawId::_nplanes;
      return _nPlanes; // tmp till we process cd3
    }

    uint16_t nStraws() const { return _nStraws; } // tmp till we process cd3

    const std::array<Plane,StrawId::_nplanes>& getPlanes() const{
      return _planes;
    }

    const std::vector<double>& getManifoldHalfLengths () const{
      return _manifoldHalfLengths;
    }

    double panelOffset() const { return _panelZOffset; }

    int nStations() const{
      return _stations.size();
    }

    const std::vector<Station>& getStations() const{
      return _stations;
    }

    const Station& getStation ( StationId id) const{
      return _stations.at(id);
    }

    const std::array<Straw,TTracker::_nttstraws>& getAllStraws() const
    {return _allStraws2;}

    const std::vector<StrawDetail>& getStrawDetails() const{
      return _strawDetails;
    }

    SupportModel getSupportModel() const{
      return _supportModel;
    }

    const Support& getSupportParams () const{
      return _supportParams;
    }

    const SupportStructure& getSupportStructure() const{
      return _supportStructure;
    }


    TubsParams getPlaneEnvelopeParams() const{
      return _planeEnvelopeParams;
    }

    TubsParams getPanelEnvelopeParams() const{
      return _panelEnvelopeParams;
    }

    const TubsParams& getInnerTrackerEnvelopeParams() const{
      return _innerTrackerEnvelopeParams;
    }

    PlacedTubs mother() const{
      return _mother;
    }

    // =============== NewTracker Accessors Start ==============

    const Straw& getStraw( const StrawId id) const{
      return *(_allStraws2_p.at(id.asUint16()));
    }

    const Plane& getPlane( const StrawId id ) const{
      return _planes.at(id.getPlane());
    }

    const Plane& getPlane( uint16_t n ) const{
      return _planes.at(n);
    }

    const Panel& getPanel ( const StrawId id ) const{
      return _planes.at(id.getPlane()).getPanel(id);
    }

    bool strawExists( StrawId const id) const{
      // return _allStraws2_p.at(id.asUint16()) != nullptr;
      return _strawExists2.at(id.asUint16());
    }

    // =============== NewTracker Accessors End   ==============

#ifndef __CINT__

    // Loop over all straws and call F.
    // F can be a class with an operator() or a free function.
    template <class F>
    inline void forAllStraws ( F& f) const{
      // for ( const auto& plane : _planes ) { // fixme: should be used after cd3 is processed
      //   plane.forAllStraws(f);
      // }
      for ( size_t i=0; i<_nPlanes; ++i){
        _planes[i].forAllStraws(f);
      }
    }

    template <class F>
    inline void forAllPanels ( F& f) const{
      // for ( const auto& plane : _planes ) {
      //   plane.forAllPanels(f);
      // }
      for ( size_t i=0; i<_nPlanes; ++i){
        _planes[i].forAllPanels(f);
      }
    }

    template <class F>
    inline void forAllPlanes ( F& f) const{
      // for ( const auto& plane : _planes ) {
      //   f(plane);
      // }
      for ( size_t i=0; i<_nPlanes; ++i){
        f(_planes[i]);
      }
    }

#endif


  protected:

    // Position of the center of the tracker, in the Mu2e coordinate system.
    double _z0;

    // Outer radius of a logical volume that will just contain the entire tracker.
    double _rOut;

    // All envelope volumes are made of this.
    std::string _envelopeMaterial;

    // Detailed info about each type of straw.
    std::vector<StrawDetail> _strawDetails;

    // An TTracker is made of two planes, sides and vanes.
    // std::vector<Plane> _planes;

    // An alternative viewpoint:
    // A TTracker is made of a collection of Stations.
    std::vector<Station> _stations;

    // There will be pointers to the objects in this container.
    //    std::deque<Straw>  _allStraws;

    // Deprecated: part of the ancient MECO TTracker design.
    // A few vestiges not yet removed.
    std::vector<Manifold> _allManifolds;

    // Outer envelope that holds the new style support structure.
    PlacedTubs _mother;

    // The envelope that holds all of the planes in the tracker,
    // including the plane supports.
    TubsParams _innerTrackerEnvelopeParams;

    // The envelope that holds all of the pieces in one plane, including supports.
    TubsParams _planeEnvelopeParams;

    // Ditto for Panel
    TubsParams _panelEnvelopeParams;

    // Which level of detail is present in the model of the support structure?
    SupportModel _supportModel;

    // All supports are the same shape; only relevant for _supportModel=="simple"
    Support _supportParams;
    double _panelZOffset; // introduced for version 5

    // A more detailed model of the supports; again each plane has identical supports.
    // only relevant for _supportModel == "detailedv0".
    SupportStructure _supportStructure;

    // All manifolds are the same shape.
    // Deprecated: these will go away soon.
    std::vector<double> _manifoldHalfLengths;

    // Inner radius of inside edge of innermost straw.
    double _envelopeInnerRadius;

    // presence info for each straw.
    //    std::vector<bool> _strawExists;

    uint16_t _nPlanes; // fixme tmp till we process cd3
    uint16_t _nStraws; // fixme tmp till we process cd3
    // =============== NewTracker Private Objects Start ==============

    // Dense array.
    std::array<Plane,StrawId::_nplanes> _planes;

    // Dense array.
    std::array<Straw,TTracker::_nttstraws> _allStraws2;

    // Sparse array: designed for indexing by StrawId.
    // For all legal entries in StrawId, this points to a straw in _straws2;
    // All other entries are null.
    std::array<Straw const*,TTracker::_maxRedirect> _allStraws2_p;

    // Another sparse array
    std::array<bool,TTracker::_maxRedirect> _strawExists2;

    // =============== NewTracker Private Objects End ==============

  };

} //namespace mu2e

#endif /* TTrackerGeom_TTracker_hh */
