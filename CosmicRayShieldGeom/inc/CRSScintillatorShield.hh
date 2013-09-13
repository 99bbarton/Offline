#ifndef CosmicRayShieldGeom_CRSScintillatorShield_hh
#define CosmicRayShieldGeom_CRSScintillatorShield_hh
//
// Representation of one ScintillatorShield in CosmicRayShield.
//
// $Id: CRSScintillatorShield.hh,v 1.6 2013/09/13 06:42:44 ehrlich Exp $
// $Author: ehrlich $
// $Date: 2013/09/13 06:42:44 $
//
// Original author KLG based on Rob Kutschke's Device
//

// C++ includes
#include <vector>

// Mu2e includes
#include "CosmicRayShieldGeom/inc/CRSScintillatorShieldId.hh"
#include "CosmicRayShieldGeom/inc/CRSScintillatorModule.hh"

// CLHEP includes
#include "CLHEP/Vector/ThreeVector.h"

namespace mu2e 
{

  class CRSScintillatorShield
  {

    friend class CosmicRayShieldMaker;

    public:

    CRSScintillatorShield():_id(-1){}

    CRSScintillatorShield(CRSScintillatorShieldId const & id, std::string const & name);

    // Accept the compiler generated destructor, copy constructor and assignment operators

    // Accessors
    const CRSScintillatorShieldId id() const { return _id;}

    const std::string& getName() {return _name;}

    int nModules() const {return _modules.size();}

    const std::vector<CRSScintillatorModule>& getCRSScintillatorModules() const
    {
      return _modules;
    }

    const CRSScintillatorModule& getModule( int n) const 
    {
      return _modules.at(n);
    }

    const CRSScintillatorModule& getModule( const CRSScintillatorModuleId& moduleid ) const
    {
      return _modules.at(moduleid.getModuleNumber());
    }

    const CRSScintillatorLayer& getLayer( const CRSScintillatorLayerId& lid ) const
    {
      return _modules.at(lid.getModuleNumber()).getLayer(lid);
    }

    CRSScintillatorBar const & getBar( const CRSScintillatorBarId& bid ) const
    {
      return _modules.at(bid.getModuleNumber()).getBar(bid);
    }

    CRSScintillatorBarDetail const & getCRSScintillatorBarDetail() const 
    {
      return _barDetails;
    }

    // Formatted string embedding the id of the shield.
    std::string name( std::string const & base ) const;

    // On readback from persistency, recursively recompute mutable members.
    //    void fillPointers( const CosmicRayShield& cosmicRayShield ) const;

    private:

    CRSScintillatorShieldId _id;

    std::string _name;

    std::vector<CRSScintillatorModule> _modules;

    // Detailed info about scintillator bars assuming they are the same for all bars within a shield
    CRSScintillatorBarDetail _barDetails;
  };

} //namespace mu2e

#endif /* CosmicRayShieldGeom_CRSScintillatorShield_hh */
