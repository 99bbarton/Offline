//
// Construct materials requested by the run-time configuration system.
//
// $Id: ConstructMaterials.cc,v 1.47 2014/01/13 22:36:39 knoepfel Exp $
// $Author: knoepfel $
// $Date: 2014/01/13 22:36:39 $
//
// Original author Rob Kutschke
//
// Notes:
// 1) This code new's many objects of type G4Material.  The lifeime
//    of these objects is controlled within G4.  We must never
//    delete them.
//

// C++ includes
#include <iostream>
#include <iomanip>

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// Mu2e includes
#include "Mu2eG4/inc/ConstructMaterials.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "ConditionsService/inc/GlobalConstantsHandle.hh"
#include "ConditionsService/inc/PhysicsParams.hh"

// CLHEP includes
#include "CLHEP/Units/PhysicalConstants.h"

// G4 includes
#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"

#include "G4Material.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"
#include "G4NistManager.hh"
#include "G4VisAttributes.hh"

using namespace std;

namespace mu2e {

  // Value used to request that all Mu2e specific materials be made.
  static const std::string DoAllValue = "DoAll";

  ConstructMaterials::ConstructMaterials(){
  }

  ConstructMaterials::~ConstructMaterials(){
  }

  // This is the main function.
  void ConstructMaterials::construct(){

    // Get access to the master geometry system and its run time config.
    art::ServiceHandle<GeometryService> geom;
    SimpleConfig const& config = geom->config();

    // Construct the requested materials.
    constructMu2eMaterials( config );

    // Print element table, if requested.
    if ( config.getBool("g4.printElements",false) ){
      mf::LogInfo  log("GEOM");
      log << *G4Element::GetElementTable();
    }

    // Print material table, if requested.
    if ( config.getBool("g4.printMaterials",false) ){
      mf::LogInfo  log("GEOM");
      log << *G4Material::GetMaterialTable();
    }

  }

  // Build the requested Mu2e specific materials.
  // Notes:
  // 1) In the methods: G4Material::AddElement( G4Element* elem, ... )
  //    Each element object keeps track of how many time it is used in a material.
  //    Therefore the first argument cannot be a const pointer.
  void ConstructMaterials::constructMu2eMaterials(SimpleConfig const& config){

    // List of requested Mu2e specific materials from the config file.
    vector<string> materialsToLoad;
    config.getVectorString("mu2e.materials",materialsToLoad);

    CheckedG4String mat = isNeeded(materialsToLoad, "CONCRETE_MARS" );
    if ( mat.doit ) {
      G4Material* ConcreteMars = new G4Material(mat.name, 2.35*CLHEP::g/CLHEP::cm3, 9 );
      ConcreteMars->AddElement( getElementOrThrow("H") , 0.006); //Hydrogen
      ConcreteMars->AddElement( getElementOrThrow("C") , 0.030); //Carbon
      ConcreteMars->AddElement( getElementOrThrow("O") , 0.500); //Oxygen
      ConcreteMars->AddElement( getElementOrThrow("Na"), 0.010); //Sodium
      ConcreteMars->AddElement( getElementOrThrow("Al"), 0.030); //Aluminum
      ConcreteMars->AddElement( getElementOrThrow("Si"), 0.200); //Silicon
      ConcreteMars->AddElement( getElementOrThrow("K") , 0.010); //Potassium
      ConcreteMars->AddElement( getElementOrThrow("Ca"), 0.200); //Calcium
      ConcreteMars->AddElement( getElementOrThrow("Fe"), 0.014); //Iron
    }

    mat = isNeeded(materialsToLoad, "BARITE" );
    if ( mat.doit ) {
      G4Material* Barite = new G4Material(mat.name, 3.5*CLHEP::g/CLHEP::cm3, 9 );
      Barite->AddElement( getElementOrThrow("H") , 0.0069); //Hydrogen
      Barite->AddElement( getElementOrThrow("O") , 0.3386); //Oxygen
      Barite->AddElement( getElementOrThrow("Mg"), 0.0011); //Magnesium
      Barite->AddElement( getElementOrThrow("Al"), 0.0039); //Aluminum
      Barite->AddElement( getElementOrThrow("Si"), 0.0100); //Silicon
      Barite->AddElement( getElementOrThrow("S") , 0.1040); //Sulfur
      Barite->AddElement( getElementOrThrow("Ca"), 0.0478); //Calcium
      Barite->AddElement( getElementOrThrow("Fe"), 0.0457); //Iron
      Barite->AddElement( getElementOrThrow("Ba"), 0.4420); //Barium - should be 0.4431, but
                                                            //         G4 complains if fractional
                                                            //         weights don't add up to 1.
    }
 

    mat = isNeeded(materialsToLoad, "HeavyConcrete");
    if ( mat.doit ) {
      G4Material* HeavyConcrete = new G4Material(mat.name, 3.295*CLHEP::g/CLHEP::cm3, 17);
      HeavyConcrete->AddElement( getElementOrThrow("H") , 0.01048482); //Hydrogen
      HeavyConcrete->AddElement( getElementOrThrow("B") , 0.00943758); //Boron
      HeavyConcrete->AddElement( getElementOrThrow("C") , 0.0129742);  //Carbon
      HeavyConcrete->AddElement( getElementOrThrow("O") , 0.27953541); //Oxygen
      HeavyConcrete->AddElement( getElementOrThrow("F") , 1.5175E-4);  //Fluorine
      HeavyConcrete->AddElement( getElementOrThrow("Na"), 3.7014E-4);  //Sodium
      HeavyConcrete->AddElement( getElementOrThrow("Mg"), 0.08298213); //Magnesium
      HeavyConcrete->AddElement( getElementOrThrow("Al"), 0.02769028); //Aluminum
      HeavyConcrete->AddElement( getElementOrThrow("Si"), 0.06317253); //Silicon
      HeavyConcrete->AddElement( getElementOrThrow("P") , 0.00176963); //Phosphorus
      HeavyConcrete->AddElement( getElementOrThrow("S") , 5.8275E-4);  //Sulfur
      HeavyConcrete->AddElement( getElementOrThrow("K") , 4.2024E-4);  //Potassium
      HeavyConcrete->AddElement( getElementOrThrow("Ca"), 0.03227609); //Calcium
      HeavyConcrete->AddElement( getElementOrThrow("Ti"), 5.457E-5);   //Titanium
      HeavyConcrete->AddElement( getElementOrThrow("Mn"), 0.00321757); //Manganese
      HeavyConcrete->AddElement( getElementOrThrow("Fe"), 0.47423935); //Iron
      HeavyConcrete->AddElement( getElementOrThrow("Sr"), 6.4097E-4);  //Strontium
    }

    mat = isNeeded(materialsToLoad, "ShieldingConcrete");
    if ( mat.doit ) {
      //
      // Concrete is 2.00 for fraction, but shielding concrete has reinforcing Iron bars:
      // www-esh.fnal.gov/TM1834_PDF_FILES/TM_1834_Revision_9.pdf
      //
      G4Material* ShieldingConcrete = new G4Material( mat.name, 2.5*CLHEP::g/CLHEP::cm3, 6);
      ShieldingConcrete->AddElement( getElementOrThrow("O") ,  0.5200);
      ShieldingConcrete->AddElement( getElementOrThrow("Si"),  0.3250);
      ShieldingConcrete->AddElement( getElementOrThrow("Ca"),  0.0600);
      ShieldingConcrete->AddElement( getElementOrThrow("Na"),  0.0150);
      ShieldingConcrete->AddElement( getElementOrThrow("Fe"),  0.0400);
      ShieldingConcrete->AddElement( getElementOrThrow("Al"),  0.0400);
    }

    mat = isNeeded(materialsToLoad, "Polyethylene");
    if ( mat.doit ){
      G4Material* Polyethylene = new G4Material( mat.name, 0.956*CLHEP::g/CLHEP::cm3, 2);
      Polyethylene->AddElement( getElementOrThrow("C"), 1);
      Polyethylene->AddElement( getElementOrThrow("H"), 2);
    }

    mat = isNeeded(materialsToLoad, "Half_Poly" );
    if ( mat.doit ) {
      G4Material* HalfPoly = new G4Material( mat.name, 0.465*CLHEP::g/CLHEP::cm3, 1);
      HalfPoly->AddMaterial( findMaterialOrThrow("Polyethylene"), 1. );
    }

    // polyethylene data below as in John Caunt Scientific, also see shieldwerx
    mat = isNeeded(materialsToLoad, "Polyethylene092");
    if ( mat.doit ){
      G4Material* Polyethylene092 = new G4Material( mat.name, 0.92*CLHEP::g/CLHEP::cm3, 2);
      Polyethylene092->AddMaterial( findMaterialOrThrow("G4_H"), 0.1428);
      Polyethylene092->AddMaterial( findMaterialOrThrow("G4_C"), 0.8572);
    }

    mat = isNeeded(materialsToLoad, "Polyethylene0956");
    if ( mat.doit ){
      G4Material* Polyethylene0956 = new G4Material( mat.name, 0.956*CLHEP::g/CLHEP::cm3, 2);
      Polyethylene0956->AddMaterial( findMaterialOrThrow("G4_H"), 0.143711);
      Polyethylene0956->AddMaterial( findMaterialOrThrow("G4_C"), 0.856289);
    }

    mat = isNeeded(materialsToLoad, "Polyethylene096");
    if ( mat.doit ){
      G4Material* Polyethylene096 = new G4Material( mat.name, 0.96*CLHEP::g/CLHEP::cm3, 2);
      Polyethylene096->AddMaterial( findMaterialOrThrow("G4_H"), 0.14);
      Polyethylene096->AddMaterial( findMaterialOrThrow("G4_C"), 0.86);
    }

    // Not real, very thin Polyethylene
    mat = isNeeded(materialsToLoad, "Polyethylene0010");
    if ( mat.doit ){
      G4Material* Polyethylene0010 = new G4Material( mat.name, 0.010*CLHEP::g/CLHEP::cm3, 2);
      Polyethylene0010->AddMaterial( findMaterialOrThrow("G4_H"), 0.143711);
      Polyethylene0010->AddMaterial( findMaterialOrThrow("G4_C"), 0.856289);
    }

    // Not real, very thin Polyethylene
    mat = isNeeded(materialsToLoad, "Polyethylene0020");
    if ( mat.doit ){
      G4Material* Polyethylene0020 = new G4Material( mat.name, 0.020*CLHEP::g/CLHEP::cm3, 2);
      Polyethylene0020->AddMaterial( findMaterialOrThrow("G4_H"), 0.143711);
      Polyethylene0020->AddMaterial( findMaterialOrThrow("G4_C"), 0.856289);
    }

    //   note that G4 has:
    //   AddMaterial("G4_POLYETHYLENE", 0.94, 0, 57.4, 2);
    //   AddElementByWeightFraction( 1, 0.143711);
    //   AddElementByWeightFraction( 6, 0.856289);
    //   AddChemicalFormula("G4_POLYETHYLENE","(C_2H_4)_N-Polyethylene");

    // borated polyethylene data as in John Caunt Scientific
    mat = isNeeded(materialsToLoad, "Polyethylene092B050d095");
    if ( mat.doit ){
      G4Material* Polyethylene092B050d095 = new G4Material( mat.name, 0.95*CLHEP::g/CLHEP::cm3, 2);
      // we will use the Polyethylene092 and add B as a material
      const double BPercentage = 5.0;
      Polyethylene092B050d095->AddMaterial(findMaterialOrThrow("Polyethylene092"), (100.-BPercentage)*CLHEP::perCent);
      Polyethylene092B050d095->AddMaterial(findMaterialOrThrow("G4_B")           , BPercentage*CLHEP::perCent);
    }

    mat = isNeeded(materialsToLoad, "Polyethylene092B300d119");
    if ( mat.doit ){
      G4Material* Polyethylene092B300d119 = new G4Material( mat.name, 1.19*CLHEP::g/CLHEP::cm3, 2);
      // we will use the Polyethylene092 and add B as a material
      const double BPercentage = 30.0;
      Polyethylene092B300d119->AddMaterial(findMaterialOrThrow("Polyethylene092"), (100.-BPercentage)*CLHEP::perCent);
      Polyethylene092B300d119->AddMaterial(findMaterialOrThrow("G4_B")           , BPercentage*CLHEP::perCent);
    }

    mat = isNeeded(materialsToLoad, "Polyethylene092Li075d106");
    if ( mat.doit ){
      G4Material* Polyethylene092Li075d106 =
        new G4Material( mat.name, 1.06*CLHEP::g/CLHEP::cm3, 2);
      // we will use the Polyethylene092 and add Li as a material
      const double LiPercentage = 7.5;
      Polyethylene092Li075d106->AddMaterial(findMaterialOrThrow("Polyethylene092"), (100.-LiPercentage)*CLHEP::perCent);
      Polyethylene092Li075d106->AddMaterial(findMaterialOrThrow("G4_Li")          , LiPercentage*CLHEP::perCent);
    } 

    // Stainless Steel (Medical Physics, Vol 25, No 10, Oct 1998) based on brachytherapy example
    // FIXME is there a better reference?
    mat = isNeeded(materialsToLoad, "StainlessSteel");
    if ( mat.doit ){
      G4Material* StainlessSteel = new G4Material( mat.name, 8.02*CLHEP::g/CLHEP::cm3, 5);
      StainlessSteel->AddMaterial(findMaterialOrThrow("G4_Mn"), 0.02);
      StainlessSteel->AddMaterial(findMaterialOrThrow("G4_Si"), 0.01);
      StainlessSteel->AddMaterial(findMaterialOrThrow("G4_Cr"), 0.19);
      StainlessSteel->AddMaterial(findMaterialOrThrow("G4_Ni"), 0.10);
      StainlessSteel->AddMaterial(findMaterialOrThrow("G4_Fe"), 0.68);
    }

    // Construction Aluminum
    //http://asm.matweb.com/search/SpecificMaterial.asp?bassnum=MA5083O
    //http://ppd-docdb.fnal.gov/cgi-bin/RetrieveFile?docid=1112;filename=MD-ENG-109.pdf;version=1
    mat = isNeeded(materialsToLoad, "A95083");
    if ( mat.doit ){
      G4Material* A95083 = new G4Material( mat.name, 2.66*CLHEP::g/CLHEP::cm3, 9);
      A95083->AddMaterial(findMaterialOrThrow("G4_Al"), 0.9400);
      A95083->AddMaterial(findMaterialOrThrow("G4_Mg"), 0.0450);
      A95083->AddMaterial(findMaterialOrThrow("G4_Mn"), 0.0070);
      A95083->AddMaterial(findMaterialOrThrow("G4_Fe"), 0.0020);
      A95083->AddMaterial(findMaterialOrThrow("G4_Si"), 0.0020);
      A95083->AddMaterial(findMaterialOrThrow("G4_Cr"), 0.0015);
      A95083->AddMaterial(findMaterialOrThrow("G4_Zn"), 0.0013);
      A95083->AddMaterial(findMaterialOrThrow("G4_Ti"), 0.0007);
      A95083->AddMaterial(findMaterialOrThrow("G4_Cu"), 0.0005);
    }


    // NbTi
    mat = isNeeded(materialsToLoad, "NbTi"); // FIXME verify it
    if ( mat.doit ){
      G4Material* NbTi = new G4Material( mat.name, 6.5*CLHEP::g/CLHEP::cm3, 2);
      NbTi->AddMaterial(findMaterialOrThrow("G4_Nb"), 0.65);
      NbTi->AddMaterial(findMaterialOrThrow("G4_Ti"), 0.35);
    }

    // NbTiCu
    mat = isNeeded(materialsToLoad, "NbTiCu"); // FIXME verify it
    if ( mat.doit ){
      G4Material* NbTiCu = new G4Material( mat.name, 7.69*CLHEP::g/CLHEP::cm3, 2);
      NbTiCu->AddMaterial(findMaterialOrThrow("NbTi"),  0.45);
      NbTiCu->AddMaterial(findMaterialOrThrow("G4_Cu"), 0.55);
    }

    // AL999Ni001 by volume ?
    mat = isNeeded(materialsToLoad, "AL999Ni001"); // FIXME verify it
    if ( mat.doit ){
      G4Material* AL999Ni001 = new G4Material( mat.name, 2.706*CLHEP::g/CLHEP::cm3, 3);
      AL999Ni001->AddMaterial(findMaterialOrThrow("G4_Al"), 0.9967);
      AL999Ni001->AddMaterial(findMaterialOrThrow("G4_Ni"), 0.0033);
    }

    // http://personalpages.to.infn.it/~tosello/EngMeet/ITSmat/SDD/Epotek-301-1.html
    // C_19_H_20_O_4

    mat = isNeeded(materialsToLoad, "C_19_H_20_O_4");
    if ( mat.doit ){
      G4Material* C_19_H_20_O_4 = new G4Material( mat.name, 1.16*CLHEP::g/CLHEP::cm3, 3);
      C_19_H_20_O_4->AddElement( getElementOrThrow("C"), 19);
      C_19_H_20_O_4->AddElement( getElementOrThrow("H"), 20);
      C_19_H_20_O_4->AddElement( getElementOrThrow("O"),  4);
    }

    // C_10_H_18_O_4

    mat = isNeeded(materialsToLoad, "C_10_H_18_O_4");
    if ( mat.doit ){
      G4Material* C_10_H_18_O_4 = new G4Material( mat.name, 1.10*CLHEP::g/CLHEP::cm3, 3);
      C_10_H_18_O_4->AddElement( getElementOrThrow("C"), 10);
      C_10_H_18_O_4->AddElement( getElementOrThrow("H"), 18);
      C_10_H_18_O_4->AddElement( getElementOrThrow("O"),  4);

    }

    // C_9_H_22_N_2

    mat = isNeeded(materialsToLoad, "C_9_H_22_N_2");
    if ( mat.doit ){
      G4Material* C_9_H_22_N_2 = new G4Material( mat.name, 0.865*CLHEP::g/CLHEP::cm3, 3);
      C_9_H_22_N_2->AddElement( getElementOrThrow("C"),  9);
      C_9_H_22_N_2->AddElement( getElementOrThrow("H"), 22);
      C_9_H_22_N_2->AddElement( getElementOrThrow("N"),  2);

    }

    // http://personalpages.to.infn.it/~tosello/EngMeet/ITSmat/SDD/Epotek-301-1.html
    mat = isNeeded(materialsToLoad, "Epotek301");
    if ( mat.doit ){
      G4Material* Epotek301 = new G4Material( mat.name, 1.19*CLHEP::g/CLHEP::cm3, 3);
      Epotek301->AddMaterial(findMaterialOrThrow("C_19_H_20_O_4"), 0.56);
      Epotek301->AddMaterial(findMaterialOrThrow("C_10_H_18_O_4"), 0.24);
      Epotek301->AddMaterial(findMaterialOrThrow("C_9_H_22_N_2"),  0.20);
    }

   // http://personalpages.to.infn.it/~tosello/EngMeet/ITSmat/SDD/E_glass.html
    mat = isNeeded(materialsToLoad, "EGlass");
    if ( mat.doit ){
      G4Material* EGlass = new G4Material (mat.name, 2.61*CLHEP::g/CLHEP::cm3, 10);
      EGlass->AddMaterial(findMaterialOrThrow("G4_SILICON_DIOXIDE"), 0.54);
      EGlass->AddMaterial(findMaterialOrThrow("G4_CALCIUM_OXIDE"), 0.19 );
      EGlass->AddMaterial(findMaterialOrThrow("G4_ALUMINUM_OXIDE"), 0.13 );
      EGlass->AddMaterial(findMaterialOrThrow("G4_MAGNESIUM_OXIDE"), 0.025 );
      EGlass->AddMaterial(findMaterialOrThrow("G4_BORON_OXIDE"), 0.075 );
      EGlass->AddMaterial(findMaterialOrThrow("G4_TITANIUM_DIOXIDE"), 0.008 );
      EGlass->AddMaterial(findMaterialOrThrow("G4_SODIUM_MONOXIDE"), 0.01 );
      EGlass->AddMaterial(findMaterialOrThrow("G4_POTASSIUM_OXIDE"), 0.01 );
      EGlass->AddMaterial(findMaterialOrThrow("G4_FERRIC_OXIDE"), 0.005 );
      EGlass->AddMaterial(findMaterialOrThrow("G4_F"), 0.007 );
    }

    // G10 http://personalpages.to.infn.it/~tosello/EngMeet/ITSmat/SDD/SDD_G10FR4.html
    // http://pdg.lbl.gov/2002/atomicrpp.pdf
    mat = isNeeded(materialsToLoad, "G10");
    if ( mat.doit ){
      G4Material* G10 = new G4Material( mat.name, 1.7*CLHEP::g/CLHEP::cm3, 2);
      G10->AddMaterial(findMaterialOrThrow("G4_SILICON_DIOXIDE"), 0.60);//FIXME do e-glass etc...
      G10->AddMaterial(findMaterialOrThrow("Epotek301"), 0.40);
    }

    // Superconducting Cable Insulation
    mat = isNeeded(materialsToLoad, "SCCableInsulation");
    if ( mat.doit ){
      G4Material* SCCableInsulation = new G4Material( mat.name, 1.54*CLHEP::g/CLHEP::cm3, 3);
      SCCableInsulation->AddMaterial(findMaterialOrThrow("G4_KAPTON"), 0.18);
      SCCableInsulation->AddMaterial(findMaterialOrThrow("Epotek301"), 0.16);
      SCCableInsulation->AddMaterial(findMaterialOrThrow("G10"), 0.66);
    }

    //http://sector7.xor.aps.anl.gov/~dufresne/UofM/techinfo/kapton.html
    //The chemical formula of Kapton is C22H10N205,  its density is 1.43
    // also see below, do we need more than one?

    // Superconducting Cable
    mat = isNeeded(materialsToLoad, "SCCable"); // FIXME verify it
    if ( mat.doit ){
      G4Material* SCCable = new G4Material( mat.name, 3.95*CLHEP::g/CLHEP::cm3, 3);
      SCCable->AddMaterial(findMaterialOrThrow("SCCableInsulation"), 0.04);
      SCCable->AddMaterial(findMaterialOrThrow("AL999Ni001"),        0.43);
      SCCable->AddMaterial(findMaterialOrThrow("NbTiCu"),            0.53);
    }

    mat = isNeeded(materialsToLoad, "IsoButane");
    if ( mat.doit ){
      G4Material* IsoButane = new G4Material( mat.name, 0.00265*CLHEP::g/CLHEP::cm3, 2);
      IsoButane->AddElement( getElementOrThrow("C"), 4);
      IsoButane->AddElement( getElementOrThrow("H"), 10);
    }

    mat = isNeeded(materialsToLoad, "StrawGasArCF4");

    if ( mat.doit ) {
      G4Material* StrawGasArCF4 = new G4Material(mat.name, 0.0028561*CLHEP::g/CLHEP::cm3, 3); // it is OK not to use kStateGas
      StrawGasArCF4->AddElement( getElementOrThrow("Ar"), 1);
      StrawGasArCF4->AddElement( getElementOrThrow("C") , 1);
      StrawGasArCF4->AddElement( getElementOrThrow("F") , 4);
    }
    
    mat = isNeeded(materialsToLoad, "StrawGas");
    if ( mat.doit ) {
     
      G4double density;
      G4double temperature = 293.15*CLHEP::kelvin;
      G4double pressure = 1.*CLHEP::atmosphere;
      G4int nel;

      G4double densityAr   = 0.00166 *CLHEP::g/CLHEP::cm3; //from PDG
      G4double densityCO2  = 0.00184 *CLHEP::g/CLHEP::cm3; //from PDG
      G4double fractionAr  = 80.0*CLHEP::perCent;

      density = fractionAr*densityAr + (1.0-fractionAr)*densityCO2;

      G4Material *GasMix = new G4Material( mat.name, density, nel=3,
                                           kStateGas, temperature, pressure);

      G4Element* Ar = getElementOrThrow("Ar");
      G4Element* C  = getElementOrThrow("C");
      G4Element* O  = getElementOrThrow("O");

      G4double atomicWeight_Ar =  39.948 *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_C  = 12.0107 *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_O  = 15.9994 *CLHEP::g/CLHEP::mole;
      G4double pwAr = fractionAr*atomicWeight_Ar;
      G4double pwC  = (1.0-fractionAr) *  1.0*atomicWeight_C;
      G4double pwO  = (1.0-fractionAr) *  2.0*atomicWeight_O;
      G4double atomicWeightMix = pwAr + pwC + pwO ;

      pwAr/=atomicWeightMix;
      pwO/=atomicWeightMix;
      GasMix->AddElement(Ar, pwAr );
      GasMix->AddElement(O , pwO  );
      GasMix->AddElement(C , 1.0-pwAr-pwO  );

    }

    mat = isNeeded(materialsToLoad, "Kapton");
    if ( mat.doit ){
      //
      // Kapton: from NIST: physics.nist.gov/cgi-bin/Star/compos.pl?matno=179
      //
      G4Material* Kapton = new G4Material(mat.name, 1.42*CLHEP::g/CLHEP::cm3, 4);
      Kapton->AddElement( getElementOrThrow("H"), 0.026362);
      Kapton->AddElement( getElementOrThrow("C"), 0.691133);
      Kapton->AddElement( getElementOrThrow("N"), 0.073270);
      Kapton->AddElement( getElementOrThrow("O"), 0.209235);
    }

    mat = isNeeded(materialsToLoad, "Scintillator");
    if ( mat.doit ){
      //
      // Scintillator.
      // We probably want several flavors of scintillator so that we can change the
      // detector by just changing a name in the config file.
      //
      G4Material* Sci = new G4Material( mat.name, 1.032*CLHEP::g/CLHEP::cm3, 2);
      Sci->AddElement( getElementOrThrow("C"), 9);
      Sci->AddElement( getElementOrThrow("H"), 10);
    }

    mat = isNeeded(materialsToLoad, "WAGVacuum");
    if ( mat.doit ){
      //
      // This is the lowest density vacuum allowed by G4.
      G4double density     = CLHEP::universe_mean_density;
      G4double pressure    = 3.e-18*CLHEP::pascal;
      G4double temperature = 2.73*CLHEP::kelvin;

      // G4 takes ownership of this object and manages its lifetime.
      new G4Material( mat.name, 1., 1.01 *CLHEP::g/CLHEP::mole,
                      density, kStateGas, temperature, pressure);
    }


    // Presume that the residual gas in the DS will be leakage from the straws,
    // pumped down to 10^{-4} torr.
    mat = isNeeded(materialsToLoad, "DSVacuum");
    if ( mat.doit ){

      G4Material* StrawLeak = findMaterialOrThrow("StrawGas");

      G4double temperature = 300.00*CLHEP::kelvin; // Temperature of the DS
      G4double pressure    = 133.322e-4*CLHEP::pascal; // 10e-4 Torr
      G4double refTemp     = StrawLeak->GetTemperature();
      G4double refPress    = StrawLeak->GetPressure();

      G4double density = StrawLeak->GetDensity()*pressure*refTemp/(refPress*temperature);

      G4Material* DSVacuum =
	new G4Material(mat.name, density, StrawLeak->GetNumberOfElements(),
		       kStateGas, temperature, pressure);

      for (size_t i = 0 ; i < StrawLeak->GetNumberOfElements(); ++i) {
	DSVacuum->AddElement(StrawLeak->GetElementVector()->at(i), StrawLeak->GetFractionVector()[i]);
      }

    }


    mat = isNeeded(materialsToLoad, "MBOverburden");
    if ( mat.doit ){
      //
      // MiniBoone model of the earthen overburden.  See Mu2e-doc-570.
      //
      G4Material* mbOverburden = new G4Material( mat.name, 2.15*CLHEP::g/CLHEP::cm3, 3);
      G4Element* eO  = getElementOrThrow("O");
      G4Element* eSi = getElementOrThrow("Si");
      G4Element* eAl = getElementOrThrow("Al");
      mbOverburden->AddElement( eO,  65);
      mbOverburden->AddElement( eSi, 20);
      mbOverburden->AddElement( eAl, 15);
    }

    mat = isNeeded(materialsToLoad, "ITGasHe_95Isob_5");
    if ( mat.doit ){

      G4double density, temperature, pressure;
      G4int nel;

      G4double densityHe   = 0.000166 *CLHEP::g/CLHEP::cm3;
      G4double densityIsoB = 0.00249  *CLHEP::g/CLHEP::cm3;
      G4double fractionHe  = 95.0*CLHEP::perCent;

      density = fractionHe*densityHe + (1.0-fractionHe)*densityIsoB;

      G4Material *GasMix = new G4Material( mat.name, density, nel=3,
                                           kStateGas, temperature= 293.15*CLHEP::kelvin, 
					   pressure= 1.*CLHEP::atmosphere);

      G4Element* He = getElementOrThrow("He");
      G4Element* C  = getElementOrThrow("C");
      G4Element* H  = getElementOrThrow("H");

      G4double atomicWeight_He =  4.002602 *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_C  = 12.0107   *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_H  =  1.00794  *CLHEP::g/CLHEP::mole;
      G4double pwHe = fractionHe*atomicWeight_He;
      G4double pwC  = (1.0-fractionHe) *  4.0*atomicWeight_C;
      G4double pwH  = (1.0-fractionHe) * 10.0*atomicWeight_H;
      G4double atomicWeightMix = pwHe + pwC + pwH ;

      pwHe/=atomicWeightMix;
      pwH/=atomicWeightMix;
      GasMix->AddElement(He, pwHe );
      GasMix->AddElement(H , pwH  );
      GasMix->AddElement(C , 1.0-pwHe-pwH  );
    }

    mat = isNeeded(materialsToLoad, "ITGasHe_90Isob_10");
    if ( mat.doit ){

      G4double density, temperature, pressure;
      G4int nel;

      G4double densityHe   = 0.000166 *CLHEP::g/CLHEP::cm3;
      G4double densityIsoB = 0.00249  *CLHEP::g/CLHEP::cm3;
      G4double fractionHe  = 90.0*CLHEP::perCent;

      density = fractionHe*densityHe + (1.0-fractionHe)*densityIsoB;

      G4Material *GasMix = new G4Material( mat.name, density, nel=3,
                                           kStateGas, temperature= 293.15*CLHEP::kelvin, 
					   pressure= 1.*CLHEP::atmosphere);

      G4Element* He = getElementOrThrow("He");
      G4Element* C  = getElementOrThrow("C");
      G4Element* H  = getElementOrThrow("H");

      G4double atomicWeight_He =  4.002602 *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_C  = 12.0107   *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_H  =  1.00794  *CLHEP::g/CLHEP::mole;
      G4double pwHe = fractionHe*atomicWeight_He;
      G4double pwC  = (1.0-fractionHe) *  4.0*atomicWeight_C;
      G4double pwH  = (1.0-fractionHe) * 10.0*atomicWeight_H;
      G4double atomicWeightMix = pwHe + pwC + pwH ;

      pwHe/=atomicWeightMix;
      pwH/=atomicWeightMix;
      GasMix->AddElement(He, pwHe );
      GasMix->AddElement(H , pwH  );
      GasMix->AddElement(C , 1.0-pwHe-pwH  );
    }

    mat = isNeeded(materialsToLoad, "ITGasHe_75Isob_25_400mbar");
    if ( mat.doit ){

      G4double density, temperature, pressure;
      G4int nel;

      G4double densityHe   = 0.000166 *CLHEP::g/CLHEP::cm3;
      G4double densityIsoB = 0.00249  *CLHEP::g/CLHEP::cm3;
      G4double fractionHe  = 75.0*CLHEP::perCent;

      density = fractionHe*densityHe + (1.0-fractionHe)*densityIsoB;
      pressure = 0.4*CLHEP::bar;
      density *= pressure/(1.0*CLHEP::atmosphere);

      G4Material *GasMix = new G4Material( mat.name, density, nel=3,
                                           kStateGas, temperature= 293.15*CLHEP::kelvin, pressure);

      G4Element* He = getElementOrThrow("He");
      G4Element* C  = getElementOrThrow("C");
      G4Element* H  = getElementOrThrow("H");

      G4double atomicWeight_He =  4.002602 *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_C  = 12.0107   *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_H  =  1.00794  *CLHEP::g/CLHEP::mole;
      G4double pwHe = fractionHe*atomicWeight_He;
      G4double pwC  = (1.0-fractionHe) *  4.0*atomicWeight_C;
      G4double pwH  = (1.0-fractionHe) * 10.0*atomicWeight_H;
      G4double atomicWeightMix = pwHe + pwC + pwH ;

      pwHe/=atomicWeightMix;
      pwH/=atomicWeightMix;
      GasMix->AddElement(He, pwHe );
      GasMix->AddElement(H , pwH  );
      GasMix->AddElement(C , 1.0-pwHe-pwH  );
    }

    mat = isNeeded(materialsToLoad, "ITGasHe_90CF4_10");
    if ( mat.doit ){

      G4double density, temperature, pressure;
      G4int nel;

      G4double densityHe   = 0.000166 *CLHEP::g/CLHEP::cm3;
      G4double densityCF4  = 0.003780 *CLHEP::g/CLHEP::cm3;
      G4double fractionHe  = 90.0*CLHEP::perCent;

      density = fractionHe*densityHe + (1.0-fractionHe)*densityCF4;

      G4Material *GasMix = new G4Material( mat.name, density, nel=3,
                                           kStateGas, temperature= 293.15*CLHEP::kelvin, 
					   pressure= 1.*CLHEP::atmosphere);

      G4Element* He = getElementOrThrow("He");
      G4Element* C  = getElementOrThrow("C");
      G4Element* F  = getElementOrThrow("F");

      G4double atomicWeight_He =  4.002602  *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_C  = 12.0107    *CLHEP::g/CLHEP::mole;
      G4double atomicWeight_F  = 18.9984032 *CLHEP::g/CLHEP::mole;
      G4double pwHe = fractionHe*atomicWeight_He;
      G4double pwC  = (1.0-fractionHe) *  1.0*atomicWeight_C;
      G4double pwF  = (1.0-fractionHe) *  4.0*atomicWeight_F;
      G4double atomicWeightMix = pwHe + pwC + pwF ;

      pwHe/=atomicWeightMix;
      pwF/=atomicWeightMix;
      GasMix->AddElement(He, pwHe );
      GasMix->AddElement(F , pwF  );
      GasMix->AddElement(C , 1.0-pwHe-pwF  );
    }

    mat = isNeeded(materialsToLoad, "ITGasMix");
    if ( mat.doit ){
      //He/C4H10-gas-mixture

      G4double density, temperature, pressure;
      G4int nel;

      G4double densityHe   = 0.0001786*CLHEP::g/CLHEP::cm3;
      G4double densityIsoB = 0.00267  *CLHEP::g/CLHEP::cm3;
      G4double fractionHe  = 90.0*CLHEP::perCent;

      density = fractionHe*densityHe + (1.0-fractionHe)*densityIsoB;

      G4Material *GasMix = new G4Material( mat.name, density, nel=3,
                                           kStateGas, temperature= 293.15*CLHEP::kelvin, 
					   pressure= 1.*CLHEP::atmosphere);

      G4Element* He = getElementOrThrow("He");
      G4Element* C  = getElementOrThrow("C");
      G4Element* H  = getElementOrThrow("H");

      GasMix->AddElement(He, 0.9   );
      GasMix->AddElement(H , 0.0173);
      GasMix->AddElement(C , 0.0827);
    }

    mat = isNeeded(materialsToLoad, "ITGasVacuum");
    if ( mat.doit ){
            //
            // This is the lowest density vacuum allowed by G4.
            G4double density     = CLHEP::universe_mean_density;
            G4double pressure    = 3.e-18*CLHEP::pascal;
            G4double temperature = 2.73*CLHEP::kelvin;

            // G4 takes ownership of this object and manages its lifetime.
            new G4Material( mat.name, 1., 1.01 *CLHEP::g/CLHEP::mole,
                            density, kStateGas, temperature, pressure);
    }

    mat = isNeeded(materialsToLoad, "CarbonFiber_resin");
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material* CFresin =
        new G4Material(mat.name, density = 1.1*CLHEP::g/CLHEP::cm3, nel=3);
      G4int natoms;
      CFresin->AddElement(getElementOrThrow("H"),natoms=5);
      CFresin->AddElement(getElementOrThrow("C"),natoms=5);
      CFresin->AddElement(getElementOrThrow("O"),natoms=2);
    }

    mat = isNeeded(materialsToLoad, "CarbonFiber");
    if ( mat.doit ){
      G4double density, fiberFrac=46.0*CLHEP::perCent;
      G4int nel, natoms;
      G4Material* CFresin = findMaterialOrThrow("CarbonFiber_resin");
      G4Material* CFibers = new G4Material("CFibers",density = 1.8*CLHEP::g/CLHEP::cm3,nel=1);
      CFibers->AddElement(getElementOrThrow("C"),natoms=1);

      density = fiberFrac*CFibers->GetDensity()+(1.0-fiberFrac)*CFresin->GetDensity();
      G4Material* CarbonFiber =
        new G4Material(mat.name, density /*= 1.384*CLHEP::g/CLHEP::cm3*/, nel=2);
      CarbonFiber->AddMaterial(CFibers, fiberFrac );
      CarbonFiber->AddMaterial(CFresin, (1.0-fiberFrac) );
    }

    mat = isNeeded(materialsToLoad, "Lyso_01");  /// Alessandra
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material* Lyso_00 =
        new G4Material(mat.name, density = 7.4*CLHEP::g/CLHEP::cm3, nel=4);
      G4Element* Lu  = getElementOrThrow("Lu");
      G4Element* Si  = getElementOrThrow("Si");
      G4Element* O  = getElementOrThrow("O");
      G4Element* Y  = getElementOrThrow("Y");
      G4Element* Ce  = getElementOrThrow("Ce");
      Lyso_00->AddElement( Lu, 71.0*CLHEP::perCent );
      Lyso_00->AddElement( Si, 7.0*CLHEP::perCent );      
      Lyso_00->AddElement( O, 18.0*CLHEP::perCent );      
      Lyso_00->AddElement( Y, 4.0*CLHEP::perCent );
      G4Material* Lyso_01 =
        new G4Material(mat.name, density = 7.4*CLHEP::g/CLHEP::cm3, nel=2);
      Lyso_01->AddMaterial( Lyso_00, 99.85*CLHEP::perCent ); 
      Lyso_01->AddElement( Ce, 0.15*CLHEP::perCent );    
    }   


    //G10-FR4 used for printed board of the I-Tracker
    // G10 http://personalpages.to.infn.it/~tosello/EngMeet/ITSmat/SDD/SDD_G10FR4.html
    // http://pdg.lbl.gov/2002/atomicrpp.pdf
    mat = isNeeded(materialsToLoad, "G10_FR4");
    if ( mat.doit ) {
      G4double density;

      G4Material* G10_FR4 =
        new G4Material(mat.name, density = 1.8*CLHEP::g/CLHEP::cm3, 2);
      G10_FR4->AddMaterial(findMaterialOrThrow("EGlass"), 0.60);
      G10_FR4->AddMaterial(findMaterialOrThrow("Epotek301"), 0.40);
    }

    mat = isNeeded(materialsToLoad, "PolypropyleneFoam");
    if ( mat.doit ){
      //Polypropylene (CH3)
      G4double density;
      G4int nel;
      G4Material *Polypropylene = new G4Material(mat.name, density = 0.04*CLHEP::g/CLHEP::cm3, nel=2);
      G4Element* H  = getElementOrThrow("H");
      G4Element* C  = getElementOrThrow("C");
      Polypropylene->AddElement(H, 3 );
      Polypropylene->AddElement(C, 1 );
    }

    mat = isNeeded(materialsToLoad, "BeFoam_018");
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material *BeFoam_018 = new G4Material(mat.name, density = 0.018*CLHEP::g/CLHEP::cm3, nel=1);
      G4Element* Be  = getElementOrThrow("Be");
      BeFoam_018->AddElement(Be, 100.0*CLHEP::perCent );
    }

    mat = isNeeded(materialsToLoad, "CFoam_332");
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material *CFoam = new G4Material(mat.name, density = 0.332*CLHEP::g/CLHEP::cm3, nel=1);
      G4Element* C  = getElementOrThrow("C");
      CFoam->AddElement(C, 100.0*CLHEP::perCent );
    }

    mat = isNeeded(materialsToLoad, "CFoam_166");
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material *CFoam = new G4Material(mat.name, density = 0.166*CLHEP::g/CLHEP::cm3, nel=1);
      G4Element* C  = getElementOrThrow("C");
      CFoam->AddElement(C, 100.0*CLHEP::perCent );
    }

    mat = isNeeded(materialsToLoad, "CFoam_080");
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material *CFoam = new G4Material(mat.name, density = 0.080*CLHEP::g/CLHEP::cm3, nel=1);
      G4Element* C  = getElementOrThrow("C");
      CFoam->AddElement(C, 100.0*CLHEP::perCent );
    }

    mat = isNeeded(materialsToLoad, "CFoam");
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material *CFoam = new G4Material(mat.name, density = 0.030*CLHEP::g/CLHEP::cm3, nel=1);
      G4Element* C  = getElementOrThrow("C");
      CFoam->AddElement(C, 100.0*CLHEP::perCent );
    }

    mat = isNeeded(materialsToLoad, "KptFoam_030");
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material *KptFoam = new G4Material(mat.name, density = 0.030*CLHEP::g/CLHEP::cm3, nel=1);
      KptFoam->AddMaterial(findMaterialOrThrow("G4_KAPTON"), 100.0*CLHEP::perCent );
    }

    mat = isNeeded(materialsToLoad, "ZirconiumHydridePolyethylene");
    if ( mat.doit ){
      G4Material* ZirconiumHydridePolyethylene =
        new G4Material( mat.name, 3.67*CLHEP::g/CLHEP::cm3, 5);
      G4Element* eC  = getElementOrThrow("C");
      G4Element* eH  = getElementOrThrow("H");
      G4Element* eB  = getElementOrThrow("B");
      G4Element* eO  = getElementOrThrow("O");
      G4Element* eZr  = getElementOrThrow("Zr");

      ZirconiumHydridePolyethylene->AddElement( eC, 8.9*CLHEP::perCent);
      ZirconiumHydridePolyethylene->AddElement( eH, 3.4*CLHEP::perCent);
      ZirconiumHydridePolyethylene->AddElement( eB, 0.5*CLHEP::perCent);
      ZirconiumHydridePolyethylene->AddElement( eO, 2.2*CLHEP::perCent);
      ZirconiumHydridePolyethylene->AddElement( eZr, 85.0*CLHEP::perCent);
    }

    mat = isNeeded(materialsToLoad, "StrawWallEq");
    if ( mat.doit ){
      G4double density;
      G4int nel;
      G4Material* strwMl = findMaterialOrThrow("G4_MYLAR");
      G4Material* strwMet1 = findMaterialOrThrow("G4_Au");
      G4Material* strwMet2 = findMaterialOrThrow("G4_Al");

      density = 1.4325*CLHEP::g/CLHEP::cm3;
      G4Material* stWallEq =
        new G4Material(mat.name, density, nel=3);
      stWallEq->AddMaterial(strwMl, 96.95e-2 );
      stWallEq->AddMaterial(strwMet1, 1.80e-2 );
      stWallEq->AddMaterial(strwMet2, 1.25e-2 );
    }

    // An alias for the stopping target material
    mat = isNeeded(materialsToLoad, "StoppingTarget_"+GlobalConstantsHandle<PhysicsParams>()->getStoppingTargetMaterial());
    if ( true /* Always load the stopping target material */ ){
      G4Material* met = findMaterialOrThrow("G4_"+GlobalConstantsHandle<PhysicsParams>()->getStoppingTargetMaterial());
      G4Material* tgt = new G4Material(mat.name, met->GetDensity(), 1);
      tgt->AddMaterial(met, 1.);
    }

    // Completed constructing Mu2e specific materials.

    // Check that all requested materials are present.
    for ( vector<string>::const_iterator i=materialsToLoad.begin();
          i!=materialsToLoad.end(); ++i ){
      if ( *i != DoAllValue ){
        findMaterialOrThrow(*i);
      }
    }
  }

  // Decide if we need to build this material.
  // If additional tests are required, call them from within this method.
  CheckedG4String ConstructMaterials::isNeeded( vector<string> const& V, string const& s){

    // Default return value is not to build it.
    CheckedG4String val(false,s);

    // Throw if the material already exists.
    uniqueMaterialOrThrow(val.name);

    // Is this material requested explicitly?
    val.doit = isRequested( V, s );

    // Is this material requested implicitly?
    if ( !val.doit ) val.doit = isRequested(V, DoAllValue);

    return val;

  }

  // Return true if the requested string is present in the container.
  // The match must be exact.
  bool ConstructMaterials::isRequested( vector<string> const& V, string const& s){
    for ( vector<string>::const_iterator i=V.begin(), e=V.end();
          i != e; ++i ){
      if ( *i == s ) return true;
    }
    return false;
  }

  // Check to see if the named material already exists.
  void ConstructMaterials::uniqueMaterialOrThrow( G4String const& name){
    if ( G4Material::GetMaterial(name,false) != 0 ){
      throw cet::exception("GEOM")
        << "mu2e::ConstructMaterials::constructMu2eMaterials(): "
        << "The requested material is already defined: "
        << name
        << "\n";
    }
  }

  // Wrapper around FindOrBuildElement.
  // This is protection against spelling mistakes in the name.
  G4Element* ConstructMaterials::getElementOrThrow( G4String const& name){

    // G4 manager for elements and materials.
    G4NistManager* nistMan = G4NistManager::Instance();

    G4Element* answer = nistMan->FindOrBuildElement(name,true);

    // Throw if we could not find a requested element.
    if ( !answer ){
      throw cet::exception("GEOM")
        << "mu2e::ConstructMaterials::constructMaterials(): "
        << "Could not load predefined G4 element named: "
        << name
        << "\n";
    }

    return answer;
  }

} // end namespace mu2e
