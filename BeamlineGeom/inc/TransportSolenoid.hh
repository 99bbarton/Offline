#ifndef BeamlineGeom_TransportSolenoid_hh
#define BeamlineGeom_TransportSolenoid_hh

//
// Class to represent the transport solenoid
//
#include "BeamlineGeom/inc/StraightSection.hh"
#include "BeamlineGeom/inc/Coil.hh"
#include "BeamlineGeom/inc/Collimator_TS1.hh"
#include "BeamlineGeom/inc/Collimator_TS3.hh"
#include "BeamlineGeom/inc/Collimator_TS5.hh"
#include "BeamlineGeom/inc/PbarWindow.hh"
#include "BeamlineGeom/inc/TorusSection.hh"
#include "BeamlineGeom/inc/TSSection.hh"

#include "GeneralUtilities/inc/EnumToStringSparse.hh"

// C++ includes
#include <algorithm>
#include <map>
#include <memory>
#include <vector>

// cet
#include "cetlib/exception.h"

namespace mu2e {

  class TransportSolenoid {
    
    friend class BeamlineMaker;

  public:
    TransportSolenoid() :
      _rTorus(0.), _rVac(0.)
    {
      // Reserve number of coils
      for ( unsigned iTS = TSRegion::TS1 ; iTS <= TSRegion::TS5 ; iTS++ ) 
        _coilMap[ (TSRegion)iTS ].reserve( getNCoils( (TSRegion)iTS ) );
    }
    
    // use compiler-generated copy c'tor, copy assignment, and d'tor

    // - only the following enums should be used for the following
    //   accessors
    class TSRegionDetail {
    public:
      enum enum_type  { unknown, TS1, TS2, TS3, TS4, TS5 };
      static std::string const& typeName() {
        static std::string type("TSRegion"); return type;
      }
      static std::map<enum_type,std::string> const& names() {
        static std::map<enum_type,std::string> nam;
        
        if ( nam.empty() ) {
          nam[unknown] = "unknown";
          nam[TS1]     = "TS1";
          nam[TS2]     = "TS2";
          nam[TS3]     = "TS3";
          nam[TS4]     = "TS4";
          nam[TS5]     = "TS5";
        }

        return nam;
      }
    };

    class TSRadialPartDetail {
    public:
      enum enum_type  { unknown, IN, OUT };
      static std::string const& typeName() {
        static std::string type("TSRadialPart"); return type;
      }
      static std::map<enum_type,std::string> const& names() {
        static std::map<enum_type,std::string> nam;

        if ( nam.empty() ) {
          nam[unknown] = "unknown";
          nam[IN]      = "IN";
          nam[OUT]     = "OUT";
        }
        
        return nam;
      }
    };

    typedef EnumToStringSparse<TSRegionDetail> TSRegion;
    typedef EnumToStringSparse<TSRadialPartDetail> TSRadialPart;

    // Cryo-stat dimensions
    double torusRadius() const { return _rTorus; }
    double innerRadius() const { return _rVac; }

    double endWallU1_rIn() const { return _rIn_endWallU1; }
    double endWallU2_rIn() const { return _rIn_endWallU2; }
    double endWallD_rIn() const  { return _rIn_endWallD;  }

    double endWallU1_rOut() const { return _rOut_endWallU1; }
    double endWallU2_rOut() const { return _rOut_endWallU2; }
    double endWallD_rOut() const  { return _rOut_endWallD;  }

    double endWallU1_halfLength() const { return _halfLength_endWallU1; }
    double endWallU2_halfLength() const { return _halfLength_endWallU2; }
    double endWallD_halfLength() const  { return _halfLength_endWallD;  }

    std::string material()       const { return _material; }
    std::string insideMaterial() const { return _insideMaterial; }

    template <class T = TSSection>
    T* getTSCryo(TSRegion::enum_type i,TSRadialPart::enum_type j) const {
      return static_cast<T*>( _cryoMap.find(i)->second.find(j)->second.get() );        
    }

    // Coils
    std::string coil_material() const { return _coilMaterial; }
    unsigned getNCoils(TSRegion::enum_type i) const { return _nCoils.at(i); }
    const std::vector<Coil>& getTSCoils(TSRegion::enum_type i) const { return _coilMap.at(i); }

    // Collimators
    CollimatorTS1 const& getColl1()  const { return _coll1;  }
    CollimatorTS3 const& getColl31() const { return _coll31; }
    CollimatorTS3 const& getColl32() const { return _coll32; }
    CollimatorTS5 const& getColl5()  const { return _coll5;  }

    // Vacua
    template <class T = TSSection>
    T* getTSVacuum(TSRegion::enum_type i) const {
      return static_cast<T*>(_vacuumMap.find(i)->second.get() );
    }

    // Poly-lining
    const TorusSection* getTSPolyLining( TSRegion::enum_type i) const {
      return _polyLiningMap.find(i)->second.get();
    }

    PbarWindow const& getPbarWindow() const { return _pbarWindow; }

  protected:

    // Cryostat
    double _rTorus;
    double _rVac;

    double _rIn_endWallU1; 
    double _rIn_endWallU2; 
    double _rIn_endWallD;  

    double _rOut_endWallU1; 
    double _rOut_endWallU2; 
    double _rOut_endWallD;  

    double _halfLength_endWallU1; 
    double _halfLength_endWallU2; 
    double _halfLength_endWallD;  

    std::string _material;
    std::string _insideMaterial;

    // - cryostat map
    typedef std::map<TSRadialPart::enum_type,std::unique_ptr<TSSection>> map_unique_ptrs_TSSection;
    std::map<TSRegion::enum_type,map_unique_ptrs_TSSection> _cryoMap;

    // Coils
    std::string _coilMaterial;
    std::map<TSRegion::enum_type,std::vector<Coil>> _coilMap;
    const std::map<TSRegion::enum_type,unsigned> _nCoils = {
      {TSRegion::TS1, 3},
      {TSRegion::TS2,18},
      {TSRegion::TS3, 8},
      {TSRegion::TS4,18},
      {TSRegion::TS5, 5}
    };
    
    // Collimators
    CollimatorTS1 _coll1;
    CollimatorTS3 _coll31;
    CollimatorTS3 _coll32;
    CollimatorTS5 _coll5;

    // Vacuum map
    std::map<TSRegion::enum_type,std::unique_ptr<TSSection>> _vacuumMap;

    // Poly-lining map
    std::map<TSRegion::enum_type,std::unique_ptr<TorusSection>> _polyLiningMap;

    PbarWindow _pbarWindow;   

  };

}
#endif /* BeamlineGeom_TransportSolenoid_hh */
