//
// A module to study background rates in the detector subsystems.
//
// $Id: BkgRates_module.cc,v 1.33 2012/06/20 17:08:11 onoratog Exp $
// $Author: onoratog $
// $Date: 2012/06/20 17:08:11 $
//
// Original author Gianni Onorato
//

#include "CLHEP/Units/PhysicalConstants.h"
#include "CalorimeterGeom/inc/Calorimeter.hh"
#include "MCDataProducts/inc/PtrStepPointMCVectorCollection.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/getTrackerOrThrow.hh"
#include "ITrackerGeom/inc/Cell.hh"
#include "ITrackerGeom/inc/ITracker.hh"
#include "MCDataProducts/inc/CaloHitMCTruthCollection.hh"
#include "MCDataProducts/inc/GenParticleCollection.hh"
#include "MCDataProducts/inc/PhysicalVolumeInfoCollection.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"
#include "MCDataProducts/inc/StepPointMCCollection.hh"
#include "MCDataProducts/inc/StrawHitMCTruthCollection.hh"
#include "RecoDataProducts/inc/CaloCrystalHitCollection.hh"
#include "RecoDataProducts/inc/CaloHitCollection.hh"
#include "RecoDataProducts/inc/StrawHitCollection.hh"
#include "TFile.h"
#include "TNtuple.h"
#include "TTrackerGeom/inc/TTracker.hh"
#include "TrackerGeom/inc/Straw.hh"
#include "TrackerGeom/inc/Tracker.hh"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Provenance.h"
#include "Mu2eUtilities/inc/LinePointPCA.hh"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "MCDataProducts/inc/StatusG4.hh"
#include <cmath>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>

using namespace std;

namespace mu2e {


  class BkgRates : public art::EDAnalyzer {
  public:

    typedef vector<int> Vint;

    explicit BkgRates(fhicl::ParameterSet const& pset):
      _diagLevel(pset.get<int>("diagLevel",0)),
      _trackerStepPoints(pset.get<string>("trackerStepPoints","tracker")),
      _makerModuleLabel(pset.get<std::string>("makerModuleLabel")),
      _generatorModuleLabel(pset.get<std::string>("generatorModuleLabel", "generate")),
      _g4ModuleLabel(pset.get<std::string>("g4ModuleLabel", "g4run")),
      _caloReadoutModuleLabel(pset.get<std::string>("caloReadoutModuleLabel", "CaloReadoutHitsMaker")),
      _caloCrystalModuleLabel(pset.get<std::string>("caloCrystalModuleLabel", "CaloCrystalHitsMaker")),
      _minimumEnergyTracker(pset.get<double>("minimumEnergyTracker",0.0001)), // MeV
      _minimumEnergyCalo(pset.get<double>("minimumEnergyCalo",0.0001)), // MeV
      _doStoppingTarget(pset.get<bool>("doStoppingTarget", 0)),
      _nAnalyzed(0),
      _tNtup(0),
      _cNtup(0),
      _tgtNtup(0),
      _nBadG4Status(0),
      _nOverflow(0),
      _nKilled(0),
      _totalcputime(0),
      _totalrealtime(0)
    {
      Vint const & _particleToSkipInST = pset.get<Vint>("pdgIdToSkipInST", Vint());
      if (_particleToSkipInST.size() > 0) {
        for (size_t i = 0; i< _particleToSkipInST.size(); ++i){
          skipPDG.insert(_particleToSkipInST[i]);
        }
      }
      cout << "Module BkgRates is starting" << endl;
    }
    virtual ~BkgRates() {
    }
    virtual void beginJob();
    virtual void endJob();

    void analyze(art::Event const& e );

  private:

    void doTracker(art::Event const& evt, bool skip);
    void doITracker(art::Event const& evt, bool skip);

    void doCalorimeter(art::Event const& evt, bool skip);
    void doStoppingTarget(art::Event const& evt);

    // Diagnostic level
    int _diagLevel;

    // Name of the tracker StepPoint collection
    std::string _trackerStepPoints;

    // Label of the module that made the hits.
    std::string _makerModuleLabel;

    // Label of the generator.
    std::string _generatorModuleLabel;

    // Label of the G4 module
    std::string _g4ModuleLabel;

    // Label of the calo readout hits maker
    std::string _caloReadoutModuleLabel;

    // Label of the calo crystal hists maker
    std::string _caloCrystalModuleLabel;

    double _minimumEnergyTracker, _minimumEnergyCalo; //minimum energy deposition of hits
    
    bool _doStoppingTarget;

    //number of analyzed events
    int _nAnalyzed;

    TNtuple* _tNtup;
    TNtuple* _cNtup;
    TNtuple* _tgtNtup;

    bool _skipEvent;

    int _nBadG4Status, _nOverflow, _nKilled;
    float _totalcputime, _totalrealtime;

    set<int> skipPDG;

  };

  void BkgRates::beginJob( ) {
  }

  void BkgRates::analyze(art::Event const& evt ) {

    ++_nAnalyzed;

    //*****test code******
        static int ncalls(0);
    ++ncalls;

    art::Handle<StatusG4> g4StatusHandle;
    evt.getByLabel( _g4ModuleLabel, g4StatusHandle);
    StatusG4 const& g4Status = *g4StatusHandle;

    if ( g4Status.status() > 1 ) {
      ++_nBadG4Status;
      mf::LogError("G4")
        << "Aborting BkgRates::analyze due to G4 status\n"
        << g4Status;
      return;
    }

    if (g4Status.overflowSimParticles()) {
      ++_nOverflow;
      mf::LogError("G4")
        << "Aborting BkgRates::analyze due to overflow of particles\n"
        << g4Status;
      return;
    }

    if (g4Status.nKilledStepLimit() > 0) {
      ++_nKilled;
      mf::LogError("G4")
        << "Aborting BkgRates::analyze due to nkilledStepLimit reached\n"
        << g4Status;
      return;
    }

    _totalcputime += g4Status.cpuTime();
    _totalrealtime += g4Status.realTime();

    art::ServiceHandle<GeometryService> geom;

    if (ncalls == 1) {

      // cout << "This should be done only in the first event" << endl;


      art::ServiceHandle<art::TFileService> tfs;

      if (geom->hasElement<TTracker>()) {
        _tNtup        = tfs->make<TNtuple>( "StrawHits", "Straw Ntuple",
                                            "evt:run:time:dt:eDep:lay:dev:sec:strawId:MChitX:MChitY:v:vMC:z:trkId:pdgId:isGen:StepFromEva:EvaIsGen:EvaCreationCode:P:CreationCode:StartVolume:StartX:StartY:StartZ:StartT:StoppingCode:EndVolume:EndX:EndY:EndZ:EndT:genId:genP:genE:genX:genY:genZ:genCosTh:genPhi:genTime::driftTime:driftDist" );
      } else if(geom->hasElement<ITracker>()) {
        _tNtup        = tfs->make<TNtuple>( "CellHits", "Cell Ntuple",
                                            "evt:run:time:eDep:lay:superlay:cellId:MChitX:MChitY:wireZMC:trkId:pdgId:isGen:StepFromEva:EvaIsGen:EvaCreationCode:P:CreationCode:StartVolume:StartX:StartY:StartZ:StartT:StoppingCode:EndVolume:EndX:EndY:EndZ:EndT:genId:genP:genE:genX:genY:genZ:genCosTh:genPhi:genTime:driftTime:driftDist" );
      }
      _cNtup        = tfs->make<TNtuple>( "CaloHits", "Calo Ntuple",
                                          "evt:run:time:eDep:rad:crId:crVane:crX:crY:crZ:trkId:pdgId:isGen:StepFromEva:EvaIsGen:EvaCreationCode:P:CreationCode:StartVolume:StartX:StartY:StartZ:StartT:StoppingCode:EndVolume:EndX:EndY:EndZ:EndT:cryFrameEnterX:cryFrameEnterY:cryFrameEnterZ:genId:genP:genE:genX:genY:genZ:genCosTh:genPhi:genTime" );

      if (_doStoppingTarget) {
        _tgtNtup      = tfs->make<TNtuple>( "ST", "Particle dead in ST ntuple",
                                            "evt:run:time:x:y:z:isGen:pdgId:trkId:stVol:isStopped");
      }
    }


    if (_doStoppingTarget) doStoppingTarget(evt);

    if (geom->hasElement<ITracker>()) {
      doITracker(evt, _skipEvent);
    }
    if (geom->hasElement<TTracker>()) {
      doTracker(evt, _skipEvent);
    }
    doCalorimeter(evt, _skipEvent);
    
  } // end of analyze

  void BkgRates::endJob() {
    cout << "BkgRates::endJob Number of events skipped "
         << "due to G4 completion status: "
         << _nBadG4Status
         << "\nBkgRates::endJob Number of overflow events "
         << "due to too many particles in G4: "
         << _nOverflow
         << "\nBkgRates::endJob Number of events with killed particles "
         << "due to too many steps in G4: "
         << _nKilled
         << "\nBkgRates::endJob total CpuTime "
         << _totalcputime
         << "\nBkgRates::endJob total RealTime "
         << _totalrealtime
         << endl;
  }
    
  void BkgRates::doTracker(art::Event const& evt, bool skip) {

    if (skip) return;

    const Tracker& tracker = getTrackerOrThrow();

    art::Handle<StrawHitCollection> pdataHandle;
    evt.getByLabel(_makerModuleLabel,pdataHandle);
    StrawHitCollection const* hits = pdataHandle.product();

    art::Handle<StrawHitMCTruthCollection> truthHandle;
    evt.getByLabel(_makerModuleLabel,truthHandle);
    StrawHitMCTruthCollection const* hits_truth = truthHandle.product();

    art::Handle<PtrStepPointMCVectorCollection> mcptrHandle;
    evt.getByLabel(_makerModuleLabel,"StrawHitMCPtr",mcptrHandle);
    PtrStepPointMCVectorCollection const* hits_mcptr = mcptrHandle.product();

    if (!(hits->size() == hits_truth->size() &&
          hits_mcptr->size() == hits->size() ) ) {
      throw cet::exception("RANGE")
        << "Strawhits: " << hits->size()
        << " MCTruthStrawHits: " << hits_truth->size()
        << " MCPtr: " << hits_mcptr->size();
    }

    art::Handle<GenParticleCollection> genParticles;
    evt.getByLabel(_generatorModuleLabel, genParticles);

    art::Handle<SimParticleCollection> simParticles;
    evt.getByLabel(_g4ModuleLabel, simParticles);

    art::Handle<PhysicalVolumeInfoCollection> volumes;
    evt.getRun().getByLabel(_g4ModuleLabel, volumes);

    bool haveSimPart = ( simParticles.isValid() && volumes.isValid() );

    // Other files might have empty collections.
    if ( haveSimPart ){
      haveSimPart = !(simParticles->empty() || volumes->empty());
    }

    size_t nStrawPerEvent = hits->size();

    for (size_t i=0; i<nStrawPerEvent; ++i) {

      float tntpArray[44];
      int idx(0);

      // Access data
      StrawHit             const&      hit(hits->at(i));
      StrawHitMCTruth      const&    truth(hits_truth->at(i));
      PtrStepPointMCVector const&    mcptr(hits_mcptr->at(i));

      //Skip the straw if the energy of the hit is smaller than the minimum required
      if (hit.energyDep() < _minimumEnergyTracker) continue;

      tntpArray[idx++] = evt.id().event(); //leaf 1
      tntpArray[idx++] = evt.run(); //leaf 2

      tntpArray[idx++] = hit.time(); //leaf 3
      tntpArray[idx++] = hit.dt(); //leaf 4
      tntpArray[idx++] = hit.energyDep(); //leaf 5

      //Get hit straw
      StrawIndex si = hit.strawIndex();
      Straw str = tracker.getStraw(si);
      StrawId sid = str.id();
      LayerId lid = sid.getLayerId();
      DeviceId did = sid.getDeviceId();
      SectorId secid = sid.getSectorId();

      tntpArray[idx++] = lid.getLayer(); //leaf 6
      tntpArray[idx++] = did; //leaf 7
      tntpArray[idx++] = secid.getSector(); //leaf 8
      tntpArray[idx++] = sid.getStraw(); //leaf 9



      //Get coordinates of the hit:

      //X, Y and Z coordinate of the straw middle point
      const CLHEP::Hep3Vector stMidPoint3 = str.getMidPoint();

      //direction of the straw
      const CLHEP::Hep3Vector stDirection3 = str.getDirection();

      //Position along the wire using mctruth info
      double vMC = truth.distanceToMid();

      //Position along the wire using dt and propagation velocity (c)
      const double signalVelocity = 299.792458; // mm/ns
      double v = 10e4 * hit.dt()/(2*signalVelocity);

      const CLHEP::Hep3Vector HitPoint = stMidPoint3 + (v/stDirection3.mag())*stDirection3;
      const CLHEP::Hep3Vector MCHitPoint = stMidPoint3 + (vMC/stDirection3.mag())*stDirection3;

      if (fabs(v) > str.getHalfLength()) {
        if (_diagLevel > 0) cout << "Position along the wire bigger than halflength" << endl;
      }

      tntpArray[idx++] = MCHitPoint.getX(); //leaf 10
      tntpArray[idx++] = MCHitPoint.getY(); //leaf 11
      tntpArray[idx++] = v; //leaf 12
      tntpArray[idx++] = vMC; //leaf 13
      tntpArray[idx++] = stMidPoint3.getZ(); //leaf 14

      //Get related G4 hits to identify the track.


      CLHEP::Hep3Vector const& strDir = str.direction();
      double strRadius = str.getRadius();
      bool foundTrack = false;
      if ( haveSimPart ) {
	for (size_t j = 0; j < mcptr.size(); ++j) {
	 
	  if (foundTrack) break;
	  
	  StepPointMC const& mchit = *mcptr[j];
	  SimParticle const& sim = simParticles->at(mchit.trackId());
	  
	  CLHEP::Hep3Vector const& StartPos = sim.startPosition();
	  LinePointPCA lppca(stMidPoint3, strDir, StartPos);
	  double insideDistance = lppca.dca();
	  if (insideDistance < strRadius) continue;
	  
	  tntpArray[idx++] = mchit.trackId().asInt(); //leaf 15
	  tntpArray[idx++] = sim.pdgId();//leaf 16
	  tntpArray[idx++] = sim.fromGenerator();//leaf 17
	  
	  int steps = 0;
	  bool foundEva = false;
	  int evaIsGen = 0;
	  int evaCreationCode = 0;
	  SimParticle& tempSim = const_cast<SimParticle&>(sim);
	  while (!foundEva) {
	    if (!(tempSim.hasParent()) ) {
	      foundEva = true;
	      if ( tempSim.fromGenerator()) {
		evaIsGen = 1;
	      }
	      evaCreationCode = tempSim.creationCode();
	      break;
	    }
	    
	    tempSim = const_cast<SimParticle&>(*sim.parent());
	    steps++;
	  }
	  
	  tntpArray[idx++] = steps; // leaf 18
	  tntpArray[idx++] = evaIsGen; // leaf 19
	  tntpArray[idx++] = evaCreationCode; // leaf 20
	  
	  
	  tntpArray[idx++] = sim.startMomentum().vect().mag(); // leaf 21
	  tntpArray[idx++] = sim.creationCode(); // leaf 22
	  tntpArray[idx++] = sim.startVolumeIndex(); // leaf 23
	  tntpArray[idx++] = sim.startPosition().x(); // leaf 24
	  tntpArray[idx++] = sim.startPosition().y(); // leaf 25
	  tntpArray[idx++] = sim.startPosition().z(); // leaf 26
	  tntpArray[idx++] = sim.startGlobalTime(); // leaf 27

	  tntpArray[idx++] = sim.stoppingCode(); // leaf 28
	  tntpArray[idx++] = sim.endVolumeIndex(); // leaf 29
	  tntpArray[idx++] = sim.endPosition().x(); // leaf 30
	  tntpArray[idx++] = sim.endPosition().y(); // leaf 31
	  tntpArray[idx++] = sim.endPosition().z(); // leaf 32
	  tntpArray[idx++] = sim.endGlobalTime(); // leaf 33
	  foundTrack = true;	  
	  
	}
      } else if ( !haveSimPart) {
	
	tntpArray[idx++] = 0; // leaf 15
	tntpArray[idx++] = 0; // leaf 16
	tntpArray[idx++] = 0; // leaf 17
	tntpArray[idx++] = 0; // leaf 18
	tntpArray[idx++] = 0; // leaf 19
	tntpArray[idx++] = 0; // leaf 20
	tntpArray[idx++] = 0; // leaf 21
	tntpArray[idx++] = 0; // leaf 22
	tntpArray[idx++] = 0; // leaf 23
	tntpArray[idx++] = 0; // leaf 24
	tntpArray[idx++] = 0; // leaf 25
	tntpArray[idx++] = 0; // leaf 26
	tntpArray[idx++] = 0; // leaf 27
	tntpArray[idx++] = 0; // leaf 28
	tntpArray[idx++] = 0; // leaf 29
	tntpArray[idx++] = 0; // leaf 30
	tntpArray[idx++] = 0; // leaf 31
	tntpArray[idx++] = 0; // leaf 32
	tntpArray[idx++] = 0; // leaf 33
	
      }
      
      size_t ngen = genParticles->size();
      if (ngen>1) {
        cout << "The plugin is supposed to analyze single background rates,"
             << "with one generated particle per event"
             << "\nThis event has more than one genparticle. Only the "
             << "first one will be stored" << endl;
      }

      SimParticleCollection::key_type idxInSim = SimParticleCollection::key_type(1);
      SimParticle const& geninSim = simParticles->at(idxInSim);
      if (!geninSim.fromGenerator()) {
        cout << "Watch out. First particle is not from generator. What's happening?" << endl;
      }
      
      if (ngen > 0) {
        GenParticle const& gen = genParticles->at(0);
        tntpArray[idx++] = gen.generatorId().id();//leaf 34
        tntpArray[idx++] = gen.momentum().vect().mag();//leaf 35
        tntpArray[idx++] = gen.momentum().e();//leaf 36
        tntpArray[idx++] = gen.position().x();//leaf 37
        tntpArray[idx++] = gen.position().y();//leaf 38
        tntpArray[idx++] = gen.position().z();//leaf 39
        tntpArray[idx++] = gen.momentum().cosTheta();//leaf 40
        tntpArray[idx++] = gen.momentum().phi();//leaf 41
        tntpArray[idx++] = gen.time();//leaf 42
      } else if ( ngen == 0 ) {
        tntpArray[idx++] = 0;//leaf 34
        tntpArray[idx++] = 0;//leaf 35
        tntpArray[idx++] = 0;//leaf 36
        tntpArray[idx++] = 0;//leaf 37
        tntpArray[idx++] = 0;//leaf 38
        tntpArray[idx++] = 0;//leaf 39
        tntpArray[idx++] = 0;//leaf 40
        tntpArray[idx++] = 0;//leaf 41
        tntpArray[idx++] = 0;//leaf 42
      }

      // Store MC truth data
      tntpArray[idx++] = truth.driftTime(); //leaf 43
      tntpArray[idx++] = truth.driftDistance(); //leaf 44

      _tNtup->Fill(tntpArray);

    } //end of Strawhits loop
  
  } // end of doTracker



  void BkgRates::doITracker(art::Event const& evt, bool skip) {

    if (skip) return;

    const Tracker& tracker = getTrackerOrThrow();
    art::Handle<StrawHitCollection> pdataHandle;
    evt.getByLabel(_makerModuleLabel,pdataHandle);
    StrawHitCollection const* hits = pdataHandle.product();

    // Get the persistent data about the StrawHitsMCTruth.
    art::Handle<StrawHitMCTruthCollection> truthHandle;
    evt.getByLabel(_makerModuleLabel,truthHandle);
    StrawHitMCTruthCollection const* hits_truth = truthHandle.product();

    // Get the persistent data about pointers to StepPointMCs
    art::Handle<PtrStepPointMCVectorCollection> mcptrHandle;
    evt.getByLabel(_makerModuleLabel,"StrawHitMCPtr",mcptrHandle);
    PtrStepPointMCVectorCollection const* hits_mcptr = mcptrHandle.product();

    if (!(hits->size() == hits_truth->size() &&
          hits_mcptr->size() == hits->size() ) ) {
      throw cet::exception("RANGE")
        << "Strawhits: " << hits->size()
        << " MCTruthStrawHits: " << hits_truth->size()
        << " MCPtr: " << hits_mcptr->size();
    }

    // Get handles to the generated and simulated particles.
    art::Handle<GenParticleCollection> genParticles;
    evt.getByLabel(_generatorModuleLabel, genParticles);

    art::Handle<SimParticleCollection> simParticles;
    evt.getByLabel(_g4ModuleLabel, simParticles);

    // Handle to information about G4 physical volumes.
    art::Handle<PhysicalVolumeInfoCollection> volumes;
    evt.getRun().getByLabel(_g4ModuleLabel, volumes);

    // Some files might not have the SimParticle and volume information.
    bool haveSimPart = ( simParticles.isValid() && volumes.isValid() );

    // Other files might have empty collections.
    if ( haveSimPart ){
      haveSimPart = !(simParticles->empty() || volumes->empty());
    }

    size_t nStrawPerEvent = hits->size();

    for (size_t i=0; i<nStrawPerEvent; ++i) {

      // Access data
      StrawHit             const&  hit(hits->at(i));
      StrawHitMCTruth      const&  truth(hits_truth->at(i));
      PtrStepPointMCVector const&  mcptr(hits_mcptr->at(i));

      double hitEnergy = hit.energyDep();

      //Skip the straw if the energy of the hit is smaller than the minimum required
      if (hitEnergy < _minimumEnergyTracker) continue;

      //Get hit straw
      StrawIndex si = hit.strawIndex();
      const Straw & str = tracker.getStraw(si);
      const Cell & cell = static_cast<const Cell&>( str );


      // cout << "Getting informations about cells" << endl;

      int sid = cell.Id().getCell();
      int lid = cell.Id().getLayer();
      int did = cell.Id().getLayerId().getSuperLayer();

      const CLHEP::Hep3Vector stMidPoint3 = str.getMidPoint();

      //time of the hit
      double hitTime = hit.time();

      //direction of the straw
      const CLHEP::Hep3Vector stDirection3 = str.getDirection();

      // cout << "Reading MCtruth info" << endl;

      // Get MC truth data
      double driftTime = truth.driftTime();
      double driftDistance = truth.driftDistance();

      //Position along the wire using mctruth info
      double vMC = truth.distanceToMid();

      const CLHEP::Hep3Vector MCHitPoint = stMidPoint3 + (vMC/stDirection3.mag())*stDirection3;

      //  cout << "Filling ntupla" << endl;

      float tntpArray[40];
      int idx(0);
      tntpArray[idx++] = evt.id().event(); //1
      tntpArray[idx++] = evt.run();//2
      tntpArray[idx++] = hitTime;//3
      tntpArray[idx++] = hitEnergy;//4
      tntpArray[idx++] = lid;//5
      tntpArray[idx++] = did;//6
      tntpArray[idx++] = sid;//7
      tntpArray[idx++] = MCHitPoint.getX();//8
      tntpArray[idx++] = MCHitPoint.getY();//9
      tntpArray[idx++] = vMC;//10



      if ( haveSimPart ){
	//Find the first stepPointMC by time of flight
	double temptime = 10e9;
	size_t jfirst = 0;
	for (size_t j = 0; j < mcptr.size(); ++j) {
	  
	  if (mcptr[j]->time() < temptime) {
	    temptime = mcptr[j]->time(); 
	    jfirst = j;
	  }
	}
	
	StepPointMC const& mchit = *mcptr[jfirst];
	
	// The simulated particle that made this hit.
	SimParticleCollection::key_type trackId(mchit.trackId());
		
	SimParticle const& sim = simParticles->at(trackId);
	
	tntpArray[idx++] = trackId.asInt(); //leaf 11
	tntpArray[idx++] = sim.pdgId();//leaf 12
	tntpArray[idx++] = sim.fromGenerator();//leaf 13
	
	int steps = 0;
	bool foundEva = false;
	int evaIsGen = 0;
	int evaCreationCode = 0;
	SimParticle& tempSim = const_cast<SimParticle&>(sim);
	while (!foundEva) {
	  if (!(tempSim.hasParent()) ) {
	    foundEva = true;
	    if ( tempSim.fromGenerator()) {
	      evaIsGen = 1;
	    }
	    evaCreationCode = tempSim.creationCode();
	    break;
	  }
	  
	  tempSim = const_cast<SimParticle&>(*sim.parent());
	  steps++;
	}
	
	tntpArray[idx++] = steps; // leaf 14
	tntpArray[idx++] = evaIsGen; // leaf 15
	tntpArray[idx++] = evaCreationCode; // leaf 16
	
	tntpArray[idx++] = sim.startMomentum().vect().mag(); // leaf 17
	tntpArray[idx++] = sim.creationCode(); // leaf 18
	tntpArray[idx++] = sim.startVolumeIndex(); // leaf 19
	tntpArray[idx++] = sim.startPosition().x(); // leaf 20
	tntpArray[idx++] = sim.startPosition().y(); // leaf 21
	tntpArray[idx++] = sim.startPosition().z(); // leaf 22
	tntpArray[idx++] = sim.startGlobalTime(); // leaf 23
	
	tntpArray[idx++] = sim.stoppingCode(); // leaf 24
	tntpArray[idx++] = sim.endVolumeIndex(); // leaf 25
	tntpArray[idx++] = sim.endPosition().x(); // leaf 26
	tntpArray[idx++] = sim.endPosition().y(); // leaf 27
	tntpArray[idx++] = sim.endPosition().z(); // leaf 28
	tntpArray[idx++] = sim.endGlobalTime(); // leaf 29

      } else if ( !haveSimPart) {

	tntpArray[idx++] = 0; // leaf 11
	tntpArray[idx++] = 0; // leaf 12
	tntpArray[idx++] = 0; // leaf 13
	tntpArray[idx++] = 0; // leaf 14
	tntpArray[idx++] = 0; // leaf 15
	tntpArray[idx++] = 0; // leaf 16
	tntpArray[idx++] = 0; // leaf 17
	tntpArray[idx++] = 0; // leaf 18
	tntpArray[idx++] = 0; // leaf 19
	tntpArray[idx++] = 0; // leaf 20
	tntpArray[idx++] = 0; // leaf 21
	tntpArray[idx++] = 0; // leaf 22
	tntpArray[idx++] = 0; // leaf 23
	tntpArray[idx++] = 0; // leaf 24
	tntpArray[idx++] = 0; // leaf 25
	tntpArray[idx++] = 0; // leaf 26
	tntpArray[idx++] = 0; // leaf 27
	tntpArray[idx++] = 0; // leaf 28
	tntpArray[idx++] = 0; // leaf 29


      }
      
      size_t ngen = genParticles->size();
      if (ngen>1) {
        cout << "The plugin is supposed to analyze single background rates,"
             << "with one generated particle per event"
             << "\nThis event has more than one genparticle. Only the "
             << "first one will be stored" << endl;
      }

      if (ngen > 0) {
	SimParticleCollection::key_type idxInSim = SimParticleCollection::key_type(1);
	SimParticle const& geninSim = simParticles->at(idxInSim);
	if (!geninSim.fromGenerator()) {
	  cout << "Watch out. First particle is not from generator. What's happening?" << endl;
	}
        GenParticle const& gen = genParticles->at(0);
        tntpArray[idx++] = gen.generatorId().id(); //30
        tntpArray[idx++] = gen.momentum().vect().mag(); //31
        tntpArray[idx++] = gen.momentum().e(); //32
        tntpArray[idx++] = gen.position().x(); //33
        tntpArray[idx++] = gen.position().y(); //34
        tntpArray[idx++] = gen.position().z(); //35
        tntpArray[idx++] = gen.momentum().cosTheta(); //36
        tntpArray[idx++] = gen.momentum().phi(); //37
        tntpArray[idx++] = gen.time(); //38
      } else if ( ngen == 0 ) {
        tntpArray[idx++] = 0; //30
        tntpArray[idx++] = 0; //31
        tntpArray[idx++] = 0; //32
        tntpArray[idx++] = 0; //33
        tntpArray[idx++] = 0; //34
        tntpArray[idx++] = 0; //35
        tntpArray[idx++] = 0; //36
        tntpArray[idx++] = 0; //37
        tntpArray[idx++] = 0; //38
      }

      tntpArray[idx++] = driftTime; //39
      tntpArray[idx++] = driftDistance; //40
      
      _tNtup->Fill(tntpArray);
      
    } //end of Cellhits loop

  } // end of doITracker

  void BkgRates::doCalorimeter(art::Event const& evt, bool skip) {

    if (skip) return;

    const double CrDensity = 7.4*CLHEP::g/CLHEP::cm3;

    //Get handle to the calorimeter
    art::ServiceHandle<GeometryService> geom;
    if( ! geom->hasElement<Calorimeter>() ) return;
    GeomHandle<Calorimeter> cg;
    double CrSize = cg->crystalHalfSize();
    double CrLength = cg->crystalHalfLength();
    double CrVolumeCm3 = (CrSize*CrSize*CrLength)*CLHEP::cm3/CLHEP::mm3; //factor 1000 because they are in mm
    double CrMassKg = CrDensity*CrVolumeCm3*CLHEP::kg/CLHEP::g;

    cout << CrSize << '\t' << CrLength << '\t' << CrVolumeCm3
         << '\t' << CrDensity << '\t' << CrMassKg << endl;

    // Get handles to calorimeter collections
    art::Handle<CaloHitCollection> caloHits;
    art::Handle<CaloCrystalHitCollection>  caloCrystalHits;
    // Get the persistent data about pointers to StepPointMCs
    art::Handle<PtrStepPointMCVectorCollection> mcptrHandle;
    //    art::Handle<StepPointMCCollection> steps;

    evt.getByLabel(_caloReadoutModuleLabel,"CaloHitMCCrystalPtr",mcptrHandle);
    //    evt.getByLabel(_g4ModuleLabel,"calorimeter",steps);
    evt.getByLabel(_caloReadoutModuleLabel, caloHits);
    evt.getByLabel(_caloCrystalModuleLabel, caloCrystalHits);

    PtrStepPointMCVectorCollection const* hits_mcptr = mcptrHandle.product();
    if (!( caloHits.isValid())) {
      return;
    }

    if (!caloCrystalHits.isValid()) {
      cout << "NO CaloCrystalHits" << endl;
      return;
    }

    // Get handles to the generated and simulated particles.
    art::Handle<GenParticleCollection> genParticles;
    evt.getByLabel(_generatorModuleLabel, genParticles);

    art::Handle<SimParticleCollection> simParticles;
    evt.getByLabel(_g4ModuleLabel, simParticles);

    // Handle to information about G4 physical volumes.
    art::Handle<PhysicalVolumeInfoCollection> volumes;
    evt.getRun().getByLabel(_g4ModuleLabel, volumes);

    // Some files might not have the SimParticle and volume information.
    bool haveSimPart = ( simParticles.isValid() && volumes.isValid() );
    // Other files might have empty collections.
    if ( haveSimPart ){
      haveSimPart = !(simParticles->empty() || volumes->empty());
    }

    if (caloCrystalHits->size()<=0) return;

    for ( size_t i=0; i<caloCrystalHits->size(); ++i ) {
      
      CaloCrystalHit const & hit = (*caloCrystalHits).at(i);
      if (hit.energyDep() < _minimumEnergyCalo) continue;
      
      std::vector<art::Ptr<CaloHit> > const & ROIds  = hit.readouts();
      
      //      cout << "Event " << evt.id().event() << ". In the caloCrystalHits there are " << ROIds.size() << " RO associated" << endl;
      
      if (ROIds.size() < 1) {
	//          cout << " Event n. " << evt.id().event()
	//   << " \t got crystal hits but not ROhits"
	//     << '\t' <<  caloCrystalHits->size()
	//   << '\t' << ROIds.size() << endl;
	continue;
      }
      
      float cntpArray[41];
      int idx(0);
      
      
      CLHEP::Hep3Vector firstHitPos(0,0,0);
      CLHEP::Hep3Vector cryFrame(0,0,0);
      
      
      size_t collectionPosition = ROIds.at(0).key();
      CaloHit const & thehit = *ROIds.at(0);
      
      CLHEP::Hep3Vector cryCenter =  cg->getCrystalOriginByRO(thehit.id());
      int vane = cg->getVaneByRO(thehit.id());
      cntpArray[idx++] = evt.id().event();
      cntpArray[idx++] = evt.run();
      cntpArray[idx++] = hit.time();
      cntpArray[idx++] = hit.energyDep();
      double CrEjoule = hit.energyDep()*CLHEP::joule/CLHEP::megaelectronvolt;
      double dose = CrEjoule/CrMassKg;
      cntpArray[idx++] = dose;
      cntpArray[idx++] = cg->getCrystalByRO(thehit.id());
      cntpArray[idx++] = vane;
      cntpArray[idx++] = cryCenter.getX() + 3904.;  //value used to shift in tracker coordinate system
      cntpArray[idx++] = cryCenter.getY();
      cntpArray[idx++] = cryCenter.getZ() - 10200;  //value used to shift in tracker coordinate system
      
      PtrStepPointMCVector const & mcptr(hits_mcptr->at(collectionPosition));
      
      StepPointMC const& mchit = *mcptr[0];
      
      
      if (haveSimPart) {
	// The simulated particle that made this hit.
	SimParticleCollection::key_type trackId(mchit.trackId());
	
	cntpArray[idx++] = trackId.asInt();
	
	SimParticle const& sim = simParticles->at(trackId);
	
	cntpArray[idx++] = sim.pdgId();
	cntpArray[idx++] = sim.fromGenerator();
	
	if (sim.stoppingCode() == ProcessCode::mu2eMaxSteps) {
	  cout << "Track " << sim.pdgId() << "  dies at "
	       << sim.endGlobalTime() << endl;
	}
	
	int steps = 0;
	bool foundEva = false;
	int evaIsGen = 0;
	int evaCreationCode = 0;
	SimParticle& tempSim = const_cast<SimParticle&>(sim);
	while (!foundEva) {
	  if (!(tempSim.hasParent()) ) {
	    foundEva = true;
	    if ( tempSim.fromGenerator()) {
	      evaIsGen = 1;
	    }
	    evaCreationCode = tempSim.creationCode();
	    break;
	  }
	  
	  tempSim = const_cast<SimParticle&>(*sim.parent());
	  steps++;
	}
	
	cntpArray[idx++] = steps;
	cntpArray[idx++] = evaIsGen;
	cntpArray[idx++] = evaCreationCode;
	cntpArray[idx++] = sim.startMomentum().vect().mag();
	cntpArray[idx++] = sim.creationCode();
	cntpArray[idx++] = sim.startVolumeIndex();
	cntpArray[idx++] = sim.startPosition().x();
	cntpArray[idx++] = sim.startPosition().y();
	cntpArray[idx++] = sim.startPosition().z();
	cntpArray[idx++] = sim.startGlobalTime();

	cntpArray[idx++] = sim.stoppingCode();
	cntpArray[idx++] = sim.endVolumeIndex();
	cntpArray[idx++] = sim.endPosition().x();
	cntpArray[idx++] = sim.endPosition().y();
	cntpArray[idx++] = sim.endPosition().z();
	cntpArray[idx++] = sim.endGlobalTime(); 

	firstHitPos = mchit.position();
	cryFrame = cg->toCrystalFrame(thehit.id(), firstHitPos);
	cntpArray[idx++] = cryFrame.x();
	cntpArray[idx++] = cryFrame.y();
	cntpArray[idx++] = cryFrame.z();                        
	
      } else if (!haveSimPart) {
	
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	cntpArray[idx++] = 0;
	
      }
      
      size_t ngen = genParticles->size();
      if (ngen>1) {
	cout << "The plugin is supposed to analyze single background rates,"
	     << "with one generated particle per event"
	     << "\nThis event has more than one genparticle. Only the "
	     << "first one will be stored" << endl;
      }
      if (ngen > 0) {
	GenParticle const& gen = genParticles->at(0);
	cntpArray[idx++] = gen.generatorId().id();
	cntpArray[idx++] = gen.momentum().vect().mag();
	cntpArray[idx++] = gen.momentum().e();
	cntpArray[idx++] = gen.position().x();
	cntpArray[idx++] = gen.position().y();
	cntpArray[idx++] = gen.position().z();
	cntpArray[idx++] = gen.momentum().cosTheta();
	cntpArray[idx++] = gen.momentum().phi();
	cntpArray[idx++] = gen.time();
      } else if ( ngen == 0 ) {
	cntpArray[idx++] = 0.;
	cntpArray[idx++] = 0.;
	cntpArray[idx++] = 0.;
	cntpArray[idx++] = 0.;
	cntpArray[idx++] = 0.;
	cntpArray[idx++] = 0.;
	cntpArray[idx++] = 0.;
	cntpArray[idx++] = 0.;
	cntpArray[idx++] = 0.;
      }
      _cNtup->Fill(cntpArray);
    }
  } // end of doCalorimeter
  
  void BkgRates::doStoppingTarget(const art::Event& event) {

    bool generatedStopped = false;

    // Find original G4 steps in the stopping target
    art::Handle<StepPointMCCollection> sthits;
    event.getByLabel(_g4ModuleLabel,"stoppingtarget",sthits);

    // SimParticles container
    art::Handle<SimParticleCollection> simParticles;
    event.getByLabel(_g4ModuleLabel, simParticles);
    if( !(simParticles.isValid()) || simParticles->empty() ) return;

    art::Handle<PhysicalVolumeInfoCollection> volumes;
    event.getRun().getByLabel(_g4ModuleLabel, volumes);

    set<SimParticleCollection::key_type> stoppedtracks;

    // Loop over all hits in the stopping target
    for ( size_t i=0; i<sthits->size(); ++i ){

      // This is G4 hit (step) in the target
      const StepPointMC& hit = (*sthits)[i];

      SimParticleCollection::key_type trackId = hit.trackId();

      SimParticle const* sim = simParticles->getOrNull(trackId);
      if( !sim ) continue;

      PhysicalVolumeInfo const& volInfo = volumes->at(sim->endVolumeIndex());

      if (!(sim->fromGenerator())) continue;

      if (skipPDG.find(sim->pdgId()) != skipPDG.end()) {
        if ( volInfo.name() == "TargetFoil_" ) {
        generatedStopped = true;

        }
      }

      if( stoppedtracks.insert(trackId).second ) {
        float tgtntpArray[11];
        int idx(0);
        tgtntpArray[idx++] = event.id().event();
        tgtntpArray[idx++] = event.run();
        tgtntpArray[idx++] = sim->endGlobalTime();
        tgtntpArray[idx++] = sim->endPosition().x();
        tgtntpArray[idx++] = sim->endPosition().y();
        tgtntpArray[idx++] = sim->endPosition().z();
        tgtntpArray[idx++] = sim->fromGenerator();
        tgtntpArray[idx++] = sim->pdgId();
        tgtntpArray[idx++] = trackId.asInt();
        tgtntpArray[idx++] = sim->endVolumeIndex();
        tgtntpArray[idx++] = (volInfo.name() == "TargetFoil_");
        
        _tgtNtup->Fill(tgtntpArray);

      }
    }
   
    _skipEvent = generatedStopped;
  }  // end doStoppingTarget
}

using mu2e::BkgRates;
DEFINE_ART_MODULE(BkgRates);

