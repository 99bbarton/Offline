//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// $Id: Shielding_MU2E02.hh,v 1.1 2014/03/04 04:26:06 genser Exp $
//
//---------------------------------------------------------------------------
//
// ClassName:   
//
// Author: 2010  Tatsumi Koi, Gunter Folger
//   created from FTFP_BERT
//
// Modified: KLG to include new muon related code and mu2e::SimpleConfig parameter

//
//----------------------------------------------------------------------------
//
#ifndef TShielding_MU2E02_h
#define TShielding_MU2E02_h 1

#include <CLHEP/Units/SystemOfUnits.h>

#include "globals.hh"
#include "G4VModularPhysicsList.hh"
#include "CompileTimeConstraints.hh"

// Mu2e include
#include "ConfigTools/inc/SimpleConfig.hh"

template<class T>
class TShielding_MU2E02: public T
{
public:
  TShielding_MU2E02(mu2e::SimpleConfig const & config, 
		    G4int verbose = 1 , G4String low_energy_neutron_model = "HP" );
  virtual ~TShielding_MU2E02();
  
public:
  // SetCuts() 
  virtual void SetCuts();

private:
  enum {ok = CompileTimeConstraints::IsA<T, G4VModularPhysicsList>::ok };
};
#include "Shielding_MU2E02.icc"
typedef TShielding_MU2E02<G4VModularPhysicsList> Shielding_MU2E02;

#endif
