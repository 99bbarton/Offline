//
// Class which manages the combo boxes and list box in the event display frame. It is able to returns the data objects associated with the selected box entries.
//
// $Id: ContentSelector.h,v 1.15 2013/05/02 06:03:41 ehrlich Exp $
// $Author: ehrlich $
// $Date: 2013/05/02 06:03:41 $
//
// Original author Ralf Ehrlich
//

#ifndef EventDisplay_src_ContentSelector_h
#define EventDisplay_src_ContentSelector_h

#include "RecoDataProducts/inc/CaloCrystalHitCollection.hh"
#include "RecoDataProducts/inc/CaloHitCollection.hh"
#include "MCDataProducts/inc/PhysicalVolumeInfoCollection.hh"
#include "MCDataProducts/inc/PointTrajectoryCollection.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"
#include "MCDataProducts/inc/StepPointMCCollection.hh"
#include "RecoDataProducts/inc/StrawHitCollection.hh"
#include "RecoDataProducts/inc/StrawHitFlagCollection.hh"
#include "RecoDataProducts/inc/StrawHitPositionCollection.hh"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include <TGComboBox.h>
#include <TGListBox.h>
#include <iostream>
#include <vector>

#ifdef BABARINSTALLED
using namespace CLHEP;
#include "KalmanTests/inc/KalRepCollection.hh"
#include "RecoDataProducts/inc/TrkExtTrajCollection.hh"
#else
#warning BaBar package is absent. KalRep cannot be displayed in the event display.
#endif

namespace mu2e_eventdisplay
{

class ContentSelector
{
  ContentSelector();
  ContentSelector(const ContentSelector &);
  ContentSelector& operator=(const ContentSelector &);

  std::vector<art::Handle<mu2e::StepPointMCCollection> > _stepPointMCVector;
  std::vector<art::Handle<mu2e::StrawHitCollection> > _strawHitVector;
  std::vector<art::Handle<mu2e::StrawHitFlagCollection> > _strawHitFlagVector;
  std::vector<art::Handle<mu2e::StrawHitPositionCollection> > _strawHitPositionVector;
  std::vector<art::Handle<mu2e::CaloCrystalHitCollection> > _caloCrystalHitVector;
  std::vector<art::Handle<mu2e::CaloHitCollection> > _caloHitVector;
  std::vector<art::Handle<mu2e::SimParticleCollection> > _simParticleVector;
  std::vector<art::Handle<mu2e::PointTrajectoryCollection> > _pointTrajectoryVector;
#ifdef BABARINSTALLED
  std::vector<art::Handle<mu2e::KalRepCollection> > _trkRecoTrkVector;
  std::vector<art::Handle<mu2e::KalRepCollection> > _hitOnTrackVector; //Hits on Tracks are stored inside of KalRep
  std::vector<art::Handle<mu2e::TrkExtTrajCollection> > _trkExtTrajVector;
#endif
  art::Handle<mu2e::PhysicalVolumeInfoCollection> _physicalVolumes;
  bool _hasPhysicalVolumes;

  TGComboBox  *_hitBox;
  TGComboBox  *_caloHitBox;
  TGListBox   *_trackBox;
  std::string _g4ModuleLabel;

  public:
  struct trackInfoStruct
  {
    int classID, index;
    std::string entryText;
    std::string moduleLabel, productInstanceName;
  };

  private:
  struct entryStruct
  {
    int         entryID, classID, vectorPos;
    std::string entryText;
    std::string className, moduleLabel, productInstanceName;
    bool operator==(const entryStruct& rhs) const
    {
      return ((this->entryID==rhs.entryID) && (this->entryText==rhs.entryText));
    }
  };
  std::vector<entryStruct> _hitEntries, _caloHitEntries, _trackEntries;
  std::vector<entryStruct> _hitFlagEntries, _hitPositionEntries;

  std::string _selectedHitFlagEntry, _selectedHitPositionEntry;

  template<class CollectionType> void createNewEntries(std::vector<art::Handle<CollectionType> > &dataVector,
                                                       const art::Event &event, const std::string &className,
                                                       std::vector<entryStruct> &newEntries, int classID);

  public:
  ContentSelector(TGComboBox *hitBox, TGComboBox *caloHitBox, TGListBox *trackBox, std::string const &g4ModuleLabel);
  void firstLoop();
  void setAvailableCollections(const art::Event& event);

  bool getSelectedHitsName(std::string &className,
                           std::string &moduleLabel,
                           std::string &productInstanceName) const;
  std::vector<trackInfoStruct> getSelectedTrackNames() const;

  template<typename CollectionType> const CollectionType* getSelectedHitCollection() const;
  template<typename CollectionType> const CollectionType* getSelectedCaloHitCollection() const;
  template<typename CollectionType> std::vector<const CollectionType*> getSelectedTrackCollection(std::vector<trackInfoStruct> &v) const;
  const mu2e::PhysicalVolumeInfoCollection *getPhysicalVolumeInfoCollection() const;
  const mu2e::PointTrajectoryCollection *getPointTrajectoryCollection(const trackInfoStruct &t) const;

  //for filter and setup dialog
  const std::vector<entryStruct> &getStrawHitFlagEntries() const     {return _hitFlagEntries;}
  const std::vector<entryStruct> &getStrawHitPositionEntries() const {return _hitPositionEntries;}
  const std::string getSelectedStrawHitFlagEntry()      {return _selectedHitFlagEntry;}
  const std::string getSelectedStrawPositionFlagEntry() {return _selectedHitPositionEntry;}
  void setSelectedStrawHitFlagEntry(std::string selectedHitFlagEntry)
                                                         {_selectedHitFlagEntry=selectedHitFlagEntry;}
  void setSelectedStrawHitPositionEntry(std::string selectedHitPositionEntry)
                                                         {_selectedHitPositionEntry=selectedHitPositionEntry;}
  const mu2e::StrawHitFlagCollection *getStrawHitFlagCollection() const;
  const mu2e::StrawHitPositionCollection *getStrawHitPositionCollection() const;

  friend class FilterDialog;
};

}

#endif /* EventDisplay_src_ContentSelector_h */
