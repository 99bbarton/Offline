//
// Class to perform BaBar Kalman fit
// Original author: Dave Brown LBNL 2012
//
// $Id: KalFit.cc,v 1.43 2014/08/22 16:10:41 tassiell Exp $
// $Author: tassiell $
// $Date: 2014/08/22 16:10:41 $
//
#include "TrkReco/inc/KalFit.hh"
#include "TrkReco/inc/PanelAmbigResolver.hh"
#include "TrkReco/inc/HitAmbigResolver.hh"
#include "TrkReco/inc/FixedAmbigResolver.hh"
#include "TrkReco/inc/DoubletAmbigResolver.hh"
#include "TrkReco/inc/TrkUtilities.hh"
#include "Mu2eBTrk/inc/BaBarMu2eField.hh"
#include "Mu2eBTrk/inc/Mu2eDetectorModel.hh"
//geometry
#include "BTrkHelper/inc/BTrkHelper.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/getTrackerOrThrow.hh"
#include "BFieldGeom/inc/BFieldConfig.hh"
#include "CalorimeterGeom/inc/Calorimeter.hh"
#include "StoppingTargetGeom/inc/StoppingTarget.hh"
#include "GeometryService/inc/DetectorSystem.hh"
// conditions
#include "ConditionsService/inc/ConditionsHandle.hh"
// data
#include "RecoDataProducts/inc/ComboHit.hh"
// tracker
#include "TTrackerGeom/inc/TTracker.hh"
#include "TrackerGeom/inc/Tracker.hh"
#include "TrackerGeom/inc/Straw.hh"
// BaBar
#include "BTrk/KalmanTrack/KalHit.hh"
#include "BTrk/KalmanTrack/KalBend.hh"
#include "BTrk/KalmanTrack/KalMaterial.hh"
#include "BTrk/TrkBase/HelixTraj.hh"
#include "BTrk/TrkBase/TrkHelixUtils.hh"
#include "BTrk/TrkBase/TrkMomCalculator.hh"
#include "BTrk/TrkBase/TrkPoca.hh"
#include "BTrk/BaBar/ErrLog.hh"
#include "BTrk/BField/BFieldFixed.hh"
#include "BTrk/DetectorModel/DetIntersection.hh"
#include "BTrk/DetectorModel/DetMaterial.hh"
#include "BTrk/ProbTools/ChisqConsistency.hh"
// boost
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/weighted_variance.hpp>
#include <boost/accumulators/statistics/weighted_median.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>
//CLHEP
#include "CLHEP/Vector/ThreeVector.h"
// C++
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <set>
//art
#include "art/Utilities/ToolMacros.h"
#include "art/Utilities/make_tool.h"

using namespace std;
using CLHEP::Hep3Vector;
using CLHEP::HepVector;
using CLHEP::HepSymMatrix;

namespace mu2e
{
// comparison functor for ordering hits.  This should operate on TrkHit, FIXME!
  struct fcomp : public binary_function<TrkHit*, TrkHit*, bool> {
    bool operator()(TrkHit* x, TrkHit* y) {
      return x->fltLen() < y->fltLen();
    }
  };

// struct for finding materials
  struct StrawFlight {
    StrawId _id;  // straw being tested
    double _flt; // flight where trajectory comes near this straw
// construct from pair
    StrawFlight(StrawId strawid, double flt) : _id(strawid), _flt(flt) {}
  };

// comparison operators understand that the same straw could be hit twice, so the flight lengths need
// to be similar befoew we consider these 'the same'
  struct StrawFlightComp : public binary_function<StrawFlight, StrawFlight, bool> {
    double _maxdiff; // maximum flight difference; below this, consider 2 intersections 'the same'
    StrawFlightComp(double maxdiff) : _maxdiff(maxdiff) {}
    bool operator () (StrawFlight const& a, StrawFlight const& b) { return a._id < b._id ||
    ( a._id == b._id && a._flt < b._flt && fabs(a._flt-b._flt)>=_maxdiff);}
  };

// construct from a parameter set
  KalFit::KalFit(fhicl::ParameterSet const& pset) :
// KalFit parameters
    _debug(pset.get<int>("debugLevel",0)),
    _maxhitchi(pset.get<double>("maxhitchi",3.5)),
    _maxpull(pset.get<double>("maxPull",5)),
    _maxweed(pset.get<unsigned>("maxweed",10)),
    // t0 parameters
    _initt0(pset.get<bool>("initT0",true)),
    _useTrkCaloHit(pset.get<bool>("useTrkCaloHit")),
    _updatet0(pset.get<vector<bool>>("updateT0")),
    _t0tol(pset.get< vector<double> >("t0Tolerance")),
    _t0errfac(pset.get<double>("t0ErrorFactor",1.2)),
    _t0nsig(pset.get<double>("t0window",2.5)),
    _dtoffset(pset.get<double>("dtOffset")),
    _strHitW(pset.get<double>("strawHitT0Weight")),
    _calHitW(pset.get<double>("caloHitT0Weight")),
    //
    _minnstraws(pset.get<unsigned>("minnstraws",15)),
    _maxmatfltdiff(pset.get<double>("MaximumMaterialFlightDifference",1000.0)), // mm separation in flightlength
    _weedhits(pset.get<vector<bool> >("weedhits")),
    _herr(pset.get< vector<double> >("hiterr")),
    _ambigstrategy(pset.get< vector<int> >("ambiguityStrategy")),
    _addmaterial(pset.get<vector<bool> >("AddMaterial")),
    _resolveAfterWeeding(pset.get<bool>("ResolveAfterWeeding",false)),
    _exup((extent)pset.get<int>("UpstreamExtent",noextension)),
    _exdown((extent)pset.get<int>("DownstreamExtent",noextension)),
    _mcTruth(pset.get<int>("mcTruth",0)),
    _ttcalc            (pset.get<fhicl::ParameterSet>("T0Calculator",fhicl::ParameterSet())),
    _bfield(0)
  {
// set KalContext parameters
    _disttol = pset.get<double>("IterationTolerance",0.1);
    _intertol = pset.get<double>("IntersectionTolerance",100.0);
    _maxiter = pset.get<long>("MaxIterations",10);
    _maxinter = pset.get<long>("MaxIntersections",0);
    _matcorr = pset.get<bool>("materialCorrection",true);
    _fieldcorr = pset.get<bool>("fieldCorrection",false);
    _smearfactor = pset.get<double>("SeedSmear",1.0e6);
    _sitethresh = pset.get<double>("SiteMomThreshold",0.2);
    _momthresh = pset.get<double>("MomThreshold",10.0);
    _mingap = pset.get<double>("mingap",1.0);
    _minfltlen = pset.get<double>("MinFltLen",0.1);
    _minmom = pset.get<double>("MinMom",10.0);
    _fltepsilon = pset.get<double>("FltEpsilon",0.001);
    _divergeflt = pset.get<double>("DivergeFlt",1.0e3);
    _mindot = pset.get<double>("MinDot",0.0);
    _maxmomdiff = pset.get<double>("MaxMomDiff",0.5);
    _momfac = pset.get<double>("MomFactor",0.0);
    _maxpardif[0] = _maxpardif[1] = pset.get<double>("MaxParameterDifference",1.0);

    _mindof = pset.get<double>("MinNDOF",10);
    // this config belongs in the BField integrator, FIXME!!!
    _bintconfig._maxRange = pset.get<double>("BFieldIntMaxRange",1.0e5); // 100 m
    _bintconfig._intTolerance = pset.get<double>("BFieldIntTol",0.01); // 10 KeV
    _bintconfig._intPathMin = pset.get<double>("BFieldIntMin",20.0); // 20 mm
    _bintconfig._divTolerance = pset.get<double>("BFieldIntDivTol",0.05); // 50 KeV
    _bintconfig._divPathMin = pset.get<double>("BFieldIntDivMin",50.0); // 50 mm
    _bintconfig._divStepCeiling = pset.get<double>("BFieldIntDivMax",500.0); // 500 mm
    // field integral errors.  This is commented out as it hasn't been shown to improve the fit
    //    double perr = pset.get<double>("BendCorrErrFrac",0.0); // fractional accuracy of trajectory
    //    double berr = pset.get<double>("BFieldMapErr",0.0); // mapping and interpolation error
//    KalBend::setErrors(perr,berr);
    // make sure we have at least one entry for additional errors
    if(_herr.size() <= 0) throw cet::exception("RECO")<<"mu2e::KalFit: no hit errors specified" << endl;
    if(_herr.size() != _ambigstrategy.size()) throw cet::exception("RECO")<<"mu2e::KalFit: inconsistent ambiguity resolution hiterr" << endl;
    if(_herr.size() != _t0tol.size()) throw cet::exception("RECO")<<"mu2e::KalFit: inconsistent ambiguity resolution t0" << endl;
    if(_herr.size() != _weedhits.size()) throw cet::exception("RECO")<<"mu2e::KalFit: inconsistent ambiguity resolution WeedHits" << endl;
    if(_herr.size() != _addmaterial.size()) throw cet::exception("RECO")<<"mu2e::KalFit: inconsistent ambiguity resolution AddMaterial" << endl;
    // Search for explicit resolver parameter sets.  These may not be used
    fhicl::ParameterSet const& fixedPset = pset.get<fhicl::ParameterSet>("FixedAmbigResolver",fhicl::ParameterSet());
    fhicl::ParameterSet const& hitPset = pset.get<fhicl::ParameterSet>("HitAmbigResolver",fhicl::ParameterSet());
    fhicl::ParameterSet const& panelPset = pset.get<fhicl::ParameterSet>("PanelAmbigResolver",fhicl::ParameterSet());
    fhicl::ParameterSet const& pocaPset = pset.get<fhicl::ParameterSet>("POCAAmbigResolver",fhicl::ParameterSet());
    fhicl::ParameterSet const& doubletPset = pset.get<fhicl::ParameterSet>("DoubletAmbigResolver",fhicl::ParameterSet());
// construct the explicit ambiguity resolvers, 1 instance per iteration
    size_t niter = _ambigstrategy.size();
    for(size_t iter=0; iter<niter; ++iter) {
      int Final(0);//      int Final = iter==niter-1 ? 1 : 0;
      AmbigResolver* ar(0);
      switch (_ambigstrategy[iter]) {
      case fixedambig:
        ar = new FixedAmbigResolver(fixedPset,_herr[iter]);
        break;
      case hitambig:
        ar = new HitAmbigResolver(hitPset,_herr[iter]);
        break;
      case panelambig:
        ar = new PanelAmbig::PanelAmbigResolver(panelPset,_herr[iter],iter);
        break;
      case doubletambig: // 4
        ar = new DoubletAmbigResolver(doubletPset,_herr[iter],iter,Final);
        break;
      default:
        break;
      }
      if(ar != 0)
        _ambigresolver.push_back(ar);
      else
        throw cet::exception("RECO")<<"mu2e::KalFit: unknown ambiguity resolver " << _ambigstrategy[iter] << " for iteration " << iter << endl;
    }

    // //DEBUG GIANIPEZ
    // if (_ambigstrategy[niter-1] == doubletambig) {
    //   int Final(0);
    //   AmbigResolver* ar = new DoubletAmbigResolver(doubletPset,_herr[niter-1],niter,Final);
    //   _ambigresolver.push_back(ar);
    // }
    
//-----------------------------------------------------------------------------
// print routine
//-----------------------------------------------------------------------------
//    _mcTruth = pset.get <int >("mcTruth"); 

    if (_mcTruth != 0) {
      fhicl::ParameterSet ps = pset.get<fhicl::ParameterSet>("mcUtils");
      _mcUtils = art::make_tool  <McUtilsToolBase>(ps);
    }
    else               _mcUtils = std::make_unique<McUtilsToolBase>();
  }

  KalFit::~KalFit(){
    for(size_t iambig=0;iambig<_ambigresolver.size();++iambig){
      delete _ambigresolver[iambig];
    }
    delete _bfield;
  }

//-----------------------------------------------------------------------------
// create the track (KalRep) from a track seed
//-----------------------------------------------------------------------------
  void KalFit::makeTrack(KalFitData& kalData){
// test if fitable
    if(fitable(*kalData.kalSeed)){
      // find the segment at the 0 flight
      double flt0 = kalData.kalSeed->flt0();
      auto kseg = kalData.kalSeed->segments().end();
      for(auto iseg= kalData.kalSeed->segments().begin(); iseg != kalData.kalSeed->segments().end(); ++iseg){
	if(iseg->fmin() <= flt0 && iseg->fmax() > flt0){
	  kseg = iseg;
	  break;
	}
      }
      if(kseg == kalData.kalSeed->segments().end()){
	std::cout << "Helix segment range doesn't cover flt0" << std::endl;
	kseg = kalData.kalSeed->segments().begin();
      }
      // create a trajectory from the seed. This shoudl be a general utility function that
      // can work with multi-segment seeds FIXME!
      // create CLHEP objects from seed native members.  This will
      // go away when we switch to SMatrix FIXME!!!
      HepVector pvec(5,0);
      HepSymMatrix pcov(5,0);
      kseg->helix().hepVector(pvec);
      kseg->covar().symMatrix(pcov);
      // Create the traj from these
      HelixTraj htraj(pvec,pcov);
      kalData.helixTraj = &htraj;
      // create the hits
      TrkStrawHitVector tshv;
      makeTrkStrawHits(kalData, tshv);
      
   // Find the wall and gas material description objects for these hits
      std::vector<DetIntersection> detinter;
      if(_matcorr)makeMaterials(tshv,*kalData.helixTraj,detinter);
   // Create the BaBar hit list, and fill it with these hits.  The BaBar list takes ownership
      // We should use the TrkHit vector everywhere, FIXME!
      std::vector<TrkHit*> thv(0);
      for(auto ihit = tshv.begin(); ihit != tshv.end(); ++ihit){
        thv.push_back(*ihit);
        if (_debug>2) { (*ihit)->print(std::cout); }
      }
      if (_useTrkCaloHit){    //use the TrkCaloHit to initialize the t0?
	//create the TrkCaloHit
	TrkCaloHit* tch(0);
	makeTrkCaloHit(kalData, tch);
	if (tch != 0) thv.push_back(tch);
      }
 
      TrkT0 t0(kalData.kalSeed->t0());
      // create Kalman rep
      kalData.krep = new KalRep(htraj, thv, detinter, *this, kalData.kalSeed->particle(), t0, flt0);
      assert(kalData.krep != 0);
      if(_initt0){
	initT0(kalData);
      }
      
      if (_debug > 0) printHits(kalData,"makeTrack_001");

// initialize history list
      kalData.krep->addHistory(TrkErrCode(),"KalFit creation");
// now fit
      TrkErrCode fitstat = fitTrack(kalData);
      kalData.krep->addHistory(fitstat,"KalFit fit");
// extend the fit
      if(fitstat.success()){
	fitstat = extendFit(kalData.krep);
	kalData.krep->addHistory(fitstat,"KalFit extension");
      }
    }
  }

  void KalFit::addHits(KalFitData&kalData, double maxchi) {
  //2017-05-02: Gianipez. In this function inten
  // fetcth the DetectorModel
   Mu2eDetectorModel const& detmodel{ art::ServiceHandle<BTrkHelper>()->detectorModel() };
// there must be a valid Kalman fit to add hits to
   KalRep* krep = kalData.krep;
   
   if(kalData.krep != 0 && kalData.missingHits.size() > 0 && krep->fitStatus().success()){
      TrkHitVector::iterator ihigh;
      TrkHitVector::reverse_iterator ilow;
// use the reference trajectory, as that's what all the existing hits do
      const TrkDifPieceTraj* reftraj = krep->referenceTraj();
      for(unsigned iind=0;iind<kalData.missingHits.size(); ++iind){
        size_t istraw = kalData.missingHits[iind].index;
        const ComboHit& strawhit(kalData.chcol->at(istraw));
        const Straw& straw = _tracker->getStraw(strawhit.strawId());
// estimate  initial flightlength
        double hflt(0.0);
        TrkHelixUtils::findZFltlen(*reftraj,straw.getMidPoint().z(),hflt);
// find the bounding sites near this hit, and extrapolate to get the hit t0
        findBoundingHits(krep,hflt,ilow,ihigh);
        const TrkHit* nearhit;
        if(ihigh != krep->hitVector().end())
          nearhit = *ihigh;
        else
          nearhit = *ilow;
        TrkT0 hitt0 = nearhit->hitT0();
        double mom = krep->momentum(nearhit->fltLen()).mag();
        double beta = krep->particleType().beta(mom);
        double tflt = (hflt-nearhit->fltLen())/(beta*CLHEP::c_light);
// update the time in the TrkT0 object
        hitt0._t0 += tflt;  // FIXME!!! assumes beta=1
// create the hit object.  Assume we're at the last iteration over added error
        TrkStrawHit* trkhit = new TrkStrawHit(strawhit,straw,istraw,hitt0,hflt,
					      _maxpull,_strHitW );
        assert(trkhit != 0);
	trkhit->setTemperature(_herr.back()); // give this hit the final annealing temperature
        trkhit->setFlag(TrkHit::addedHit);
// guess the ambiguity form the sign of the doca
	int iambig;
	if (kalData.missingHits[iind].doca > 0) iambig =  1;
	else                                    iambig = -1;
// can set ambiguity only for deactivated hit
	trkhit->setActivity(false);
	trkhit->setAmbig(iambig);
// must be initialy active for KalRep to process correctly
        trkhit->setActivity(true);
// set the hit ambiguity.  This is a preliminary value before using the official ambig resolver
	TrkPoca poca(krep->traj(),hflt,*trkhit->hitTraj(),0.0);
	int newamb = poca.doca() > 0 ? 1 : -1;
	trkhit->setAmbig(newamb);
// add the hit to the track
        krep->addHit(trkhit);
// check the raw residual: This call works because the HOT isn't yet processed as part of the fit.
        double chi = fabs(trkhit->residual()/trkhit->hitRms());
//if it's outside limits, deactivate the HOT
        if(chi > maxchi || (!trkhit->isPhysical(maxchi)))
          trkhit->setActivity(false);
   // find the DetElem associated this straw
        const DetStrawElem* strawelem = detmodel.strawElem(trkhit->straw());
// see if this KalRep already has a KalMaterial with this element: if not, add it
        bool hasmat(false);
        std::vector<const KalMaterial*> kmats;
        krep->findMaterialSites(strawelem,kmats);
// if this is a reflecting track the same material can appear multiple times: check the flight lengths
        if(kmats.size() > 0){
          for(auto kmat: kmats) {
            if( fabs( kmat->globalLength() - trkhit->fltLen()) < _maxmatfltdiff){
              hasmat = true;
              break;
            }
          }
        }
        if(!hasmat){
          // create intersection object for this element; it includes all materials
          DetIntersection strawinter(strawelem, krep->referenceTraj(),trkhit->fltLen());
          strawinter.thit = trkhit;
          // compute initial intersection: this gets updated each fit iteration
          strawelem->reIntersect(krep->referenceTraj(),strawinter);
          krep->addInter(strawinter);
        }
      }
// refit the last iteration of the track
      TrkErrCode fitstat = fitIteration(kalData,_herr.size()-1);
      krep->addHistory(fitstat,"AddHits");
    }
  }
//
  TrkErrCode KalFit::fitTrack(KalFitData&kalData) {
    // loop over external hit errors, ambiguity assignment, t0 toleratnce
    TrkErrCode fitstat;
    for(size_t iherr=0;iherr < _herr.size(); ++iherr) {
      fitstat = fitIteration(kalData,iherr);
      if(_debug > 0) cout << "Iteration " << iherr 
      << " NDOF = " << kalData.krep->nDof() 
      << " Fit Status = " <<  fitstat << endl;
      if(!fitstat.success())break;
    }
    return fitstat;
  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
  TrkErrCode KalFit::fitIteration(KalFitData&kalData, int iter) {

    if (iter == -1) iter =  _herr.size()-1;
    _annealingStep = iter;//used in the printHits routine

    // update the external hit errors.  This isn't strictly necessary on the 1st iteration.
    TrkHitVector* thv   = &(kalData.krep->hitVector());
    for (auto itsh=thv->begin();itsh!=thv->end(); ++itsh){
      (*itsh)->setTemperature(_herr[iter]);
    }
 
   // update t0, and propagate it to the hits
    double oldt0 = kalData.krep->t0()._t0;
    unsigned niter(0);
    bool changed(true);
    TrkErrCode retval = TrkErrCode::succeed;

    KalRep* krep =  kalData.krep;

    while(retval.success() && changed && ++niter < maxIterations()){
//-----------------------------------------------------------------------------
// convention: resolve drift signs before the fit with respect to the trajectory
// determined at the previous iteration
//-----------------------------------------------------------------------------
      changed = _ambigresolver[iter]->resolveTrk(krep);
      // force a refit
      krep->resetFit();
      retval = krep->fit();
      if(! retval.success())break;
      // updates
      if(_updatet0[iter]){
	updateT0(kalData);
        changed |= fabs(krep->t0()._t0-oldt0) > _t0tol[iter];
      }
      // drop outliers
      if(_weedhits[iter]){
        kalData.nweediter = 0;
        changed |= weedHits(kalData,iter);
	changed |= unweedBestHit(kalData,_maxhitchi);
      }
      // find missing materials
      unsigned nmat(0);
      if(_addmaterial[iter]){
	nmat = addMaterial(krep);
        changed |= nmat>0;
      }
      if(_debug > 1) std::cout << "Inner iteration " << niter << " changed = "
	<< changed << " t0 old " << oldt0 << " new " << krep->t0()._t0 
	<< " nmat = " << nmat << endl;
      oldt0 = krep->t0()._t0;
    }
    if(_debug > 1)
      std::cout << "Outer iteration " << iter << " stopped after "
      << niter << " iterations" << std::endl;
// make sure the fit is current
    if(!krep->fitCurrent())
      retval = krep->fit();
    return retval;
  }

  bool
  KalFit::fitable(TrkDef const& tdef){
    return tdef.strawHitIndices().size() >= _minnstraws;
  }

  bool
  KalFit::fitable(KalSeed const& kseed){
    return kseed.segments().size() > 0 && kseed.hits().size() >= _minnstraws;
  }

  void
  KalFit::makeTrkStrawHits(KalFitData& kalData, TrkStrawHitVector& tshv ) {

    std::vector<TrkStrawHitSeed>const hseeds = kalData.kalSeed->hits();
    HelixTraj const htraj = *kalData.helixTraj;
    // compute particle velocity to 
    for(auto ths : hseeds ){
      // create a TrkStrawHit from this seed.
      size_t index = ths.index();
      const ComboHit& strawhit(kalData.chcol->at(index));
      const Straw& straw = _tracker->getStraw(strawhit.strawId());
      TrkStrawHit* trkhit = new TrkStrawHit(strawhit,straw,ths.index(),ths.t0(),ths.trkLen(),
					    _maxpull,_strHitW);
      assert(trkhit != 0);
      // set the initial ambiguity
      trkhit->setAmbig(ths.ambig());
      // refine the flightlength, as otherwise hits in the same plane are at exactly the same flt, which can cause problems
      TrkErrCode pstat = trkhit->updatePoca(&htraj);
      if(pstat.failure()){
        trkhit->setActivity(false);
      }
      tshv.push_back(trkhit);
    }
 // sort the hits by flightlength
    std::sort(tshv.begin(),tshv.end(),fcomp());
  }

  void 
  KalFit::makeTrkCaloHit  (KalFitData& kalData, TrkCaloHit *&tch){
    art::Ptr<CaloCluster> const& calo = kalData.kalSeed->caloCluster();
    if (calo.isNonnull()){
      mu2e::GeomHandle<mu2e::Calorimeter> ch;
      Hep3Vector cog = ch->geomUtil().mu2eToTracker(ch->geomUtil().diskToMu2e( calo->diskId(), calo->cog3Vector())); 
      // estimate fltlen from pitch
      double td = kalData.kalSeed->segments()[0].helix().tanDip();
      double sd = td/sqrt(1.0+td*td);
      double fltlen = cog.z()/sd - kalData.kalSeed->flt0();
      // t0 represents the time the particle reached the sensor; estimate that
      HitT0 ht0;
      //ht0._t0 = kalData.t0._t0 + _ttcalc.timeOfFlightTimeOffset(cog.z()) + _ttcalc.caloClusterTimeOffset();
      ht0._t0    = kalData.kalSeed->caloCluster()->time() + _ttcalc.caloClusterTimeOffset();
      ht0._t0err = _ttcalc.caloClusterTimeErr();
      
      Hep3Vector const& clusterAxis = Hep3Vector(0, 0, 1);//FIXME! should come from crystal
      double      crystalHalfLength = ch->caloInfo().getDouble("crystalZLength")/2.;
      tch = new TrkCaloHit(*kalData.kalSeed->caloCluster().get(), cog, crystalHalfLength, clusterAxis, ht0, fltlen, _calHitW, _dtoffset);
    }
  }


//-----------------------------------------------------------------------------
// '*' in front of the hit drift radius: the hit drift sign is not defined
//     and the drift ambiguity has been set to 0
// '?': the drift sign determined by the resolver is different from the MC truth
//-----------------------------------------------------------------------------
  void 
  KalFit::printHits(KalFitData& KRes, const char* Caller) {
    const KalRep* Trk  = KRes.krep;

    if (Trk == NULL)  return;

    printf("[KalFitHackNew::printHits] BEGIN called from %s _annealingStep:%i \n",Caller,_annealingStep);
    printf("---------------------------------------------------------------------------------");
    printf("-----------------------------------------------------\n");
      //      printf("%s",Prefix);
    printf("  TrkID       Address    N  NA      P       pT     costh    T0      T0Err   Omega");
    printf("      D0       Z0      Phi0   TanDip    Chi2    FCons\n");
    printf("---------------------------------------------------------------------------------");
    printf("-----------------------------------------------------\n");

    Hep3Vector trk_mom;

    double h1_fltlen = Trk->firstHit()->kalHit()->hit()->fltLen() - 10;
    trk_mom          = Trk->momentum(h1_fltlen);
    double mom       = trk_mom.mag();
    double pt        = trk_mom.perp();
    double costh     = trk_mom.cosTheta();
    double chi2      = Trk->chisq();
    int    nhits     = Trk->hitVector().size();
    int    nact      = Trk->nActive();
    double t0        = Trk->t0().t0();
    double t0err     = Trk->t0().t0Err();
//-----------------------------------------------------------------------------
// in all cases define momentum at lowest Z - ideally, at the tracker entrance
//-----------------------------------------------------------------------------
    double s1     = Trk->firstHit()->kalHit()->hit()->fltLen();
    double s2     = Trk->lastHit ()->kalHit()->hit()->fltLen();
    double s      = std::min(s1,s2);

    double d0     = Trk->helix(s).d0();
    double z0     = Trk->helix(s).z0();
    double phi0   = Trk->helix(s).phi0();
    double omega  = Trk->helix(s).omega();
    double tandip = Trk->helix(s).tanDip();

    double fit_consistency = Trk->chisqConsistency().consistency();
    int q         = Trk->charge();

    printf("%5i %16p %3i %3i %8.3f %7.3f %8.4f %7.3f %7.4f",
	   -1,
	   Trk,
	   nhits,
	   nact,
	   q*mom,pt,costh,t0,t0err
	   );

    printf(" %8.5f %8.3f %8.3f %8.4f %7.4f",omega,d0,z0,phi0,tandip
	   );
    printf(" %8.3f %10.3e\n",chi2,fit_consistency);
//-----------------------------------------------------------------------------
// print detailed information about the track hits
//-----------------------------------------------------------------------------
    printf("--------------------------------------------------------------------");
    printf("----------------------------------------------------------------");
    printf("--------------------------------------------\n");
    printf(" ih  SInd Flag      A     len         x        y        z      HitT     HitDt");
    printf(" Pl Pn  L  W     T0       Xs      Ys        Zs      resid  sigres");
    printf("   Rdrift   mcdoca  totErr hitErr  t0Err penErr extErr\n");
    printf("--------------------------------------------------------------------");
    printf("----------------------------------------------------------------");
    printf("--------------------------------------------\n");

    mu2e::TrkStrawHit     *hit;
    Hep3Vector            pos;
    const mu2e::ComboHit  *sh;
    const mu2e::Straw     *straw;
    int                   ihit;
    double                len;
    HepPoint              plen;

    ihit = 0;
    for (int it=0; it<nhits; ++it) {
      hit   = static_cast<TrkStrawHit*> (KRes.krep->hitVector().at(it));
      sh    = &hit->comboHit();
      straw = &hit->straw();

      hit->hitPosition(pos);

      len   = hit->fltLen();
      plen  = Trk->position(len);

      double mcdoca = _mcUtils->mcDoca(KRes.event,KRes.shDigiLabel.data(),straw);

      ihit += 1;
      printf("%3i %5i 0x%08x %1i %9.3f %8.3f %8.3f %9.3f %8.3f %7.3f",
             ihit,
             straw->id().asUint16(),
             hit->hitFlag(),
             hit->isActive(),
             len,
             //      hit->hitRms(),
             plen.x(),plen.y(),plen.z(),
             sh->time(), -1.//sh->dt()
             );

      printf(" %2i %2i %2i %2i",
             straw->id().getPlane(),
             straw->id().getPanel(),
             straw->id().getLayer(),
             straw->id().getStraw()
             );

      printf(" %8.3f",hit->hitT0().t0());

      double res, sigres;
      hit->resid(res, sigres, true);

      printf(" %8.3f %8.3f %9.3f %7.3f %7.3f",
             pos.x(),
             pos.y(),
             pos.z(),
             res,
             sigres
             );

      if      (hit->ambig()       == 0) printf(" * %6.3f",hit->driftRadius());
      else if (hit->ambig()*mcdoca > 0) printf("   %6.3f",hit->driftRadius()*hit->ambig());
      else                              printf(" ? %6.3f",hit->driftRadius()*hit->ambig());

      printf("  %7.3f  %6.3f %6.3f %6.3f %6.3f %6.3f\n",
             mcdoca,
             hit->totalErr(),
             hit->hitErr(),
             hit->t0Err(),
             hit->penaltyErr(),
             hit->temperature()*hit->driftVelocity()
             );
    }
  }


  void
  KalFit::makeMaterials(TrkStrawHitVector const& tshv, HelixTraj const& htraj,std::vector<DetIntersection>& detinter) {
  // fetcth the DetectorModel
    Mu2eDetectorModel const& detmodel{ art::ServiceHandle<BTrkHelper>()->detectorModel() };
    // loop over strawhits and extract the straws
    for (auto trkhit : tshv) {
   // find the DetElem associated this straw
      const DetStrawElem* strawelem = detmodel.strawElem(trkhit->straw());
      // create intersection object for this element; it includes all materials
      DetIntersection strawinter;
      strawinter.delem = strawelem;
      strawinter.pathlen = trkhit->fltLen();
      strawinter.thit = trkhit;
      // compute initial intersection: this gets updated each fit iteration
      strawelem->reIntersect(&htraj,strawinter);
      detinter.push_back(strawinter);
    }
  }

  unsigned KalFit::addMaterial(KalRep* krep) {
    _debug>3 && std::cout << __func__ << " called " << std::endl;
    unsigned retval(0);
// TTracker geometry
    // const Tracker& tracker = getTrackerOrThrow();
    const TTracker& ttracker = dynamic_cast<const TTracker&>(*_tracker);
// fetcth the DetectorModel
    Mu2eDetectorModel const& detmodel{ art::ServiceHandle<BTrkHelper>()->detectorModel() };
// storage of potential straws
    StrawFlightComp strawcomp(_maxmatfltdiff);
    std::set<StrawFlight,StrawFlightComp> matstraws(strawcomp);
// loop over Planes
    double strawradius = ttracker.strawRadius();
    unsigned nadded(0);
    // for(auto const& plane : ttracker.getPlanes()){
    for ( size_t i=0; i!= ttracker.nPlanes(); ++i){
      const auto& plane = ttracker.getPlane(i);
       _debug>3 && std::cout << __func__ << " plane " << plane.id() << std::endl;
      // if (!(plane.exists())) continue;
      // # of straws in a panel
      int nstraws = plane.getPanel(0).nStraws();
       _debug>3 && std::cout << __func__ << " nstraws " << nstraws << std::endl;
// get an approximate z position for this plane from the average position of the 1st and last straws
      // Hep3Vector s0 = plane.getPanel(0).getLayer(0).getStraw(0).getMidPoint();
      // plane id is id of 0th straw
      Hep3Vector s0 = plane.getPanel(0).getStraw(StrawId(plane.id())).getMidPoint();
       _debug>3 && std::cout << __func__ << " s0 via panel " << s0 << std::endl;
      // funky convention for straw numbering in a layer FIXME!!!!
      // Hep3Vector sn = plane.getPanel(0).getLayer(1).getStraw(2*plane.getPanel(0).getLayer(1).nStraws()-1).getMidPoint();
      Hep3Vector sn = plane.getPanel(0).getStraw(nstraws-1).getMidPoint();
      _debug>3 && std::cout << __func__ << " sn via panel " << sn << std::endl;
      double pz = 0.5*(s0.z() + sn.z());
      _debug>3 && std::cout << __func__ << " an approximate z position for this plane " << plane.id() << " " << pz << std::endl;
// find the transverse position at this z using the reference trajectory
      double flt = krep->referenceTraj()->zFlight(pz);
      HepPoint pos = krep->referenceTraj()->position(flt);
      Hep3Vector posv(pos.x(),pos.y(),pos.z());
// see if this position is in the active region.  Double the straw radius to be generous
      double rho = posv.perp();
      double rmin = s0.perp()-2*strawradius;
      double rmax = sn.perp()+2*strawradius;
      if(rho > rmin && rho < rmax){
  // loop over panels
        for(auto const& panel : plane.getPanels()){
          if (_debug>4) {
            std::cout << __func__ << " panel " << panel.id() << std::endl;
            std::cout << __func__ << " printing all straws in layer 0 " << std::endl;
            for (const auto straw_p : panel.getStrawPointers() ) {
              Straw const& straw(*straw_p);
              StrawId sid = straw.id();
              if ( sid.getLayer() != 0 ) continue;
              std::cout.width(7);
              std::cout << sid << ", ";
            }
            std::cout << std::endl;
            std::cout << __func__ << " printing all straws in layer 1 " << std::endl;
            for (const auto straw_p : panel.getStrawPointers() ) {
              Straw const& straw(*straw_p);
              StrawId sid = straw.id();
              if ( sid.getLayer() != 1 ) continue;
              std::cout.width(7);
              std::cout << sid << ", ";
            }
            std::cout << std::endl;
          }
      // get the straw direction for this panel
          // Hep3Vector sdir = panel.getLayer(0).getStraw(0).getDirection();
          Hep3Vector sdir = panel.getStraw(0).getDirection();
      // get the transverse direction to this and z
          static Hep3Vector zdir(0,0,1.0);
          Hep3Vector pdir = sdir.cross(zdir);
     //  project the position along this
          double prho = posv.dot(pdir);
      // test for acceptance of this panel
          if(prho > rmin && prho < rmax) {
          // translate the transverse position into a rough straw number
          // nstraws is the number of straws in the panel
            int istraw = (int)rint(nstraws*(prho-s0.perp())/(sn.perp()-s0.perp()));
            // take a few straws around this
            for(int is = max(0,istraw-3); is<min(nstraws,istraw+3); ++is){
              _debug>3 && std::cout << __func__ << " taking a few straws, istraw, is "
                                    << istraw << ", " << is << std::endl;
              _debug>3 && std::cout << __func__ << " straw id "
                                    << panel.getStraw(is).id() << std::endl;
              if (_debug>4) {
                if ( panel.getStraw(is).id().getLayer()==0) {
                  std::cout << __func__ << " straw id l0 by id "
                            << panel.getStraw(StrawId(panel.id().asUint16()+is)).id()
                            << std::endl;
                } else {
                  std::cout << __func__ << " straw id l1 by id "
                            << panel.getStraw(StrawId(panel.id().asUint16()+is)).id()
                            << std::endl;
                }
              }
              matstraws.insert(StrawFlight(panel.getStraw(is).id(),flt));
              ++nadded;
            }
          }
        }
      }
    }
// Now test if the Kalman rep hits these straws
    if(_debug>2)std::cout << "Found " << matstraws.size() << " unique possible straws " << " out of " << nadded << std::endl;
    for(auto const& strawflt : matstraws){
      const DetStrawElem* strawelem = detmodel.strawElem(strawflt._id);
      DetIntersection strawinter;
      strawinter.delem = strawelem;
      strawinter.pathlen = strawflt._flt;
      if(strawelem->reIntersect(krep->referenceTraj(),strawinter)){
// If the rep already has a material site for this element, skip it
        std::vector<const KalMaterial*> kmats;
        krep->findMaterialSites(strawelem,kmats);
        if(_debug>2)std::cout << "found intersection with straw " << strawelem->straw()->id() << " with "
        << kmats.size() << " materials " << std::endl;
// test material isn't on the track
        bool hasmat(false);
        for(auto kmat : kmats ){
          const DetStrawElem* kelem = dynamic_cast<const DetStrawElem*>(kmat->detIntersection().delem);
          if(kelem != 0){
            StrawFlight ksflt(kelem->straw()->id(),kmat->globalLength());
            if(_debug>2)std::cout << " comparing flights " << kmat->globalLength() << " and " << strawflt._flt << std::endl;
            if(!strawcomp.operator()(strawflt,ksflt)){
              if(_debug>2)std::cout << "operator returned false!!" << std::endl;
              // this straw is already on the track: stop
              hasmat = true;
              break;
            }
          }
        }
        if(kmats.size() == 0 || !hasmat) {
          if(_debug>2)std::cout << "Adding material element" << std::endl;
          // this straw doesn't have an entry in the Kalman fit: add it`
          DetIntersection detinter(strawelem, krep->referenceTraj(),strawflt._flt);
          krep->addInter(detinter);
          ++retval;
        }
      }
    }
    if(_debug>1)std::cout << "Added " << retval << " new material sites" << std::endl;
    return retval;
  }

  bool
  KalFit::weedHits(KalFitData& kalData, int iter) {
    // Loop over HoTs and find HoT with largest contribution to chi2.  If this value
    // is greater than some cut value, deactivate that HoT and reFit
    KalRep* krep = kalData.krep;
    bool retval(false);
    double worst = -1.;
    //    TrkHit* worsthit = 0;
    TrkStrawHit  *worsthit = 0;
    TrkHitVector *thv      = &(krep->hitVector());

    for (auto ihit=thv->begin();ihit!=thv->end(); ++ihit){
      //      TrkHit* hit = *ihit;
      TrkStrawHit*hit = dynamic_cast<TrkStrawHit*>(*ihit);
      if (hit == 0)     continue;
      if (hit->isActive()) {
        double resid, residErr;
        if(hit->resid(resid, residErr, true)){
          double value = fabs(resid/residErr);
          if (value > _maxhitchi && value > worst) {
            worst = value;
            worsthit = hit;
          }
        }
      }
    }

    if (iter == -1) iter = _ambigresolver.size()-1;

    if(0 != worsthit){
      retval = true;
      worsthit->setActivity(false);
      worsthit->setFlag(TrkHit::weededHit);
      if (_resolveAfterWeeding) {
//-----------------------------------------------------------------------------
// _resolveAfterWeeding=0 makes changes in the logic fully reversible
//-----------------------------------------------------------------------------
        _ambigresolver[iter]->resolveTrk(krep);
      }
      TrkErrCode fitstat = krep->fit();
      krep->addHistory(fitstat, "HitWeed");
      
      if (_debug > 0) {
	char msg[200];
        sprintf(msg,"KalFit::weedHits Iteration = %2i success = %i",iter,fitstat.success());
        printHits(kalData,msg);
      }
      
      // Recursively iterate
      kalData.nweediter++;
      if (fitstat.success() && kalData.nweediter < _maxweed) {
        retval |= weedHits(kalData,iter);
      }
    }
    return retval;
  }

  bool
  KalFit::unweedHits(KalFitData& kalData, double maxchi) {
    bool retval = unweedBestHit(kalData, maxchi);
    // if any hits were added, re-analyze ambiguity
    if (retval && _resolveAfterWeeding) {
      KalRep* krep = kalData.krep;
      // 2015-04-12 P.Murat: '_resolveAfterWeeding' is here to make my changes fully reversible
      // I think, resolving ambiguities before each fit, makes a lot of sense
      //
      // Moved to after iteration: PanelAmbig resolver can change the state of hit resulting in infinte
      // loop if the resolver is called each iteration
      int last = _herr.size()-1;
      _ambigresolver[last]->resolveTrk(krep);
      if(!krep->fitCurrent()){
    // if this changed the track state, refit it
        krep->resetFit();
        TrkErrCode fitstat = krep->fit();
        krep->addHistory(fitstat, "HitUnWeedResolver");
  
	if (_debug > 0) {
	  char msg[200];
	  sprintf(msg,"KalFit::unweedHits Iteration = %2i success = %i",last,fitstat.success());
	  printHits(kalData,msg);
	}
      }
    }
    return retval;
  }

  bool
  KalFit::unweedBestHit(KalFitData& kalData, double maxchi) {
    // Loop over inactive HoTs and find the one with the smallest contribution to chi2.  If this value
    // is less than some cut value, reactivate that HoT and reFit
    KalRep*   krep = kalData.krep;
    bool      retval(false);
    double    best = 1.e12;
// no need to cast
    TrkHit*   besthit = 0;
    const TrkHitVector* thv = &(krep->hitVector());
    for (auto ihit=thv->begin();ihit!=thv->end(); ++ihit){
      TrkHit* hit = *ihit;
      if (!hit->isActive()) {
        double resid, residErr;
        if(hit->resid(resid, residErr, true)){
          double chival = fabs(resid/residErr);
  // test both for a good chisquared and for the drift radius to be physical
          if (chival < maxchi && hit->isPhysical(maxchi) && chival < best) {
            best = chival;
            besthit = hit;
          }
        }
      }
    }
    if(0 != besthit){
      retval = true;
      besthit->setActivity(true);
      besthit->setFlag(TrkHit::unweededHit);
      TrkErrCode fitstat = krep->fit();
      if (fitstat.success() && besthit->isActive() ) {
	krep->addHistory(fitstat, "HitUnWeed");
	// Recursively iterate
        retval |= unweedBestHit(kalData, maxchi);
      }
    }
    return retval;
  }

  BField const&
  KalFit::bField() const {
    if(_bfield == 0){
      GeomHandle<BFieldConfig> bfconf;
      if(_fieldcorr){
// create a wrapper around the mu2e field
        _bfield = new BaBarMu2eField();
      } else {
// create a fixed field using the nominal value
        GeomHandle<BFieldConfig> bfconf;
        _bfield=new BFieldFixed(bfconf->getDSUniformValue());
        assert(_bfield != 0);
      }
    }
    return *_bfield;
  }

  const TrkVolume*
  KalFit::trkVolume(trkDirection trkdir) const {
    //FIXME!!!!
    return 0;
  }

  void
  KalFit::initT0(KalFitData&kalData) {
    TrkT0 t0;
    const CaloCluster* cl = kalData.caloCluster;
    double          t0flt = kalData.krep->referenceTraj()->zFlight(0);//htraj.zFlight(0.0);
    // get flight distance of z=0
    // estimate the momentum at that point using the helix parameters.  This is
    // assumed constant for this crude estimate
    double loclen;
    double fltlen(0.0);
    const HelixTraj* htraj = dynamic_cast<const HelixTraj*>(kalData.krep->referenceTraj()->localTrajectory(fltlen,loclen));
    double mom = TrkMomCalculator::vecMom(*htraj,bField(),t0flt).mag();
    // compute the particle velocity
    double vflt = kalData.krep->particleType().beta(mom)*CLHEP::c_light;

    if (cl) {
//-----------------------------------------------------------------------------
// calculate the path length of the particle from the middle of the Tracker to the
// calorimeter, cl->Z() is calculated wrt the tracker center
// _dtoffset : global time offset between the tracker and the calorimeter, 
//             think of an average cable delay
//-----------------------------------------------------------------------------
      const HelixTraj* hel  = kalData.helixTraj;
      Hep3Vector       gpos = _calorimeter->geomUtil().diskToMu2e(cl->diskId(),cl->cog3Vector());
      Hep3Vector       tpos = _calorimeter->geomUtil().mu2eToTracker(gpos);
      double           path = tpos.z()/hel->sinDip();

      t0._t0    =  cl->time() + _dtoffset - path/vflt;
      t0._t0err = 1;
      //set the new T0
      kalData.krep->setT0(t0, t0flt);
    } else {
      using namespace boost::accumulators;
      // make an array of all the hit times, correcting for propagation delay
      unsigned nind = kalData.krep->hitVector().size();
      std::vector<double> times;
      std::vector<double> timesweight;
      times.reserve(nind);
      timesweight.reserve(nind);

      // use the reference trajectory, as that's what all the existing hits do
      const TrkDifPieceTraj* reftraj = (kalData.krep->referenceTraj());
      // loop over hits
      double      htime(0);    
      for(auto ith=kalData.krep->hitVector().begin(); ith!=kalData.krep->hitVector().end(); ++ith){
	(*ith)->trackT0Time(htime, t0flt, reftraj, vflt);
	times.push_back(htime);
	timesweight.push_back((*ith)->t0Weight());
      }

      // find the median time
      accumulator_set<double,stats<tag::weighted_variance >,double >         wmean;
      //fill the accumulator using the weights
      int nhits(times.size());
      for (int i=0; i<nhits; ++i){
	wmean(times.at(i), weight=timesweight.at(i));
      }
      t0._t0    = extract_result<tag::weighted_mean>(wmean);
      t0._t0err = sqrt(extract_result<tag::weighted_variance>(wmean)/nhits);
      //set the new T0
      kalData.krep->setT0(t0, t0flt);
    }
  }

  void
  KalFit::updateCalT0(KalFitData& kalData){

    TrkT0 t0;
    double mom, vflt, path, t0flt, flt0(0.0);

    KalRep* krep   = kalData.krep;
    bool converged = TrkHelixUtils::findZFltlen(krep->traj(),0.0,flt0);

    // get helix from kalrep

    const CaloCluster* cl = kalData.caloCluster;

    HelixTraj trkHel(krep->helix(flt0).params(),krep->helix(flt0).covariance());

                                        // get flight distance of z=0
    t0flt = trkHel.zFlight(0.0);

    if (converged) {
//-----------------------------------------------------------------------------
// estimate the momentum at that point using the helix parameters.
// This is assumed constant for this crude estimate
// compute the particle velocity
//-----------------------------------------------------------------------------
      mom  = TrkMomCalculator::vecMom(trkHel,bField(),t0flt).mag();
      vflt = krep->particleType().beta(mom)*CLHEP::c_light;
//-----------------------------------------------------------------------------
// path length of the particle from the middle of the Tracker to the  calorimeter
// set dummy error value
//-----------------------------------------------------------------------------
      Hep3Vector gpos = _calorimeter->geomUtil().diskToMu2e(cl->diskId(),cl->cog3Vector());
      Hep3Vector tpos = _calorimeter->geomUtil().mu2eToTracker(gpos);

      path      = tpos.z()/trkHel.sinDip();
      t0._t0    = cl->time() + _dtoffset - path/vflt;
      t0._t0err = 1.;

      krep->setT0(t0,flt0);
      updateHitTimes(krep);
    }
  }


  bool
  KalFit::updateT0(KalFitData& kalData){
    KalRep* krep = kalData.krep;
    using namespace boost::accumulators;
    TrkHitVector *thv = &(krep->hitVector());
    bool retval(false);
// need to have a valid fit
    if(krep->fitValid()){
// find the global fltlen associated with z=0.
      double flt0(0.0);
      bool converged = TrkHelixUtils::findZFltlen(krep->traj(),0.0,flt0);
      if(converged){
        std::vector<double> hitt0; // store t0, to allow outlyer removal
        std::vector<double> hitt0err;
        size_t nhits = krep->hitVector().size();
        hitt0.reserve(nhits);
        hitt0err.reserve(nhits);
        // loop over the hits
        for(auto ihit=thv->begin(); ihit != thv->end(); ihit++){
          TrkHit* hit = *ihit;
          if(hit->isActive() && hit->hasResidual()){
            // find the residual, exluding this hits measurement
            double resid,residerr;
	    double pTime, doca;//propagation-time
	    CLHEP::Hep3Vector trjDir(krep->traj().direction(hit->fltLen()));
            if(krep->resid(hit,resid,residerr,true)){
	      if (hit->signalPropagationTime(pTime, doca, resid, residerr, trjDir)){	      
                // subtracting hitT0 makes this WRT the previous track t0
                hitt0.push_back(hit->time() - pTime - hit->hitT0()._t0);
                // assume residual error dominates
                hitt0err.push_back(residerr);
	      }
            }
          }
        }
        if(hitt0.size() >1){
          TrkT0 t0;
          // find the median
          accumulator_set<double, stats<tag::median(with_p_square_quantile) > > med;
          med = std::for_each( hitt0.begin(), hitt0.end(), med );
          t0._t0 = extract_result<tag::median>(med);
          // iterate an outlier search and linear fit until the set of used hits doesn't change
          bool changed(true);
          std::vector<bool> used(hitt0.size(),true);
          unsigned niter(0);
          while(changed && niter < 10){
            niter++;
            changed = false;
            accumulator_set<double,stats<tag::weighted_variance>,double > wmean;
            for(unsigned ihit=0;ihit<hitt0.size();ihit++){
              bool useit = fabs(hitt0[ihit]-t0._t0) < _t0nsig*hitt0err[ihit];
              changed |= useit != used[ihit];
              used[ihit] = useit;
              if(useit){
                wmean(hitt0[ihit], weight=1.0/(hitt0err[ihit]*hitt0err[ihit]));
              }
            }
            unsigned nused = extract_result<tag::count>(wmean);
            if(nused > 1){
              t0._t0 = extract_result<tag::weighted_mean>(wmean);
              t0._t0err = sqrt(extract_result<tag::weighted_variance>(wmean)/nused);
            } else {
              break;
            }
          }
          // reset t0
          if(!changed){
            // put in t0 from the track.
            t0._t0 += krep->t0()._t0;
            krep->setT0(t0,flt0);
            updateHitTimes(krep);
            retval = true;
          }
        }
      }
    }
    return retval;
  }
  
  void
  KalFit::updateHitTimes(KalRep* krep) {
  // compute the time the track came closest to the sensor for each hit, starting from t0 and working out.
  // this function allows for momentum change along the track.
  // find the bounding hits on either side of this
    TrkHitVector *thv = &(krep->hitVector());
    std::sort(thv->begin(),thv->end(),fcomp());
    TrkHitVector::iterator ihigh;
    TrkHitVector::reverse_iterator ilow;
    findBoundingHits(krep, krep->flt0(),ilow,ihigh);
    // reset all the hit times
    double flt0 = krep->flt0();
    TrkT0 hitt0 = krep->t0();
    for(TrkHitVector::iterator ihit= ihigh;ihit != thv->end(); ++ihit){
      TrkHit* hit = *ihit;
      double flt1 = hit->fltLen();
// particle transit time to this hit from the reference
      double tflt = krep->transitTime(flt0, flt1);
// update the time in the TrkT0 object
      hitt0._t0 += tflt;
      //      (*ihit)->updateHitT0(hitt0);
      (*ihit)->setHitT0(hitt0);
// update the reference flightlength
      flt0 = flt1;
    }
// now the same, moving backwards.
    flt0 = krep->flt0();
    hitt0 = krep->t0();
    for(TrkHitVector::reverse_iterator ihit= ilow;ihit != thv->rend(); ++ihit){
      TrkHit* hit = *ihit;
      double flt1 = hit->fltLen();
      double tflt = krep->transitTime(flt0, flt1);
      hitt0._t0 += tflt;
      //      (*ihit)->updateHitT0(hitt0);
      (*ihit)->setHitT0(hitt0);
      flt0 = flt1;
    }
  }

  void
  KalFit::findBoundingHits(KalRep* krep,double flt0,
			   TrkHitVector::reverse_iterator& ilow,
			   TrkHitVector::iterator& ihigh) {
    TrkHitVector* hits = &(krep->hitVector());
    ilow = hits->rbegin();
    ihigh = hits->begin();
    while(ilow != hits->rend() && (*ilow)->fltLen() > flt0 )++ilow;
    while(ihigh != hits->end() && (*ihigh)->fltLen() < flt0 )++ihigh;
  }

// attempt to extend the fit to the specified location
  TrkErrCode KalFit::extendFit(KalRep* krep) {
    TrkErrCode retval;
    // find the downstream and upstream Z positions to extend to
    if(_exdown != noextension){
      double downz = extendZ(_exdown);
    // convert to flightlength using the fit trajectory
      double downflt = krep->pieceTraj().zFlight(downz);
    // actually extend the track
      retval = krep->extendThrough(downflt);
    }
    // same for upstream extension
    if(retval.success() && _exup != noextension){
      double upz = extendZ(_exup);
      double upflt = krep->pieceTraj().zFlight(upz);
      retval = krep->extendThrough(upflt);
    }
    return retval;
  }

  double KalFit::extendZ(extent ex) {
    double retval(0.0);
    if(ex == target){
      GeomHandle<StoppingTarget> target;
      GeomHandle<DetectorSystem> det;
      retval = det->toDetector(target->centerInMu2e()).z() - 0.5*target->cylinderLength();
    } else if(ex == ipa) {
    // the following is wrong FIXME!!
      GeomHandle<StoppingTarget> target;
      GeomHandle<DetectorSystem> det;
      retval = det->toDetector(target->centerInMu2e()).z() +  0.5*target->cylinderLength();
    } else if(ex == tracker) {
      retval = 0.0;
    } else if(ex == calo) {
      GeomHandle<Calorimeter> cg;
      return cg->caloInfo().getDouble("envelopeZ1");
    }
    return retval;
  }

  void KalFit::findTrkCaloHit(KalRep*krep, TrkCaloHit*tch){
    for(auto ith=krep->hitVector().begin(); ith!=krep->hitVector().end(); ++ith){
      TrkCaloHit* tsh = dynamic_cast<TrkCaloHit*>(*ith);
      if(tsh != 0) {
	tch = tsh;
	break;
      }
    }

  }
  

}
