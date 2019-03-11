// C++ includes.
#include <cmath>
#include <iostream>
#include <cstdlib>

// Framework includes.
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e includes.
#include "ConditionsService/inc/AcceleratorParams.hh"
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "ConditionsService/inc/DAQParams.hh"
#include "GlobalConstantsService/inc/GlobalConstantsHandle.hh"
#include "GlobalConstantsService/inc/PhysicsParams.hh"
#include "GlobalConstantsService/inc/ParticleDataTable.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/WorldG4.hh"
#include "GeometryService/inc/DetectorSystem.hh"
#include "GeometryService/inc/Mu2eEnvelope.hh"
#include "DataProducts/inc/PDGCode.hh"
#include "ConfigTools/inc/SimpleConfig.hh"
#include "Mu2eUtilities/inc/rm48.hh"
#include "GeneralUtilities/inc/safeSqrt.hh"
#include "StoppingTargetGeom/inc/StoppingTarget.hh"
#include "ExtinctionMonitorFNAL/Geometry/inc/ExtMonFNAL.hh"
#include "CalorimeterGeom/inc/Calorimeter.hh"
#include "MCDataProducts/inc/GenParticleCollection.hh"

#include "CRYGenerator.h"
#include "CRYSetup.h"
#include "EventGenerator/inc/CosmicCRY.hh"

// From CLHEP
#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Vector/ThreeVector.h"

using CLHEP::Hep3Vector;
using CLHEP::HepLorentzVector;

namespace mu2e 
{
  // The following part is needed for the RNG
  template<class T> class RNGWrapper {
    public:
      static void set(T* object, double (T::*func)(void));
      static double rng(void);
    private:
      static T* m_obj;
      static double (T::*m_func)(void);
  };// end of RNGWrapper class

  template<class T> T* RNGWrapper<T>::m_obj;
  template<class T> double (T::*RNGWrapper<T>::m_func)(void);
  template<class T> void RNGWrapper<T>::set(T* object, double (T::*func)(void)) {
    m_obj = object; m_func = func;
  }
  template<class T> double RNGWrapper<T>::rng(void) { return (m_obj->*m_func)(); }


  CosmicCRY::CosmicCRY( art::Run& run,
      const SimpleConfig& config, CLHEP::HepRandomEngine & engine )
    : _verbose(config.getInt("cosmicCRY.verbose", 0) )
      , _returnMuons( config.getBool("cosmicCRY.returnMuons", true))
      , _returnNeutrons( config.getBool("cosmicCRY.returnNeutrons", true))
      , _returnProtons( config.getBool("cosmicCRY.returnProtons", true))
      , _returnGammas( config.getBool("cosmicCRY.returnGammas", true))
      , _returnElectrons( config.getBool("cosmicCRY.returnElectrons", true))
      , _returnPions( config.getBool("cosmicCRY.returnPions", true))
      , _returnKaons( config.getBool("cosmicCRY.returnKaons", true))

      , _month(config.getInt("cosmicCRY.month", 6))
        , _day(config.getInt("cosmicCRY.day", 21))
        , _year(config.getInt("cosmicCRY.year", 2020))
        , _latitude( config.getDouble("cosmicCRY.latitude", 41.8))
        , _altitude( config.getInt("cosmicCRY.altitude", 0))
        , _subboxLength( config.getDouble("cosmicCRY.subboxLength", 100.))

        , _setupString("")
        , _refY0(config.getDouble("cosmicCRY.refY0", 20000.))
        , _refPointChoice(config.getString("cosmicCRY.refPoint", "UNDEFINED"))
        , _directionChoice(config.getString("cosmicCRY.directionChoice", "ALL"))
        , _cosmicReferencePointInMu2e()
        , _vertical(false)
        , _projectToEnvelope(config.getBool("cosmicCRY.projectToEnvelope", false))
        , _geomInfoObtained(false)
        , _targetBoxXmin(config.getDouble("cosmicCRY.targetBoxXmin", -1000))
        , _targetBoxXmax(config.getDouble("cosmicCRY.targetBoxXmax", 1000))
        , _targetBoxYmin(config.getDouble("cosmicCRY.targetBoxYmin", -1000))
        , _targetBoxYmax(config.getDouble("cosmicCRY.targetBoxYmax", 1000))
        , _targetBoxZmin(config.getDouble("cosmicCRY.targetBoxZmin", -1000))
        , _targetBoxZmax(config.getDouble("cosmicCRY.targetBoxZmax", 1000))
        {
          _cryDataPath = std::string(std::getenv("CRYDATAPATH"));
          if( _cryDataPath.length() == 0) 
          {
            mf::LogError("CosmicCRY") << "no variable CRYDATAPATH set. Exit now.";
            throw cet::exception("Rethrow") << "This job cannot continue without a valid CRYDATAPATH.";
          }

          createSetupString();
          _crySetup = new CRYSetup(_setupString, _cryDataPath);

          RNGWrapper<CLHEP::HepRandomEngine>::set(
              &engine,
              &CLHEP::HepRandomEngine::flat);
          _crySetup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);

          if (_verbose > 1) 
            engine.showStatus();

          _cryGen = std::make_unique<CRYGenerator>(_crySetup);

          _GeV2MeV = CLHEP::GeV / CLHEP::MeV;
          _m2mm = CLHEP::m / CLHEP::mm;
        }


  const double CosmicCRY::getLiveTime() {
    return _cryGen->timeSimulated();
  }

  const double CosmicCRY::getShowerSumEnergy() {
    return _showerSumEnergy;
  }

  void CosmicCRY::generate( GenParticleCollection& genParts )
  {
    // Ref point, need to be here so that geometry info can be obtained
    // , but let's do this only once
    if (!_geomInfoObtained) {
      GeomHandle<Mu2eEnvelope> env;
      GeomHandle<WorldG4>  worldGeom;
      GeomHandle<ExtMonFNAL::ExtMon> extMonFNAL;
      GeomHandle<DetectorSystem> detsys;

      // slightly smaller box to avoid rounding error problem if any
      double deltaX = 1; // mm
      // _targetBoxXmin = env->xmin() + deltaX;
      // _targetBoxXmax = env->xmax() - deltaX;
      // _targetBoxYmin = env->ymin() + deltaX;
      // _targetBoxYmax = env->ymax() - deltaX;
      // _targetBoxZmin = env->zmin() + deltaX;
      // _targetBoxZmax = env->zmax() - deltaX;

      _worldXmin = worldGeom->mu2eOriginInWorld().x() - worldGeom->halfLengths()[0] + deltaX;
      _worldXmax = worldGeom->mu2eOriginInWorld().x() + worldGeom->halfLengths()[0] - deltaX;
      _worldYmin = worldGeom->mu2eOriginInWorld().y() - worldGeom->halfLengths()[1] + deltaX;
      _worldYmax = worldGeom->mu2eOriginInWorld().y() + worldGeom->halfLengths()[1] - deltaX;
      _worldZmin = worldGeom->mu2eOriginInWorld().z() - worldGeom->halfLengths()[2] + deltaX;
      _worldZmax = worldGeom->mu2eOriginInWorld().z() + worldGeom->halfLengths()[2] - deltaX;

      mf::LogInfo("CosmicCRY") << "Target box: " << _targetBoxXmin << ", " << 
        _targetBoxXmax << ", " << _targetBoxYmin << ", " << _targetBoxYmax << 
        ", " << _targetBoxZmin << ", " << _targetBoxZmax;
      mf::LogInfo("CosmicCRY") << "World: " << _worldXmin << ", " << 
        _worldXmax << ", " << _worldYmin << ", " << _worldYmax << ", " 
        << _worldZmin << ", " << _worldZmax;

      if (_refPointChoice == "TRACKER") 
        _cosmicReferencePointInMu2e = Hep3Vector(detsys->getOrigin().x(),
            _refY0, detsys->getOrigin().z());
      else if (_refPointChoice == "EXTMONFNAL") 
        _cosmicReferencePointInMu2e = 
          Hep3Vector(extMonFNAL->detectorCenterInMu2e().x(),
              _refY0, extMonFNAL->detectorCenterInMu2e().z());
      else if (_refPointChoice == "CALO") 
      {
        GeomHandle<Calorimeter> calorimeter;
        _cosmicReferencePointInMu2e = Hep3Vector(detsys->getOrigin().x(),
            _refY0, calorimeter->disk(0).geomInfo().origin().z());
      }
      else if (_refPointChoice == "UNDEFINED") 
        _cosmicReferencePointInMu2e = Hep3Vector(0., _refY0, 0.);

      _geomInfoObtained = true;
    }

    // std::cout << _cosmicReferencePointInMu2e << std::endl;
    // Getting CRY particles
    std::vector<CRYParticle*> *secondaries = new std::vector<CRYParticle*>;
    _cryGen->genEvent(secondaries);

    double secondPtot = 0.;
    _showerSumEnergy = 0.;

    std::ostringstream oss;
    for (unsigned j=0; j<secondaries->size(); j++) {
      CRYParticle* secondary = (*secondaries)[j];

      GlobalConstantsHandle<ParticleDataTable> pdt;
      const HepPDT::ParticleData& p_data = pdt->particle(secondary->PDGid()).ref();
      double mass = p_data.mass().value(); // in MeV


      double ke = secondary->ke(); // MeV by default in CRY
      double totalE = ke + mass;
      _showerSumEnergy += totalE;
      double totalP = safeSqrt(totalE * totalE - mass * mass);

      secondPtot += totalP;

      // Change coordinate system since y points upward, z points along
      // the beam line; which make cry(xyz) -> mu2e(zxy), uvw -> mu2e(zxy)
      Hep3Vector position(
          secondary->y() * _m2mm + _cosmicReferencePointInMu2e.x(),
          secondary->z() * _m2mm + _cosmicReferencePointInMu2e.y(),
          secondary->x() * _m2mm + _cosmicReferencePointInMu2e.z()); // to mm
      HepLorentzVector mom4(totalP*secondary->v(), totalP*secondary->w(),
          totalP*secondary->u(), totalE);

      if (_projectToEnvelope) {
        // Moving the CRY particle around: find all intersections with Mu2e
        // envelope, then set the position at the *first* intersection
        _targetBoxIntersections.clear();
        calIntersections(position, mom4.vect(),
            _targetBoxIntersections, _targetBoxXmin, _targetBoxXmax,
            _targetBoxYmin, _targetBoxYmax, _targetBoxZmin, _targetBoxZmax);

        if (_targetBoxIntersections.size() > 0) {
          int idx = 0;
          double highestY = _targetBoxIntersections.at(idx).y();
          for (unsigned i = 0; i < _targetBoxIntersections.size(); ++i) {
            if (_targetBoxIntersections.at(i).y() > highestY) {
              idx = i;
              highestY = _targetBoxIntersections.at(idx).y();
            }
          }

          position = _targetBoxIntersections.at(idx);
          genParts.push_back(GenParticle(static_cast<PDGCode::type>(secondary->PDGid()),
                GenId::cosmicCRY, position, mom4,
                secondary->t() - _cryGen->timeSimulated()));
        }
      }
      else
        genParts.push_back(GenParticle(static_cast<PDGCode::type>(secondary->PDGid()),
              GenId::cosmicCRY, position, mom4,
              secondary->t() - _cryGen->timeSimulated()));

      if (_verbose > 1) {
        oss << "Secondary " << j 
          << ": " << CRYUtils::partName(secondary->id()) 
          << " (pdgId " << secondary->PDGid() << ")"
          << ", kinetic energy " << secondary->ke()  << " MeV"
          << ", position " 
          << "(" << secondary->x()
          << ", " << secondary->y()
          << ", " << secondary->z()
          << ") m"
          << ", position at world boundaries" 
          << "("  << position.x()
          << ", " << position.y()
          << ", " << position.z()
          << ") m"
          << ", time " << secondary->t() << " sec"
          << ", mass: " << mass
          << ", mom: " << totalP
          << ", mom dir.: " << secondary->u() <<", " << secondary->v()
          << ", " << secondary->w() << "\n";

      }

      delete secondary;
    }

    if (_verbose > 1) {
      oss << "Total E: " << _showerSumEnergy;
      mf::LogInfo("CosmicCRY") << oss.str();
    }

    delete secondaries;
  }

  void CosmicCRY::createSetupString()
  {
    if (_returnMuons) 
      _setupString.append("returnMuons 1 "); // must have the trailing white space
    else
      _setupString.append("returnMuons 0 ");

    if (_returnNeutrons) 
      _setupString.append("returnNeutrons 1 ");
    else
      _setupString.append("returnNeutrons 0 ");

    if (_returnProtons) 
      _setupString.append("returnProtons 1 ");
    else
      _setupString.append("returnProtons 0 ");

    if (_returnGammas) 
      _setupString.append("returnGammas 1 ");
    else
      _setupString.append("returnGammas 0 ");

    if (_returnElectrons) 
      _setupString.append("returnElectrons 1 ");
    else
      _setupString.append("returnElectrons 0 ");

    if (_returnPions) 
      _setupString.append("returnPions 1 ");
    else
      _setupString.append("returnPions 0 ");

    if (_returnKaons) 
      _setupString.append("returnKaons 1 ");
    else
      _setupString.append("returnKaons 0 ");

    std::ostringstream oss;
    oss << "date " << _month << "-" << _day << "-" << _year << " ";
    oss << "latitude " << _latitude << " ";
    oss << "altitude " << _altitude << " ";
    oss << "subboxLength " << _subboxLength << " ";

    _setupString.append(oss.str());
  }

  void CosmicCRY::calIntersections(Hep3Vector orig, Hep3Vector dir,
      std::vector<CLHEP::Hep3Vector> intersections, double xMin, double xMax,
      double yMin, double yMax, double zMin, double zMax) {
    // roof: _targetBoxYmax, _targetBoxXmin, _targetBoxXmax, _targetBoxZmin, _targetBoxZmax
    // skip projection if the particle goes parallely to the plane
    if (dir.y() != 0.) {
      double t = (yMax - orig.y()) / dir.y();
      double x1 = dir.x() * t + orig.x();
      double z1 = dir.z() * t + orig.z();
      if (pointInBox(x1, z1, xMin, zMin, xMax, zMax)) {
        _targetBoxIntersections.push_back(Hep3Vector(x1, yMax, z1));
      }
    }

    //east: zMin, xMin, xMax, yMin, yMax
    if (dir.z() != 0.) {
      double t = (zMin - orig.z()) / dir.z();
      double x1 = dir.x() * t + orig.x();
      double y1 = dir.y() * t + orig.y();
      if (pointInBox(x1, y1, xMin, yMin, xMax, yMax)) {
        _targetBoxIntersections.push_back(Hep3Vector(x1, y1, zMin));
      }
    }

    //west: zMax, xMin, xMax, yMin, yMax
    if (dir.z() != 0.) {
      double t = (zMax - orig.z()) / dir.z();
      double x1 = dir.x() * t + orig.x();
      double y1 = dir.y() * t + orig.y();
      if (pointInBox(x1, y1, xMin, yMin, xMax, yMax)) {
        _targetBoxIntersections.push_back(Hep3Vector(x1, y1, zMax));
      }
    }

    //south: xMin, yMin, yMax, zMin, zMax
    if (dir.x() != 0.) {
      double t = (xMin - orig.x()) / dir.x();
      double z1 = dir.z() * t + orig.z();
      double y1 = dir.y() * t + orig.y();
      if (pointInBox(z1, y1, zMin, yMin, zMax, yMax)) {
        _targetBoxIntersections.push_back(Hep3Vector(xMin, y1, z1));
      }
    }

    //north: xMax, yMin, yMax, zMin, zMax
    if (dir.x() != 0.) {
      double t = (xMax - orig.x()) / dir.x();
      double z1 = dir.z() * t + orig.z();
      double y1 = dir.y() * t + orig.y();
      if (pointInBox(z1, y1, zMin, yMin, zMax, yMax)) {
        _targetBoxIntersections.push_back(Hep3Vector(xMax, y1, z1));
      }
    }

  }

  bool CosmicCRY::pointInBox(double x, double y, double x0, double y0,
      double x1, double y1)
  {
    bool ret = false;
    if ((x >= x0) && (x <= x1) && (y >= y0) && (y <= y1)) {
      ret = true;
    }
    return ret;
  }
}
