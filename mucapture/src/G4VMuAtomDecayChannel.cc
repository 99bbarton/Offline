// ------------------------------------------------------------
//      GEANT 4 implementation file
//
//      History: first implementation, cloned from G4VDecayChannel
//      20 February 2010 Kevin Lynch
// ------------------------------------------------------------

#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4DecayTable.hh"
#include "G4DecayProducts.hh"
#include "G4VMuAtomDecayChannel.hh"

const G4String G4VMuAtomDecayChannel::noName = " ";

G4VMuAtomDecayChannel::G4VMuAtomDecayChannel(const G4String &aName, G4int Verbose)
               :kinematics_name(aName),
		rbranch(0.0),
		numberOfDaughters(0),
		parent_name(0), daughters_name(0),
		particletable(0),
		parent(0), daughters(0),
		parent_mass(0.0), daughters_mass(0),
		verboseLevel(Verbose)		
{
  // set pointer to G4ParticleTable (static and singleton object)
  particletable = G4ParticleTable::GetParticleTable();
}

G4VMuAtomDecayChannel::G4VMuAtomDecayChannel(const G4String  &aName, 
			       const G4String& theParentName,
			       G4double        theBR,
			       G4int           theNumberOfDaughters,
			       const G4String& theDaughterName1,
			       const G4String& theDaughterName2,
			       const G4String& theDaughterName3,
			       const G4String& theDaughterName4 )
               :kinematics_name(aName),
		rbranch(theBR),
		numberOfDaughters(theNumberOfDaughters),
		parent_name(0), daughters_name(0),
		particletable(0),
		parent(0), daughters(0),
		parent_mass(0.0), daughters_mass(0),
		verboseLevel(1)		
{
  // set pointer to G4ParticleTable (static and singleton object)
  particletable = G4ParticleTable::GetParticleTable();

  // parent name
  parent_name = new G4String(theParentName);

  // cleate array
  daughters_name = new G4String*[numberOfDaughters];
  for (G4int index=0;index<numberOfDaughters;index++) daughters_name[index]=0;

  // daughters' name
  if (numberOfDaughters>0) daughters_name[0] = new G4String(theDaughterName1);
  if (numberOfDaughters>1) daughters_name[1] = new G4String(theDaughterName2);
  if (numberOfDaughters>2) daughters_name[2] = new G4String(theDaughterName3);
  if (numberOfDaughters>3) daughters_name[3] = new G4String(theDaughterName4);
}



G4VMuAtomDecayChannel::G4VMuAtomDecayChannel(const G4VMuAtomDecayChannel &right)
{
  kinematics_name = right.kinematics_name;
  verboseLevel = right.verboseLevel;
  rbranch = right.rbranch;

  // copy parent name
  parent_name = new G4String(*right.parent_name);
  parent = 0;
  parent_mass = 0.0; 

  //create array
  numberOfDaughters = right.numberOfDaughters;

  if ( numberOfDaughters >0 ) {
    daughters_name = new G4String*[numberOfDaughters];
    //copy daughters name
    for (G4int index=0; index < numberOfDaughters; index++)
      {
	daughters_name[index] = new G4String(*right.daughters_name[index]);
      }
  }

  //
  daughters_mass = 0;
  daughters = 0;

  // particle table
  particletable = G4ParticleTable::GetParticleTable();
}

G4VMuAtomDecayChannel & G4VMuAtomDecayChannel::operator=(const G4VMuAtomDecayChannel &right)
{
  if (this != &right) { 
    kinematics_name = right.kinematics_name;
    verboseLevel = right.verboseLevel;
    rbranch = right.rbranch;

    // copy parent name
    parent_name = new G4String(*right.parent_name);

    // clear daughters_name array
    ClearDaughtersName();

    // recreate array
    numberOfDaughters = right.numberOfDaughters;
    if ( numberOfDaughters >0 ) {
      daughters_name = new G4String*[numberOfDaughters];
      //copy daughters name
      for (G4int index=0; index < numberOfDaughters; index++) {
	  daughters_name[index] = new G4String(*right.daughters_name[index]);
      }
    }
  }

  //
  parent = 0;
  daughters = 0;
  parent_mass = 0.0;
  daughters_mass = 0;

  // particle table
  particletable = G4ParticleTable::GetParticleTable();

  return *this;
}


G4VMuAtomDecayChannel::~G4VMuAtomDecayChannel()
{
  ClearDaughtersName();
  if (parent_name != 0) delete parent_name;
  parent_name = 0;
  if (daughters_mass != 0) delete [] daughters_mass;
  daughters_mass =0;
} 

void G4VMuAtomDecayChannel::ClearDaughtersName()
{
  if ( daughters_name != 0) {
    if (numberOfDaughters>0) {
#ifdef G4VERBOSE
      if (verboseLevel>1) {
	G4cerr << "G4VMuAtomDecayChannel::ClearDaughtersName "
	       << " for " << *parent_name << G4endl;
      }
#endif
      for (G4int index=0; index < numberOfDaughters; index++) { 
	if (daughters_name[index] != 0) delete daughters_name[index];
      }
    }
    delete [] daughters_name;
    daughters_name = 0;
  }
  // 
  if (daughters != 0) delete [] daughters;
  if (daughters_mass != 0) delete [] daughters_mass;
  daughters = 0;
  daughters_mass = 0;

  numberOfDaughters = 0;
}

void G4VMuAtomDecayChannel::SetNumberOfDaughters(G4int size)
{
  if (size >0) {
    // remove old contents
    ClearDaughtersName();
    // cleate array
    daughters_name = new G4String*[size];
    for (G4int index=0;index<size;index++) daughters_name[index]=0;
    numberOfDaughters = size;
  }
}

void G4VMuAtomDecayChannel::SetDaughter(G4int anIndex, 
				 const G4String &particle_name)
{
  // check numberOfDaughters is positive
  if (numberOfDaughters<=0) {
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VMuAtomDecayChannel::SetDaughter: ";
      G4cout << "Number of daughters is not defined" << G4endl;
    }
#endif
    return;
  }

  // check existence of daughters_name array
  if (daughters_name == 0) {
    // cleate array
    daughters_name = new G4String*[numberOfDaughters];
    for (G4int index=0;index<numberOfDaughters;index++) {
      daughters_name[index]=0;
    }
  }

  // check an index    
  if ( (anIndex<0) || (anIndex>=numberOfDaughters) ) {
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VMuAtomDecayChannel::SetDaughter";
      G4cout << "index out of range " << anIndex << G4endl;
    }
#endif
  } else {
    // delete the old name if it exists
    if (daughters_name[anIndex]!=0) delete daughters_name[anIndex];
    // fill the name
    daughters_name[anIndex] = new G4String(particle_name);
    // refill the array of daughters[] if it exists
    if (daughters != 0) FillDaughters();
#ifdef G4VERBOSE
    if (verboseLevel>1) {
      G4cout << "G4VMuAtomDecayChannel::SetDaughter[" << anIndex <<"] :";
      G4cout << daughters_name[anIndex] << ":" << *daughters_name[anIndex]<<G4endl;
    }
#endif
  }
}

void G4VMuAtomDecayChannel::SetDaughter(G4int anIndex, const G4ParticleDefinition * parent_type)
{
  if (parent_type != 0) SetDaughter(anIndex, parent_type->GetParticleName());
}

void G4VMuAtomDecayChannel::FillDaughters()
{
  G4int index;
  
#ifdef G4VERBOSE
  if (verboseLevel>1) G4cout << "G4VMuAtomDecayChannel::FillDaughters()" <<G4endl;
#endif
  if (daughters != 0) delete [] daughters;

  // parent mass
  if (parent == 0) FillParent();  
  G4double parentmass = parent->GetPDGMass();

  //
  G4double sumofdaughtermass = 0.0;
  if ((numberOfDaughters <=0) || (daughters_name == 0) ){
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VMuAtomDecayChannel::FillDaughters   ";
      G4cout << "[ " << parent->GetParticleName() << " ]";
      G4cout << "numberOfDaughters is not defined yet";
    }
#endif
    daughters = 0;
    G4Exception("G4VMuAtomDecayChannel::FillDaughters",
		"can not fill daughters", FatalException,
		"numberOfDaughters is not defined yet");    
  } 

  //create and set the array of pointers to daughter particles
  daughters = new G4ParticleDefinition*[numberOfDaughters];
  if (daughters_mass != 0) delete [] daughters_mass;
  daughters_mass = new G4double[numberOfDaughters];
  // loop over all daughters
  for (index=0; index < numberOfDaughters;  index++) { 
    if (daughters_name[index] == 0) {
      // daughter name is not defined
#ifdef G4VERBOSE
      if (verboseLevel>0) {
	G4cout << "G4VMuAtomDecayChannel::FillDaughters  ";
	G4cout << "[ " << parent->GetParticleName() << " ]";
	G4cout << index << "-th daughter is not defined yet" << G4endl;
      }
#endif
      daughters[index] = 0;
      G4Exception("G4VMuAtomDecayChannel::FillDaughters",
		  "can not fill daughters", FatalException,
		  "name of a daughter is not defined yet");    
    } 
    //search daughter particles in the particle table 
    daughters[index] = particletable->FindParticle(*daughters_name[index]);
    if (daughters[index] == 0) {
      // can not find the daughter particle
#ifdef G4VERBOSE
      if (verboseLevel>0) {
	G4cout << "G4VMuAtomDecayChannel::FillDaughters  ";
	G4cout << "[ " << parent->GetParticleName() << " ]";
	G4cout << index << ":" << *daughters_name[index];
	G4cout << " is not defined !!" << G4endl;
        G4cout << " The BR of this decay mode is set to zero " << G4endl;
      }
#endif
      SetBR(0.0);
      return;
    }
#ifdef G4VERBOSE
    if (verboseLevel>1) {
      G4cout << index << ":" << *daughters_name[index];
      G4cout << ":" << daughters[index] << G4endl;
    }
#endif
    daughters_mass[index] = daughters[index]->GetPDGMass();
    sumofdaughtermass += daughters[index]->GetPDGMass();
  }  // end loop over all daughters

  // check sum of daghter mass
  G4double widthMass = parent->GetPDGWidth();
  if ( (parent->GetParticleType() != "nucleus") &&
       (sumofdaughtermass > parentmass + 5*widthMass) ){
   // !!! illegal mass  !!!
#ifdef G4VERBOSE
   if (GetVerboseLevel()>0) {
     G4cout << "G4VMuAtomDecayChannel::FillDaughters ";
     G4cout << "[ " << parent->GetParticleName() << " ]";
     G4cout << "    Energy/Momentum conserevation breaks " <<G4endl;
     if (GetVerboseLevel()>1) {
       G4cout << "    parent:" << *parent_name;
       G4cout << " mass:" << parentmass/GeV << "[GeV/c/c]" <<G4endl;
       for (index=0; index < numberOfDaughters; index++){
	 G4cout << "     daughter " << index << ":" << *daughters_name[index];
	 G4cout << " mass:" << daughters[index]->GetPDGMass()/GeV;
	 G4cout << "[GeV/c/c]" <<G4endl;
       }
     }
   }
#endif
 }
}


void G4VMuAtomDecayChannel::FillParent()
{
  if (parent_name == 0) {
    // parent name is not defined
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VMuAtomDecayChannel::FillParent   ";
      G4cout << ": parent name is not defined !!" << G4endl;
    }
#endif
    parent = 0;
    G4Exception("G4VMuAtomDecayChannel::FillParent()",
		"can not fill parent", FatalException,
		"parent name is not defined yet");    
  }
  // search parent particle in the particle table
  parent = particletable->FindParticle(*parent_name);
  if (parent == 0) {
    // parent particle does not exist
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VMuAtomDecayChannel::FillParent   ";
      G4cout << *parent_name << " does not exist !!" << G4endl;
    }
#endif
    G4Exception("G4VMuAtomDecayChannel::FillParent()",
		"can not fill parent", FatalException,
		"parent does not exist");    
  }
  parent_mass = parent->GetPDGMass();
}

void G4VMuAtomDecayChannel::SetParent(const G4ParticleDefinition * parent_type)
{
  if (parent_type != 0) SetParent(parent_type->GetParticleName());
}

G4int G4VMuAtomDecayChannel::GetAngularMomentum()
{
  // determine angular momentum

  // fill pointers to daughter particles if not yet set  
  if (daughters == 0) FillDaughters();

  const G4int PiSpin = parent->GetPDGiSpin();
  const G4int PParity = parent->GetPDGiParity();
  if (2==numberOfDaughters) {     // up to now we can only handle two particle decays
    const G4int D1iSpin  = daughters[0]->GetPDGiSpin();
    const G4int D1Parity = daughters[0]->GetPDGiParity();
    const G4int D2iSpin  = daughters[1]->GetPDGiSpin();
    const G4int D2Parity = daughters[1]->GetPDGiParity();
    const G4int MiniSpin = std::abs (D1iSpin - D2iSpin);
    const G4int MaxiSpin = D1iSpin + D2iSpin;
    const G4int lMax = (PiSpin+D1iSpin+D2iSpin)/2; // l is allways int
    G4int lMin;
#ifdef G4VERBOSE
    if (verboseLevel>1) {
      G4cout << "iSpin: " << PiSpin << " -> " << D1iSpin << " + " << D2iSpin << G4endl;
      G4cout << "2*jmin, 2*jmax, lmax " << MiniSpin << " " << MaxiSpin << " " << lMax << G4endl;
    }
#endif
    for (G4int j=MiniSpin; j<=MaxiSpin; j+=2){    // loop over all possible spin couplings
      lMin = std::abs(PiSpin-j)/2;
#ifdef G4VERBOSE 
      if (verboseLevel>1)
	G4cout << "-> checking 2*j=" << j << G4endl;
#endif
      for (G4int l=lMin; l<=lMax; l++) {
#ifdef G4VERBOSE
	if (verboseLevel>1)
	  G4cout << " checking l=" << l << G4endl;
#endif
        if (l%2==0) {
	  if (PParity == D1Parity*D2Parity) {    // check parity for this l
	    return l;
          } 
	} else {
	  if (PParity == -1*D1Parity*D2Parity) {    // check parity for this l
            return l;
          }
        }
      }
    }
  } else {
    G4Exception("G4VMuAtomDecayChannel::GetAngularMomentum",
		"can not calculate", JustWarning,
		"Sorry, can't handle 3 particle decays (up to now)");
    return 0;
  }
  G4Exception ("G4VMuAtomDecayChannel::GetAngularMomentum",
		"can not calculate", JustWarning,
		"Can't find angular momentum for this decay!");
  return 0;
}

void G4VMuAtomDecayChannel::DumpInfo()
{
  G4cout << " BR:  " << rbranch << "  [" << kinematics_name << "]";
  G4cout << "   :  " ;
  for (G4int index=0; index < numberOfDaughters; index++)
  {
    if(daughters_name[index] != 0) {
      G4cout << " " << *(daughters_name[index]);
    } else {
      G4cout << " not defined ";
    }
  }
  G4cout << G4endl;
}

const G4String& G4VMuAtomDecayChannel::GetNoName() const
{
  return noName;
}
