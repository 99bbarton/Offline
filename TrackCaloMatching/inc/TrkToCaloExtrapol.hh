//
// container for the info of the extrapolated trajectory on the calorimeter
//
// $Id: TrkToCaloExtrapol.hh,v 1.4 2012/08/31 22:34:53 brownd Exp $
// $Author: brownd $
// $Date: 2012/08/31 22:34:53 $
//
// Original author G. Pezzullo
//


#ifndef TrackCaloMatching_TrkToCaloExtrapol_hh
#define TrackCaloMatching_TrkToCaloExtrapol_hh

// Mu2e includes:
#include "art/Persistency/Common/Ptr.h"
#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Matrix/Vector.h"
#include "CLHEP/Vector/TwoVector.h"
#include "CLHEP/Matrix/Matrix.h"
#include "CLHEP/Geometry/HepPoint.h"
#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Matrix/SymMatrix.h"
#include "BaBar/BbrGeom/include/BbrLorentzVectorErr.hh"

//tracker includes
#include "KalmanTrack/KalRep.hh"

// C++ includes
#include <vector>

namespace mu2e {

typedef  art::Ptr< const KalRep * const  >            KalRepPtr;

struct TrkToCaloExtrapol{

private:
        int                                            _vaneId;    //vane index, runs from 0 to nVanes
        KalRepPtr                                     _trk;
        double                             _pathLengthEntrance;
        double                                 _pathLengthExit;



public:

        TrkToCaloExtrapol():_vaneId(-1),
        _pathLengthEntrance(0.0),
        _pathLengthExit(0.0){}



        TrkToCaloExtrapol(int& vane, KalRepPtr& trk, double& entrance, double& exit):
                _vaneId(vane),
                _trk(trk),
                _pathLengthEntrance(entrance),
                _pathLengthExit(exit){
        }
        ~TrkToCaloExtrapol(){}

        //Accessors
        int                                             vaneId() const;
        double                                            time() const;
        double                                         timeErr() const;
        double                              pathLengthEntrance() const;
        double                           pathLenghtEntranceErr() const;
        double                                  pathLengthExit() const;
        double                                              t0() const;
        double                                           t0Err() const;
        Hep3Vector                                  t0Momentum() const;
        BbrVectorErr                             t0MomentumErr() const;
        HepPoint                              entrancePosition() const;
        BbrPointErr                        entrancePositionErr() const;
        HepPoint                                  exitPosition() const;
        BbrPointErr                            exitPositionErr() const;
        Hep3Vector                                    momentum() const;
        BbrVectorErr                               momentumErr() const;
        KalRepPtr const&                               trk() const{ return _trk; }


        void print( std::ostream& ost = std::cout, bool doEndl = true ) const;

        bool           operator == (const TrkToCaloExtrapol & other) const ;

        bool           operator<( const TrkToCaloExtrapol& other) const{
                return ( _pathLengthEntrance< other._pathLengthEntrance);
        }

};

inline std::ostream& operator<<( std::ostream& ost,TrkToCaloExtrapol const& t){
        t.print(ost,false);
        return ost;
}

} // namespace mu2e

#endif /* TrackCaloMatching_TrkToCaloExtrapol_hh */
