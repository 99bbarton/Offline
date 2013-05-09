//
// Geometry and identifier info about the Calorimeter.
//
// Original author B. Echenard
//

// C++ includes
#include <iostream>
#include <algorithm>

// Mu2e includes
#include "CalorimeterGeom/inc/DiskCalorimeter.hh"
#include "CalorimeterGeom/inc/Disk.hh"

//other includes
#include "CLHEP/Vector/Rotation.h"
#include "CLHEP/Vector/TwoVector.h"


namespace mu2e {



   bool DiskCalorimeter::isInsideDisk(int idisk, CLHEP::Hep3Vector const& pos) const 
   {   
	
	CLHEP::Hep3Vector posInSection = toSectionFrame(idisk, pos);
        
        double zlim    = _crystalHalfLength + _wrapperThickness + _caseThickness + _roHalfThickness + 0.5;         
	double rinlim  = _diskInnerRadius[idisk] - _caseThickness - 0.5;
	double routlim = _diskOuterRadius[idisk]+0.5 + _caseThickness;
	
	if (posInSection.z()< -zlim || posInSection.z() > zlim ) return false;
	if (posInSection.perp() <  rinlim || posInSection.perp()  > routlim ) return false;
	return true;
    }

    bool DiskCalorimeter::isInsideCalorimeter(CLHEP::Hep3Vector const& pos) const 
    {   
        for (unsigned int idisk=0;idisk<_nSections;++idisk) if (isInsideDisk(idisk, pos)) return true;
        return false;    
    }
 

 


    int DiskCalorimeter::crystalIdxFromPosition(CLHEP::Hep3Vector const& pos) const 
    {   
	int offset(0);
	for (unsigned int idisk=0;idisk<_nSections;++idisk) {
 	   if ( isInsideDisk(idisk,pos) ) {
		 CLHEP::Hep3Vector posInSection = toSectionFrame(idisk, pos);
		 return offset + disk(idisk).idxFromPosition(posInSection.x(),posInSection.y());
	   } 
	   offset += disk(idisk).nCrystals();
	}       	  
	return -1;
    }

    std::vector<int> DiskCalorimeter::neighbors(int crystalId, int level)  const
    {

	int iv = caloSectionId(crystalId);
	int ic = localCrystalId(crystalId);

	int offset(0);
	for (int i=0;i<iv;++i) offset += disk(i).nCrystals();

	std::vector<int> list = disk(iv).neighbors(ic,level);
	transform(list.begin(), list.end(), list.begin(),bind2nd(std::plus<int>(), offset));  

	return list;
    }









}
