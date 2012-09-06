#ifndef RecoDataProducts_CaloCluster_hh
#define RecoDataProducts_CaloCluster_hh
//
// Calorimeter's data cluster container
//
// $Id: CaloCluster.hh,v 1.5 2012/09/06 19:58:26 kutschke Exp $
// $Author: kutschke $
// $Date: 2012/09/06 19:58:26 $
//
// Original author G. Pezzullo & G. Tassielli
//

// Mu2e includes:
#include "art/Persistency/Common/Ptr.h"
#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Units/PhysicalConstants.h"

// C++ includes
#include <vector>
#include "RecoDataProducts/inc/CaloCrystalHitCollection.hh"
#include "RecoDataProducts/inc/CaloCrystalHit.hh"

namespace mu2e {

  typedef art::Ptr< CaloCrystalHit>                CaloCrystalHitPtr;
  typedef std::vector<CaloCrystalHitPtr>     CaloCrystalHitPtrVector;

  struct CaloCluster{

  private:
    int                                       _vaneId; // number of vane
    float                                       _time; // (ns)
    int                                       _cogRow; // running from 0, 1, ... (nCrystalR - 1)
    int                                    _cogColumn; // running from 0, 1, ... (nCrystalZ - 1)
    float                                  _energyDep; // (MeV)
    CLHEP::Hep3Vector                     _cog3Vector; // center of gravity of the cluster in the localVaneFrame
    CLHEP::Hep3Vector                _cog3VectorError;
    CaloCrystalHitPtrVector _caloCrystalHitsPtrVector;

  public:
    CaloCluster():
      _vaneId(0),
      _time(0.),
      _cogRow(0),
      _cogColumn(0),
      _energyDep(0.){
    }

    CaloCluster(int iVane):
      _vaneId(iVane),
      _time(0.),
      _cogRow(0),
      _cogColumn(0),
      _energyDep(0.){
    }

    CaloCluster(int                               iVane,
                float                              time,
                float                            energy,
                CaloCrystalHitPtrVector caloCrystalHits ):
      _vaneId(iVane),
      _time(time),
      _energyDep(energy),
      _caloCrystalHitsPtrVector(caloCrystalHits){
    }

    void print( std::ostream& ost = std::cout, bool doEndl = true ) const;

    void AddHit (CaloCrystalHitPtr &a);

    //Accessors
    int                                              vaneId() const{return _vaneId;}          // index of the vane
    float                                              time() const{return _time;}            // (ns)
    int                                              cogRow() const{return _cogRow;}          // running from 0, 1, ... (nCrystalR - 1)
    int                                           cogColumn() const{return _cogColumn;}       // running from 0, 1, ... (nCrystalZ - 1)
    float                                         energyDep() const{return _energyDep;}       // (MeV)
    CLHEP::Hep3Vector const&                     cog3Vector() const{return _cog3Vector;}      // Which coordinate system?
    CLHEP::Hep3Vector const&                cog3VectorError() const{return _cog3VectorError;} // FIXME:  Should be 3x3 matrix.
    CaloCrystalHitPtrVector const& caloCrystalHitsPtrVector() const{return _caloCrystalHitsPtrVector;}
    size_t                                             size() const{return _caloCrystalHitsPtrVector.size();}

    //Setting parameters
    void SetVaneId (int vane) {
      _vaneId = vane;
    }

    void SetTime (float  time) {
      _time = time;
    }

    void SetCogRow ( int row) {
      _cogRow = row;
    }

    void SetCogColumn ( int column) {
      _cogColumn = column;
    }

    void SetEnergyDep ( float  energyDep) {
      _energyDep = energyDep;
    }

    void SetCog3Vector ( CLHEP::Hep3Vector cog3Vector) {
      _cog3Vector = cog3Vector;
    }

    void SetCog3VectorError ( CLHEP::Hep3Vector cog3VectorErr) {
      _cog3VectorError = cog3VectorErr;
    }

  };

} // namespace mu2e

#endif /* RecoDataProducts_CaloCluster_hh */
