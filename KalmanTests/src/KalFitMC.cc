//
// MC functions associated with KalFit
// $Id: KalFitMC.cc,v 1.43 2012/12/13 21:13:58 brownd Exp $
// $Author: brownd $ 
// $Date: 2012/12/13 21:13:58 $
//
//geometry
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/getTrackerOrThrow.hh"
#include "GeometryService/inc/VirtualDetector.hh"
#include "GeometryService/inc/DetectorSystem.hh"
#include "BFieldGeom/inc/BFieldConfig.hh"
// services
#include "ConditionsService/inc/GlobalConstantsHandle.hh"
#include "ConditionsService/inc/ParticleDataTable.hh"
#include "art/Framework/Services/Optional/TFileService.h"
// data
#include "art/Framework/Principal/Event.h"
#include "RecoDataProducts/inc/StrawHitCollection.hh"
#include "RecoDataProducts/inc/StrawHit.hh"
#include "MCDataProducts/inc/StrawHitMCTruth.hh"
#include "MCDataProducts/inc/PtrStepPointMCVectorCollection.hh"
#include "MCDataProducts/inc/StrawHitMCTruthCollection.hh"
#include "MCDataProducts/inc/StepPointMCCollection.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"
#include "MCDataProducts/inc/VirtualDetectorId.hh"
// tracker
#include "TrackerGeom/inc/Tracker.hh"
#include "TrackerGeom/inc/Straw.hh"
// BaBar
#include "BaBar/BaBar.hh"
#include "KalmanTests/inc/TrkDef.hh"
#include "KalmanTests/inc/TrkStrawHit.hh"
#include "KalmanTests/inc/KalFitMC.hh"
#include "TrkBase/TrkHelixUtils.hh"
#include "TrkBase/TrkHotList.hh"
#include "TrkBase/TrkPoca.hh"
#include "TrkBase/TrkMomCalculator.hh"
#include "BField/BFieldFixed.hh"
#include "KalmanTrack/KalHit.hh"
//CLHEP
#include "CLHEP/Units/PhysicalConstants.h"
// root 
#include "TMath.h"
#include "TFile.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TGMsgBox.h"
#include "TTree.h"
// C++
#include <iostream>
#include <string>
#include <functional>

using namespace std; 
 
 
namespace mu2e 
{
  // comparison functor for ordering step points
  struct timecomp : public binary_function<MCStepItr,MCStepItr, bool> {
    bool operator()(MCStepItr x,MCStepItr y) { return x->time() < y->time(); }
  };
  struct devicecomp : public binary_function<const TrkStrawHit*, const TrkStrawHit*, bool> {
    bool operator()(const TrkStrawHit* x, const TrkStrawHit* y) { 
// predicate on device first, as fltlen might be ambiguous for inactive hots
      if(x->straw().id().getDevice() == y->straw().id().getDevice())
	return x->fltLen() < y->fltLen();
      else
	return(x->straw().id().getDevice() < y->straw().id().getDevice());
    }
  };


 KalFitMC::~KalFitMC(){}
  
  KalFitMC::KalFitMC(fhicl::ParameterSet const& pset) :
    _mcstrawhitslabel(pset.get<std::string>("MCStrawHitsLabel","makeSH")),
    _mcptrlabel(pset.get<std::string>("MCPtrLabel","makeSH")),
    _mcstepslabel(pset.get<std::string>("MCStepsLabel","g4run")),
    _simpartslabel(pset.get<std::string>("SimParticleLabel","g4run")),
    _strawhitslabel(pset.get<std::string>("strawHitsLabel","makeSH")),
    _mintrkmom(pset.get<double>("minTrkMom",60.0)),
    _mct0err(pset.get<double>("mcT0Err",0.1)),
    _mcambig(pset.get<bool>("mcAmbiguity",false)),
    _debug(pset.get<int>("debugLevel",0)),
    _diag(pset.get<int>("diagLevel",1)),
    _minnhits(pset.get<unsigned>("minNHits",10)),
    _maxnhits(pset.get<unsigned>("maxNHits",120)),
    _maxarcgap(pset.get<int>("MaxArcGap",2)),
    _purehits(pset.get<bool>("pureHits",false)),
    _trkdiag(0),_hitdiag(0)
  {
// define the ids of the virtual detectors
    _midvids.push_back(VirtualDetectorId::TT_Mid);
    _midvids.push_back(VirtualDetectorId::TT_MidInner);
    _entvids.push_back(VirtualDetectorId::TT_FrontHollow);
    _entvids.push_back(VirtualDetectorId::TT_FrontPA); 
    _xitvids.push_back(VirtualDetectorId::TT_Back);
  }

// Find MC truth for the given particle entering a given detector(s).  Unfortunately the same logical detector can map
// onto several actual detectors, as they are not allowed to cross volume boundaries, so we have to check against
// several ids.  The track may also pass through this detector more than once, so we return a vector, sorted by time.
  void 
  KalFitMC::findMCSteps(StepPointMCCollection const* mcsteps, cet::map_vector_key const& trkid, std::vector<int> const& vids,
  std::vector<MCStepItr>& steps) {
    steps.clear();
    if(mcsteps != 0){
      // Loop over the step points, and find the one corresponding to the given detector
      for( MCStepItr imcs =mcsteps->begin();imcs!= mcsteps->end();imcs++){
	if(vids.size() == 0 ||  (imcs->trackId() == trkid && std::find(vids.begin(),vids.end(),imcs->volumeId()) != vids.end())){
	  steps.push_back(imcs);
	}
      }
      // sort these in time
      std::sort(steps.begin(),steps.end(),timecomp());
    }
  }

// define seed helix, t0, and hits coming from a given particle using MC truth.  Note that the input
// trkdef object must reference a valid straw hit collection
  bool
  KalFitMC::trkFromMC(cet::map_vector_key const& trkid,TrkDef& mytrk) {
    if(_mcdata._mcsteps == 0)return false;
// preset to failure
    bool retval(false);
    GlobalConstantsHandle<ParticleDataTable> pdt;
    const Tracker& tracker = getTrackerOrThrow();
    unsigned nstraws = mytrk.strawHitCollection()->size();
    if(nstraws >= _minnhits ){
      GeomHandle<DetectorSystem> det;
      GeomHandle<VirtualDetector> vdg;
      std::vector<hitIndex> indices;
// find the mcstep at the middle of the detector
      std::vector<MCStepItr> steps;
      findMCSteps(_mcdata._mcvdsteps,trkid,_midvids,steps);
      if(steps.size() > 0 && vdg->exist(steps[0]->volumeId())&& steps[0]->momentum().mag() > _mintrkmom){
// take the first point
        MCStepItr imcs = steps[0];
        double t0 = imcs->time();
        double charge = pdt->particle(imcs->simParticle()->pdgId()).ref().charge();
        CLHEP::Hep3Vector mom = imcs->momentum();
// need to transform into the tracker coordinate system
        CLHEP::Hep3Vector pos = det->toDetector(imcs->position());
        if(_debug > 1)std::cout << "Defining track at virtual detector id= " << imcs->volumeId()
          << " name " << VirtualDetectorId(imcs->volumeId()).name()
          << " position = " << pos
          << " momentum = " << mom
          << " time = " << t0 << std::endl;
// find the indices of the true primary particle straw hits
        for(size_t istraw=0;istraw<nstraws;istraw++){
          PtrStepPointMCVector const& mcptr(_mcdata._mchitptr->at(istraw));
	  StrawHit const& sh = mytrk.strawHitCollection()->at(istraw);
	  Straw const& straw = tracker.getStraw(sh.strawIndex());
	  int ambig(0);
	  if(_mcambig){
// compute the hit ambiguity using MC truth, using the earliest energy deposit
	    Hep3Vector dir = mcptr[0]->momentum().unit();
	    Hep3Vector start = mcptr[0]->position();
	    Hep3Vector end = start + dir*mcptr[0]->stepLength();
	    Hep3Vector mid = 0.5*(start+end);
	    Hep3Vector mcsep = straw.getMidPoint() - mid;
	    Hep3Vector mcperp = (dir.cross(straw.getDirection())).unit();
	    double dperp = mcperp.dot(mcsep);
	    ambig = dperp > 0 ? 1 : -1; // follow TrkPoca convention
	  }
	  unsigned nprimary(0);
          for( size_t j=0; j<mcptr.size(); ++j ) {
            StepPointMC const& mchit = *mcptr[j];
 // not sure if this works with bkgs merged FIXME!!!!  we also need to treat energy from daughters different
 // from energy from completely different particles
            if( mchit.trackId() == trkid )nprimary++;
          }
// decide if we want all hits with any contribution from this particle, or only pure hits from this particle.
          if( nprimary > 0 && (nprimary == mcptr.size() || (!_purehits) ) )indices.push_back(hitIndex(istraw,ambig));
        }
        if(indices.size() >= _minnhits && indices.size() <= _maxnhits){
// nominal magnetic field.
          GeomHandle<BFieldConfig> bfconf;
          HepVector parvec(5,0);
// Babar interface still uses HepPoint: FIXME!!!!
// use the z component of th enominal field to define the initial helix parameters.  Off-axis terms are
// ignored anyways by this interface
          double hflt(0.0);
          TrkHelixUtils::helixFromMom( parvec, hflt, 
            HepPoint(pos.x(),pos.y(),pos.z()),
            mom,charge,bfconf->getDSUniformValue().z());
  // dummy covariance matrix; this should be set according to measured values, FIXME!!!!!
          HepSymMatrix dummy(5,1); 
          dummy(1,1)=1.; dummy(2,2)=0.1*0.1;dummy(3,3)=1e-2*1e-2;
          dummy(4,4)=1.; dummy(5,5)=0.1*0.1;
	  TrkFitDirection::FitDirection fdir = parvec[4]>0 ?
	  TrkFitDirection::downstream : TrkFitDirection::upstream;
	  TrkParticle::type ptype = (TrkParticle::type)imcs->simParticle()->pdgId();
          mytrk = TrkDef(mytrk.strawHitCollection(),indices,parvec,dummy,TrkParticle(ptype),TrkFitDirection(fdir));
	  mytrk.setT0(TrkT0(t0,_mct0err));
          retval = true;
        }
      }
    }
    return retval;
  }
 
  void KalFitMC::hitDiag(const TrkStrawHit* strawhit) {
    if(_hitdiag == 0)createHitDiag();
// straw info
    _edep = strawhit->strawHit().energyDep();
//    CLHEP::Hep3Vector dmid = strawhit->wirePosition()-strawhit->straw().getMidPoint(); 
// trkhit  info
    _rdrift = strawhit->driftRadius();
    _rdrifterr = strawhit->driftRadiusErr();
    _amb = strawhit->ambig();
    CLHEP::Hep3Vector shpos;
    strawhit->hitPosition(shpos);
    _shpos = shpos;
//    _shposs.push_back(shpos);
    _dmid = strawhit->timeDiffDist();
    _dmiderr = strawhit->timeDiffDistErr();
// residual info
    double resid,residerr;
    if(strawhit->resid(resid,residerr,true)){
      _resid = resid;
      _residerr = residerr;
    } else {
      _resid = _residerr = -100.0;
    }
    _hitt0 = strawhit->hitT0()._t0;
    _hitt0err = strawhit->hitT0()._t0err;
    _hflt = strawhit->hitLen();
    _trkflt = strawhit->fltLen();
    _active = strawhit->isActive();
    _use = strawhit->usability();
// mc information
    unsigned istraw = strawhit->index();
    StrawHitMCTruth const& mcstrawhit = (_mcdata._mcstrawhits->at(istraw));
    _mcrdrift = mcstrawhit.driftDistance();
    _mcdmid = mcstrawhit.distanceToMid();
    CLHEP::Hep3Vector mcpos;
    PtrStepPointMCVector const& mcptr(_mcdata._mchitptr->at(istraw));
    _nmcsteps = mcptr.size();
    double esum(0.0);
    double mct0(0.0);
    _pdist = 0.0;
    _pperp = 0.0;
    _pmom = 0.0;
    for( size_t imc=0; imc< _nmcsteps; ++imc ) {
      StepPointMC const& mchit = *mcptr[imc];
      if( mchit.trackId().asInt() == 1 ){
        double edep = mchit.eDep();
        esum += edep;
        mcpos += mchit.position()*edep;
        mct0 += mchit.time()*edep;
        CLHEP::Hep3Vector dprod = mchit.position()-mchit.simParticle()->startPosition();
        _pdist += dprod.mag()*edep;
        _pperp += dprod.perp(Hep3Vector(0.0,0.0,1.0))*edep;
        _pmom = mchit.momentum().mag()*edep;
      }
    }
    if(esum > 0.0){
      mcpos /= esum;
      mct0 /= esum;
      _pdist /= esum;
      _pperp /= esum;
      _pmom /= esum;
    }
    _mchpos = mcpos;
    _mchitt0 = mct0;
    _hitdiag->Fill();
  }
  
  void
  KalFitMC::fillHitsVector(const KalRep* krep,std::vector<const TrkStrawHit*> & hits) {
    hits.clear();
    if(krep != 0) {
  // extract the hits from the kalrep and perform diagnstics
      const TrkHotList* hots = krep->hotList();
      hits.reserve(hots->nHit());
      for(TrkHotList::hot_iterator ihot=hots->begin();ihot != hots->end();++ihot){
	const TrkStrawHit* hit = dynamic_cast<const TrkStrawHit*>(ihot.get());
	if(hit != 0)hits.push_back(hit);
      }
      std::sort(hits.begin(),hits.end(),devicecomp());
    }
  }

  void
  KalFitMC::kalDiag(const KalRep* krep,bool fill) {
    GeomHandle<VirtualDetector> vdg;
    GeomHandle<DetectorSystem> det;

    if(krep != 0) {
      if(_trkdiag == 0)createTrkDiag();
// find the associated MC partcle
      std::vector<MCHitSum> kmcinfo;
      findMCTrk(krep,kmcinfo);
// mc track patermeter info for the particle which generated most of the hits
      if(kmcinfo.size()>0 ){
	SimParticle const& sp = *(kmcinfo[0]._spp);
	mcTrkInfo(sp);
	if( sp.genParticle().isNonnull())
	  _mcgenid = sp.genParticle()->generatorId().id();
	else
	  _mcgenid = 0;
	_mcpdgid = sp.pdgId();
	_mcproc = sp.creationCode();
      } else {
	_mcpdgid = 0;
	_mcgenid = -1;
	_mcproc = -1;
      }
// no information on iterations either!
      _nt0iter = _nweediter = -1;
      if(_diag > 1){
	std::vector<const TrkStrawHit*> hits;
	fillHitsVector(krep,hits);	
	hitsDiag(hits);
	if(_diag > 2)
	  arcsDiag(hits);
      }
      if(krep->fitCurrent()){
	_t0 = krep->t0().t0();
	_t0err = krep->t0().t0Err();
	_fitstatus = krep->fitStatus().success();
	_nhits = krep->hotList()->nHit();
	_niter = krep->iterations();
	_ndof = krep->nDof();
	_nactive = krep->nActive();
	_chisq = krep->chisq();
	_fitcon = krep->chisqConsistency().significanceLevel();
	_radlen = krep->radiationFraction();
	_nsites = krep->siteList().size();
	_firstflt = krep->firstHit()->globalLength();
	_lastflt = krep->lastHit()->globalLength();
	// get the fit at the first hit
	CLHEP::Hep3Vector entpos = det->toDetector(vdg->getGlobal(VirtualDetectorId::TT_FrontPA));
	double zent = entpos.z();
	double firsthitfltlen = krep->firstHit()->kalHit()->hitOnTrack()->fltLen() - 10;
	double lasthitfltlen = krep->lastHit()->kalHit()->hitOnTrack()->fltLen() - 10;
	double entlen = std::min(firsthitfltlen,lasthitfltlen);
	TrkHelixUtils::findZFltlen(krep->traj(),zent,entlen,0.1); 
	double loclen(0.0);
	const TrkSimpTraj* ltraj = krep->localTrajectory(entlen,loclen);
	_fitpar = helixpar(ltraj->parameters()->parameter());
	_fiterr = helixpar(ltraj->parameters()->covariance());
	CLHEP::Hep3Vector fitmom = krep->momentum(entlen);
	BbrVectorErr momerr = krep->momentumErr(entlen);
	_fitmom = fitmom.mag();
	Hep3Vector momdir = fitmom.unit();
	HepVector momvec(3);
	for(int icor=0;icor<3;icor++)
	  momvec[icor] = momdir[icor];
	_fitmomerr = sqrt(momerr.covMatrix().similarity(momvec));
	CLHEP::Hep3Vector seedmom = TrkMomCalculator::vecMom(*(krep->seed()),krep->kalContext().bField(),0.0);
	_seedmom = seedmom.mag();
      } else {
	_fitstatus = -krep->fitStatus().failure();
	_nhits = -1;
	_fitmom = -1.0;
	_seedmom = -1.0;
      }
    } else {
      _fitstatus = -1000;
      _nhits = -1;
      _fitmom = -1.0;
      _seedmom = -1.0;
      _mcpdgid = 0;
      _mcgenid = -1;
      _mcproc = -1;
// use the primary particle (if it exists)
      static cet::map_vector_key trkid(1);
      for ( SimParticleCollection::const_iterator isp = _mcdata._simparts->begin();
	isp != _mcdata._simparts->end(); ++isp ){
	if(isp->second.id() == trkid){
	  SimParticle const& sp = isp->second;
	  _mcgenid = sp.genParticle()->generatorId().id();
	  _mcpdgid = sp.pdgId();
	  _mcproc = sp.creationCode();
	  fillMCTrkInfo(sp,_mcinfo);
	  break;
	}
      }
    }
    if(fill)_trkdiag->Fill(); 
  }


  void
  KalFitMC::findMCTrk(const KalRep* krep, std::vector<MCHitSum>& mcinfo) {
    mcinfo.clear();
// get the straw hits from the track
    std::vector<const TrkStrawHit*> hits;
    fillHitsVector(krep,hits);	
// loop over the hits and find the associated steppoints
    for(size_t itsh=0;itsh<hits.size();++itsh){
      const TrkStrawHit* tsh = hits[itsh];
      PtrStepPointMCVector const& mcptr(_mcdata._mchitptr->at(tsh->index()));
      if(_mcdata._mcsteps != 0){
	fillMCHitSum(mcptr,mcinfo);
      }
    }
  }
  
  void KalFitMC::arcsDiag(std::vector<const TrkStrawHit*> const& hits) {
    _tainfo.clear();
    // find the arcs
    std::vector<TrkArc> arcs;
    findArcs(hits,arcs);
    _narcs = arcs.size();
    // loop over arcs
    for(size_t iarc=0;iarc < arcs.size(); ++iarc){
      TrkArcInfo tainfo;
      TrkArc const& arc = arcs[iarc];
      tainfo._narctsh = arc._ntsh;
      tainfo._narcactive = arc._nactive;
      tainfo._arctshlen = hits[arc._end]->fltLen() - hits[arc._begin]->fltLen();
      tainfo._arcactivelen = hits[arc._endactive]->fltLen() - hits[arc._beginactive]->fltLen();
      _tainfo.push_back(tainfo);
    }
  }

  void KalFitMC::hitsDiag(std::vector<const TrkStrawHit*> const& hits) {
    _tshinfo.clear();
    _ncactive = 0;
 // loop over hits
    for(size_t itsh=0;itsh<hits.size();++itsh){
      const TrkStrawHit* tsh = hits[itsh];
      if(tsh != 0){
        TrkStrawHitInfo tshinfo;
        tshinfo._active = tsh->isActive();
        tshinfo._usable = tsh->usability();
	tshinfo._device = tsh->straw().id().getDevice();
	tshinfo._sector = tsh->straw().id().getSector();
	tshinfo._layer = tsh->straw().id().getLayer();
	tshinfo._straw = tsh->straw().id().getStraw();
	static HepPoint origin(0.0,0.0,0.0);
	CLHEP::Hep3Vector hpos = tsh->hitTraj()->position(tsh->hitLen()) - origin;
	tshinfo._z = hpos.z();
	tshinfo._phi = hpos.phi();
	tshinfo._rho = hpos.perp();
	double resid,residerr;
	if(tsh->resid(resid,residerr,true)){
	  tshinfo._resid = resid;
	  tshinfo._residerr = residerr;
	} else {
	  tshinfo._resid = tshinfo._residerr = -1000.;
	}
	tshinfo._rdrift = tsh->driftRadius();
	tshinfo._rdrifterr = tsh->driftRadiusErr();
	tshinfo._trklen = tsh->fltLen();
	tshinfo._hlen = tsh->hitLen();
	tshinfo._ht = tsh->hitT0()._t0;
	tshinfo._tddist = tsh->timeDiffDist();
	tshinfo._tdderr = tsh->timeDiffDistErr();
	tshinfo._ambig = tsh->ambig();
	if(tsh->poca() != 0)
	  tshinfo._doca = tsh->poca()->doca();
	else
	  tshinfo._doca = -100.0;
	tshinfo._exerr = tsh->extErr();
	tshinfo._penerr = tsh->penaltyErr();
	tshinfo._t0err = tsh->t0Err();
	// MC information	
	PtrStepPointMCVector const& mcptr(_mcdata._mchitptr->at(tsh->index()));
	tshinfo._mcn = mcptr.size();
	if(_mcdata._mcsteps != 0){
	  std::vector<MCHitSum> mcsum;
	  fillMCHitSum(mcptr,mcsum);
	  tshinfo._mcnunique = mcsum.size();
	  tshinfo._mcppdg = mcsum[0]._pdgid;
	  tshinfo._mcpgen = mcsum[0]._gid;
	  tshinfo._mcpproc = mcsum[0]._pid;
// first hit is the one that set t0
	  tshinfo._mcht = mcptr[0]->time();
	  art::Ptr<SimParticle> sp = mcptr[0]->simParticle();
	  tshinfo._mcpdg = sp->pdgId();
	  tshinfo._mcproc = sp->creationCode();
	  tshinfo._mcgen = -1;
	  if(sp->genParticle().isNonnull())
	    tshinfo._mcgen = sp->genParticle()->generatorId().id();
// find the step midpoint
	  Hep3Vector start = mcptr[0]->position();
	  Hep3Vector dir = mcptr[0]->momentum().unit();
	  Hep3Vector end = start + dir*mcptr[0]->stepLength();
	  Hep3Vector mid = 0.5*(start+end);
	  Hep3Vector mcsep = mid-tsh->straw().getMidPoint();
	  Hep3Vector mcperp = (dir.cross(tsh->straw().getDirection())).unit();
	  double dperp = mcperp.dot(mcsep);
	  tshinfo._mcdist = fabs(dperp);
	  tshinfo._mcambig = dperp > 0 ? -1 : 1; // follow TrkPoca convention
	  tshinfo._mclen = mcsep.dot(tsh->straw().getDirection());
	}
	_tshinfo.push_back(tshinfo);
// count active conversion hits
	if(tshinfo._mcgen==2&&tsh->isActive())++_ncactive;
      }
    }
  }

  void KalFitMC::mcTrkInfo(SimParticle const&  sp) {
    GeomHandle<VirtualDetector> vdg;
    GeomHandle<DetectorSystem> det;
// find the mc info at the entrance to the detector
    fillMCTrkInfo(sp,_mcinfo);
    // find MC info at tracker
    cet::map_vector_key trkid = sp.id();
    std::vector<MCStepItr> entsteps,midsteps,xitsteps;
    findMCSteps(_mcdata._mcvdsteps,trkid,_entvids,entsteps);
    if(entsteps.size() > 0 && vdg->exist(entsteps[0]->volumeId()))
      fillMCTrkInfo(entsteps.front(),_mcentinfo);
    findMCSteps(_mcdata._mcvdsteps,trkid,_midvids,midsteps);
    if(midsteps.size() > 0 && vdg->exist(midsteps[0]->volumeId()))
      fillMCTrkInfo(midsteps.front(),_mcmidinfo);
    findMCSteps(_mcdata._mcvdsteps,trkid,_xitvids,xitsteps);
    if(xitsteps.size() > 0 && vdg->exist(xitsteps[0]->volumeId()))
      fillMCTrkInfo(xitsteps.front(),_mcxitinfo);
// find MC info about brems in the tracker
// should get these numbers from a service, FIXME!!!!
    static double _trkminz(-1501.0);
    static double _trkmaxz(1501.0);
    _bremsesum = 0.0;
    _bremsemax = 0.0;
    _bremsz = _trkminz;
    for ( SimParticleCollection::const_iterator isp = _mcdata._simparts->begin();
	isp != _mcdata._simparts->end(); ++isp ){
      SimParticle const& sp = isp->second;
// find photons with parent = the conversion electron created by brems
      if(sp.parent() != 0 && sp.parent()->id() == trkid && sp.pdgId() == PDGCode::gamma  && sp.creationCode() == ProcessCode::eBrem){
	CLHEP::Hep3Vector pos = det->toDetector(sp.startPosition());
	if(pos.z() > _trkminz && pos.z() < _trkmaxz){
	  double de = sp.startMomentum().e();
	  if(de > _bremsemax){
	    _bremsemax = de;
	    _bremsz = pos.z();
	  }
	  _bremsesum += de;
	}
      }
    }
  }

  void KalFitMC::fillMCHitSummary(){
    size_t nsh = _mcdata._mchitptr->size();
    _mchitsums.reserve(nsh);
    for(size_t ish=0;ish<nsh;++ish){
      PtrStepPointMCVector const& mcptr(_mcdata._mchitptr->at(ish));
      std::vector<MCHitSum> mcsum;
      fillMCHitSum(mcptr,mcsum); 
      _mchitsums.push_back(mcsum);
    }
  }

// Summarize an associated set of StepPointMCs from a StrawHit according to their parents.  This assignes
// daughter contributions to their parents
  void KalFitMC::fillMCHitSum(PtrStepPointMCVector const& mcptr,std::vector<MCHitSum>& summary ){
// first, create a map from daughters to mothers
    std::map<SPPtr,SPPtr> mdmap;
    findRelatives(mcptr,mdmap);
// Loop over the step points, and fill the summary vector
    for( size_t imc=0; imc< mcptr.size(); ++imc ) {
// find the primary parent, and create a pair for this step point's energy deposition
      art::Ptr<SimParticle> sp = mcptr[imc]->simParticle();
// find it's parent
      art::Ptr<SimParticle> spp = mdmap[sp];
// create the summary
      MCHitSum tsum(*mcptr[imc],spp);
// Add this energy to this particle, or create the entry if this is the first time this particle is seen
      std::vector<MCHitSum>::iterator ifnd = std::find(summary.begin(),summary.end(),tsum);
      if(ifnd == summary.end())
        summary.push_back(tsum);
      else
        ifnd->append(*mcptr[imc]);
    }
// sort this according to deposited energy
    std::sort(summary.begin(),summary.end(),MCHitSum::ecomp());
  }

// map daughters onto parents within the context of an associated set of StepPointMCs (like from a StrawHit).
  void KalFitMC::findRelatives(PtrStepPointMCVector const& mcptr,std::map<SPPtr,SPPtr>& mdmap){
    // loop over steps
    for( size_t imc=0; imc< mcptr.size(); ++imc ) {
      art::Ptr<SimParticle> spp = mcptr[imc]->simParticle();		  
      if(mdmap.find(spp) == mdmap.end()){
    // start with self-reference
        mdmap[spp] = spp;
      }
    }
    // loop over the steps again, trying to map particles to their parents
    for( size_t imc=0; imc< mcptr.size(); ++imc ) {
      art::Ptr<SimParticle> spp = mcptr[imc]->simParticle();		  
      if(mdmap[spp] == spp && !spp.isNull()){
    // move up through the genealogy till we find the highest-rank parent directly contributing
    // to this strawhit
        art::Ptr<SimParticle> sppp = spp->parent();
        while(!sppp.isNull()){
          std::map<SPPtr,SPPtr>::iterator ifnd = mdmap.find(sppp);
          if(ifnd != mdmap.end())
    // repoint this particle to its highest-level contributing parent
            mdmap[spp] = sppp;
    // move on to the parent's parent, as there can be gaps in the direct contribution
          sppp = sppp->parent();
        }
      }
    }
  }
 
  TTree*
  KalFitMC::createTrkDiag(){
    art::ServiceHandle<art::TFileService> tfs;
    _trkdiag=tfs->make<TTree>("trkdiag","trk diagnostics");
    _trkdiag->Branch("fitstatus",&_fitstatus,"fitstatus/I");
    _trkdiag->Branch("t0",&_t0,"t0/F");
    _trkdiag->Branch("t0err",&_t0err,"t0err/F");
    _trkdiag->Branch("nhits",&_nhits,"nhits/I");
    _trkdiag->Branch("ndof",&_ndof,"ndof/I");
    _trkdiag->Branch("niter",&_niter,"niter/I");
    _trkdiag->Branch("nt0iter",&_nt0iter,"nt0iter/I");
    _trkdiag->Branch("nweediter",&_nweediter,"nweediter/I");
    _trkdiag->Branch("nactive",&_nactive,"nactive/I");
    _trkdiag->Branch("ncactive",&_ncactive,"ncactive/I");
    _trkdiag->Branch("narcs",&_narcs,"narcs/I");
    _trkdiag->Branch("nchits",&_nchits,"nchits/I");
    _trkdiag->Branch("chisq",&_chisq,"chisq/F");
    _trkdiag->Branch("fitcon",&_fitcon,"fitcon/F");
    _trkdiag->Branch("radlen",&_radlen,"radlen/F");
    _trkdiag->Branch("firstflt",&_firstflt,"firstflt/F");
    _trkdiag->Branch("lastflt",&_lastflt,"lastflt/F");
    _trkdiag->Branch("nsites",&_nsites,"nsites/I");
    _trkdiag->Branch("fitmom",&_fitmom,"fitmom/F");
    _trkdiag->Branch("fitmomerr",&_fitmomerr,"fitmomerr/F");
    _trkdiag->Branch("seedmom",&_seedmom,"seedmom/F");
    _trkdiag->Branch("fitpar",&_fitpar,"d0/F:p0/F:om/F:z0/F:td/F");
    _trkdiag->Branch("fiterr",&_fiterr,"d0err/F:p0err/F:omerr/F:z0err/F:tderr/F");
// basic MC info
    _trkdiag->Branch("mcpdgid",&_mcpdgid,"mcpdgid/I");
    _trkdiag->Branch("mcgenid",&_mcgenid,"mcgenid/I");
    _trkdiag->Branch("mcproc",&_mcproc,"mcproc/I");
// mc info at production and several spots in the tracker
    _trkdiag->Branch("mcinfo",&_mcinfo,"mcpdgid/I:mct0/F:mcmom/F:mcx/F:mcy/F:mcz/F:mcd0/F:mcp0/F:mcom/F:mcz0/F:mctd/F");
    _trkdiag->Branch("mcentinfo",&_mcentinfo,"mcentpdgid/I:mcentt0/F:mcentmom/F:mcentx/F:mcenty/F:mcentz/F:mcentd0/F:mcentp0/F:mcentom/F:mcentz0/F:mcenttd/F");
    _trkdiag->Branch("mcmidinfo",&_mcmidinfo,"mcmidpdgid/I:mcmidt0/F:mcmidmom/F:mcmidx/F:mcmidy/F:mcmidz/F:mcmidd0/F:mcmidp0/F:mcmidom/F:mcmidz0/F:mcmidtd/F");
    _trkdiag->Branch("mcxitinfo",&_mcxitinfo,"mcxitpdgid/I:mcxitt0/F:mcxitmom/F:mcxitx/F:mcxity/F:mcxitz/F:mcxitd0/F:mcxitp0/F:mcxitom/F:mcxitz0/F:mcxittd/F");
// info about energy loss in the tracker
    _trkdiag->Branch("bremsesum",&_bremsesum,"bremsesum/F");
    _trkdiag->Branch("bremsemax",&_bremsemax,"bremsemax/F");
    _trkdiag->Branch("bremsz",&_bremsz,"bremsz/F");
// track hit and arc info    
    if(_diag > 1)
      _trkdiag->Branch("tshinfo",&_tshinfo);
    if(_diag > 2)
      _trkdiag->Branch("tainfo",&_tainfo);
    return _trkdiag;
  }

  TTree*
  KalFitMC::createHitDiag(){
    art::ServiceHandle<art::TFileService> tfs;
    _hitdiag=tfs->make<TTree>("hitdiag","hit diagnostics");      
    _hitdiag->Branch("shpos",&_shpos,"x/F:y/F:z/F");
    _hitdiag->Branch("rdrift",&_rdrift,"rdrift/F");
    _hitdiag->Branch("rdrifterr",&_rdrifterr,"rdrifterr/F");
    _hitdiag->Branch("amb",&_amb,"amb/I");
    _hitdiag->Branch("hitt0",&_hitt0,"hitt0/F");
    _hitdiag->Branch("hitt0err",&_hitt0err,"hitt0err/F");
    _hitdiag->Branch("dmid",&_dmid,"dmid/F");
    _hitdiag->Branch("dmiderr",&_dmiderr,"dmiderr/F");
    _hitdiag->Branch("resid",&_resid,"resid/F");
    _hitdiag->Branch("residerr",&_residerr,"residerr/F");
    _hitdiag->Branch("hflt",&_hflt,"hflt/F");
    _hitdiag->Branch("trkflt",&_trkflt,"trkflt/F");
    _hitdiag->Branch("active",&_active,"active/B");
    _hitdiag->Branch("use",&_use,"use/I");
    _hitdiag->Branch("edep",&_edep,"edep/F");
    _hitdiag->Branch("pdist",&_pdist,"pdist/F");
    _hitdiag->Branch("pperp",&_pperp,"pperp/F");
    _hitdiag->Branch("pmom",&_pmom,"pmom/F");

    _hitdiag->Branch("nmcsteps",&_nmcsteps,"nmcsteps/i");
    _hitdiag->Branch("mchpos",&_mchpos,"x/F:y/F:z/F");
    _hitdiag->Branch("mcrdrift",&_mcrdrift,"mcrdrift/F");
    _hitdiag->Branch("mchitt0",&_mchitt0,"mchitt0/F");
    _hitdiag->Branch("mcdmid",&_mcdmid,"mcdmid/F");

//      _hitdiag->Branch("shdir","CLHEP::HepVector",&_shdir);
    return _hitdiag;
  }

// find the MC truth objects in the event and set the local cache
  bool
  KalFitMC::findMCData(const art::Event& evt) {
    _mcdata.clear();
    _mchitsums.clear();
    art::Handle<StrawHitMCTruthCollection> truthHandle;
    if(evt.getByLabel(_mcstrawhitslabel,truthHandle))
      _mcdata._mcstrawhits = truthHandle.product();
  // Get the persistent data about pointers to StepPointMCs
    art::Handle<PtrStepPointMCVectorCollection> mchitptrHandle;
    if(evt.getByLabel(_mcptrlabel,"StrawHitMCPtr",mchitptrHandle))
      _mcdata._mchitptr = mchitptrHandle.product();
  // Get the persistent data about the StepPointMCs, from the tracker and the virtual detectors
    art::Handle<StepPointMCCollection> mctrackerstepsHandle;
    if(evt.getByLabel(_mcstepslabel,"tracker",mctrackerstepsHandle))
      _mcdata._mcsteps = mctrackerstepsHandle.product();
    art::Handle<StepPointMCCollection> mcVDstepsHandle;
    if(evt.getByLabel(_mcstepslabel,"virtualdetector",mcVDstepsHandle))
      _mcdata._mcvdsteps = mcVDstepsHandle.product();
    art::Handle<SimParticleCollection> simParticlesHandle;
    if(evt.getByLabel(_simpartslabel,simParticlesHandle))
      _mcdata._simparts = simParticlesHandle.product();
// fill hit summary
    fillMCHitSummary();
    if( _mcdata.good()) {
// count # of conversion straw hits
      _strawhits = 0;
      _nchits = 0;
      art::Handle<mu2e::StrawHitCollection> strawhitsH;
      if(evt.getByLabel(_strawhitslabel,strawhitsH))
	_strawhits = strawhitsH.product();
      if(_strawhits != 0){
	unsigned nstrs = _strawhits->size();
	for(unsigned istr=0; istr<nstrs;++istr){
	  const std::vector<MCHitSum>& mcsum = mcHitSummary(istr);
	  if(mcsum.size()>0){
	    bool conversion = (mcsum[0]._pdgid == 11 && mcsum[0]._gid == 2);
	    if(conversion){
	      ++_nchits;
	    }
	  }
	}
      }
      return true;
    }
    return false;
  }
  std::vector<int>const& KalFitMC::VDids(TRACKERPOS tpos) const {
    switch(tpos) {
      case trackerEnt: default:
	return _entvids;
      case trackerMid:
	return _midvids;
      case trackerExit:
	return _xitvids;
    }
  }

  double KalFitMC::MCT0(TRACKERPOS tpos) const {
    switch(tpos) {
      case trackerEnt: default:
	return _mcentinfo._time;
      case trackerMid:
	return _mcmidinfo._time;
      case trackerExit:
	return _mcxitinfo._time;
    }
  }
  double KalFitMC::MCMom(TRACKERPOS tpos) const {
    switch(tpos) {
      case trackerEnt: default:
	return _mcentinfo._mom;
      case trackerMid:
	return _mcmidinfo._mom;
      case trackerExit:
	return _mcxitinfo._mom;
    }
  }
  const helixpar& KalFitMC::MCHelix(TRACKERPOS tpos) const {
    switch(tpos) {
      case trackerEnt: default:
	return _mcentinfo._hpar;
      case trackerMid:
	return _mcmidinfo._hpar;
      case trackerExit:
	return _mcxitinfo._hpar;
    }
  }

  void
  KalFitMC::findArcs(std::vector<const TrkStrawHit*> const& straws, std::vector<TrkArc>& arcs) const {
    arcs.clear();
// define an initial arc
    size_t istraw(0);
    while(istraw < straws.size()){
      int igap(0);
      TrkArc arc(istraw);
      bool firstactive(false);
      do {
	arc._end = istraw;
	++arc._ntsh;
	if(straws[istraw]->isActive()){
	  arc._endactive = istraw;
	  ++arc._nactive;
	  if(!firstactive){
	    arc._beginactive = istraw;
	    firstactive = true;
	  }
	}
// advance to the next straw and compute the gap
	++istraw;
      	if(istraw< straws.size())igap = straws[istraw]->straw().id().getDevice()-straws[arc._end]->straw().id().getDevice();
// loop while the gap is small
      } while(istraw < straws.size() && igap <= _maxarcgap);
// end of an arc: record it
      arcs.push_back(arc);
// update the arc to start with this new straw
      arc = TrkArc(istraw);
    }
  }

  int
  KalFitMC::findArc(size_t itsh,std::vector<TrkArc>& arcs ) {
    for(size_t iarc=0;iarc<arcs.size();++iarc){
      if(itsh >= arcs[iarc]._begin && itsh <= arcs[iarc]._end)
	return iarc;
    }
    return -1;
  }

  void
  KalFitMC::fillMCTrkInfo(MCStepItr const& imcs, MCTrkInfo& einfo) const {
    GlobalConstantsHandle<ParticleDataTable> pdt;
    GeomHandle<DetectorSystem> det;
    GeomHandle<BFieldConfig> bfconf;

    einfo._time = imcs->time();
    einfo._pdgid = imcs->simParticle()->pdgId(); 
    double charge = pdt->particle(imcs->simParticle()->pdgId()).ref().charge();
    CLHEP::Hep3Vector mom = imcs->momentum();
    einfo._mom = mom.mag();
    // need to transform into the tracker coordinate system
    CLHEP::Hep3Vector pos = det->toDetector(imcs->position());
    HepPoint ppos =(pos.x(),pos.y(),pos.z());
    einfo._pos = pos;
    double hflt(0.0);
    HepVector parvec(5,0);
    TrkHelixUtils::helixFromMom( parvec, hflt,ppos,
	mom,charge,bfconf->getDSUniformValue().z());
    einfo._hpar = helixpar(parvec);
  }

  void
  KalFitMC::fillMCTrkInfo(SimParticle const& sp, MCTrkInfo& einfo) const {
    GlobalConstantsHandle<ParticleDataTable> pdt;
    GeomHandle<DetectorSystem> det;
    GeomHandle<BFieldConfig> bfconf;

    einfo._time = sp.startGlobalTime();
    einfo._pdgid = sp.pdgId();
    double charge = pdt->particle(sp.pdgId()).ref().charge();
    CLHEP::Hep3Vector mom = sp.startMomentum();
    einfo._mom = mom.mag();
    // need to transform into the tracker coordinate system
    CLHEP::Hep3Vector pos = det->toDetector(sp.startPosition());
    HepPoint ppos =(pos.x(),pos.y(),pos.z());
    einfo._pos = pos;
    double hflt(0.0);
    HepVector parvec(5,0);
    TrkHelixUtils::helixFromMom( parvec, hflt,ppos,
	mom,charge,bfconf->getDSUniformValue().z());
    einfo._hpar = helixpar(parvec);
  }


}
