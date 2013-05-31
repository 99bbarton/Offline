//
// Class which extracts informayion from the framework event objects to build the event display shapes (e.g. tracks, straws, support structures).
//
// $Id: DataInterface.h,v 1.32 2013/05/31 18:07:18 gandr Exp $
// $Author: gandr $
// $Date: 2013/05/31 18:07:18 $
//
// Original author Ralf Ehrlich
//

#ifndef EventDisplay_src_DataInterface_h
#define EventDisplay_src_DataInterface_h

#include "CLHEP/Vector/ThreeVector.h"
#include "EventDisplay/src/ContentSelector.h"
#include "MCDataProducts/inc/SimParticleCollection.hh"
#include "RecoDataProducts/inc/StrawHitFlag.hh"
#include "art/Framework/Principal/Event.h"
#include "boost/shared_ptr.hpp"
#include <TObject.h>
#include <list>
#include <map>

class TGeoManager;
class TGeoVolume;
class TGMainFrame;

namespace mu2e_eventdisplay
{

class VirtualShape;
class Track;
class Straw;
class Cube;
class Cylinder;
class Cone;
class ComponentInfo;
class ContentSelector;
class EventDisplayFrame;

class DataInterface
{
  DataInterface();
  DataInterface(const DataInterface &);
  DataInterface& operator=(const DataInterface &);

  public:
  struct timeminmax
  {
    double mint, maxt;
  };
  struct spaceminmax
  {
    double minx, maxx;
    double miny, maxy;
    double minz, maxz;
  };

  private:
  art::Event    *_event;
  TGeoManager   *_geometrymanager; //bare pointer needed since ROOT manages this object
  TGeoVolume    *_topvolume;       //bare pointer needed since ROOT manages this object
  EventDisplayFrame  *_mainframe;  //points to the EventDisplayFrame object
                                   //ROOT needs a bare pointer to this object when dealing
                                   //with context menus (the function which gets called
                                   //from the context menu belongs to this object)
  std::list<boost::shared_ptr<VirtualShape> >   _components;
  std::map<int, boost::shared_ptr<Straw> >      _straws;
  std::map<int, boost::shared_ptr<Cube> >       _crystals;
  std::vector<boost::shared_ptr<Straw> >        _hits;
  std::vector<boost::shared_ptr<Cube> >         _crystalhits;
  std::vector<boost::shared_ptr<Cylinder> >     _driftradii;
  std::vector<boost::shared_ptr<Track> >        _tracks;
  std::vector<boost::shared_ptr<VirtualShape> > _supportstructures;
  std::vector<boost::shared_ptr<VirtualShape> > _otherstructures;
  std::vector<boost::shared_ptr<Cube> >         _crvscintillatorbars;
  std::vector<boost::shared_ptr<VirtualShape> > _mbsstructures;
  std::vector<boost::shared_ptr<Cone> > _mecostylepastructures;
  CLHEP::Hep3Vector _detSysOrigin;
  timeminmax        _hitsTimeMinmax, _tracksTimeMinmax;
  spaceminmax       _trackerMinmax, _targetMinmax, _calorimeterMinmax, _tracksMinmax;
  int               _numberHits, _numberCrystalHits;
  bool              _showUnhitStraws, _showUnhitCrystals;
  unsigned int _minPoints;
  double _minTime;
  double _maxTime;
  double _minMomentum;
  bool _showElectrons;
  bool _showMuons;
  bool _showGammas;
  bool _showNeutrinos;
  bool _showNeutrons;
  bool _showOthers;
  mu2e::StrawHitFlag _hitFlagSetting;

  void createGeometryManager();
  void removeAllComponents();
  void removeNonGeometryComponents();
  void findBoundaryT(timeminmax &m, double t);
  void findBoundaryP(spaceminmax &m, double x, double y, double z);
  void resetBoundaryT(timeminmax &m);
  void resetBoundaryP(spaceminmax &m);
  void toForeground();
  void findTrajectory(boost::shared_ptr<ContentSelector> const &contentSelector,
                      boost::shared_ptr<Track> const &track, const cet::map_vector_key &id,
                      double t1, double t2,
                      const mu2e::SimParticleCollection *simParticles,
                      const std::vector<cet::map_vector_key> &daughterVect,
                      const ContentSelector::trackInfoStruct &trackInfo);
  struct trajectoryStruct
  {
    CLHEP::Hep3Vector v;
    double t;
    trajectoryStruct() {t=NAN;}
  };

  public:
  DataInterface(EventDisplayFrame *mainframe);
  virtual ~DataInterface();

  void startComponents();
  void updateComponents(double time, boost::shared_ptr<ContentSelector> contentSelector);
  void fillGeometry();
  void fillEvent(boost::shared_ptr<ContentSelector> const &contentSelector);
  void makeSupportStructuresVisible(bool visible);
  void makeOtherStructuresVisible(bool visible);
  void makeCrvScintillatorBarsVisible(bool visible);
  void makeStrawsVisibleBeforeStart(bool visible);
  void makeCrystalsVisibleBeforeStart(bool visible);
  void makeMuonBeamStopStructuresVisible(bool visible);
  void makeMecoStyleProtonAbsorberVisible(bool visible);
  void useHitColors(bool hitcolors, bool whitebackground);
  void useTrackColors(boost::shared_ptr<ContentSelector> const &contentSelector, bool trackcolors, bool whitebackground);
  void getFilterValues(unsigned int &minPoints, double &minTime, double &maxTime, double &minMomentum,
                       bool &showElectrons, bool &showMuons, bool &showGammas, 
                       bool &showNeutrinos, bool &showNeutrons, bool &showOthers,
                       mu2e::StrawHitFlag &hitFlagSetting);
  void setFilterValues(unsigned int minPoints, double minTime, double maxTime, double minMomentum,
                       bool showElectrons, bool showMuons, bool showGammas, 
                       bool showNeutrinos, bool showNeutrons, bool showOthers,
                       mu2e::StrawHitFlag hitFlagSetting);
  int getNumberHits() {return _numberHits;}
  int getNumberCrystalHits() {return _numberCrystalHits;}

  timeminmax getHitsTimeBoundary() {return _hitsTimeMinmax;}
  timeminmax getTracksTimeBoundary() {return _tracksTimeMinmax;}
  spaceminmax getTrackerBoundary() {return _trackerMinmax;}
  spaceminmax getTargetBoundary() {return _targetMinmax;}
  spaceminmax getCalorimeterBoundary() {return _calorimeterMinmax;}
  spaceminmax getTracksBoundary() {return _tracksMinmax;}
  spaceminmax getSpaceBoundary(bool useTarget, bool useCalorimeter, bool useTracks);
};

}
#endif /* EventDisplay_src_DataInterface_h */
