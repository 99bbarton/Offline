#ifndef Mu2eG4_nestTorus_hh
#define Mu2eG4_nestTorus_hh
//
// Free function to create and place a new G4Torus inside a logical volume.
//
// $Id: nestTorus.hh,v 1.9 2013/08/16 19:54:34 knoepfel Exp $
// $Author: knoepfel $
// $Date: 2010/03/15
//

#include <array>
#include <string>
#include <vector>

#include "G4Helper/inc/VolumeInfo.hh"

class G4Material;
class G4LogicalVolume;
class G4VPhysicalVolume;
class G4CSGSolid;

#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "G4Colour.hh"


namespace mu2e {

  VolumeInfo nestTorus ( std::string const& name,
                         std::array<double,5> const halfDim,
                         G4Material* material,
                         G4RotationMatrix const* rot,
                         G4ThreeVector const& offset,
                         G4LogicalVolume* parent,
                         int copyNo,
                         bool const isVisible,
                         G4Colour const color,
                         bool const forceSolid,
                         bool const forceAuxEdgeVisible,
                         bool const placePV,
                         bool const doSurfaceCheck
                         );

  // Alternate argument list (and different behavior)
  // using  VolumeInfo object
  VolumeInfo nestTorus ( std::string const& name,
                         std::array<double,5> const halfDim,
                         G4Material* material,
                         G4RotationMatrix const* rot,
                         G4ThreeVector const& offset,
                         const VolumeInfo& parent,
                         int copyNo,
                         bool const isVisible,
                         G4Colour const color,
                         bool const forceSolid,
                         bool const forceAuxEdgeVisible,
                         bool const placePV,
                         bool const doSurfaceCheck
                         );


}

#endif /* Mu2eG4_nestTorus_hh */
