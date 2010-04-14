#ifndef STRAWINDEX_HH
#define STRAWINDEX_HH

//
// Dense integer identifier of one straw.
// Has values 0...(N-1), where N is the number
// of straws in the system.  This works for both the LTracker
// and the TTracker.
//
// $Id: StrawIndex.hh,v 1.2 2010/04/14 14:16:41 kutschke Exp $
// $Author: kutschke $
// $Date: 2010/04/14 14:16:41 $
//
// Original author Rob Kutschke
//
// This class exists to allow compile time checks that
// the code is using the correct sort of index when indexing
// into a container that requires a StrawIndex as an index.
// For now it is a bit of an experiment.
//
// Most users should never need to access a StrawIndex as an
// integer or set one from an integer.  If this goes according
// to plan, only the code that creates the LTracker will need
// to set one from an integer.
//

#include <ostream>

namespace mu2e {


  class StrawIndex{

  public:

    // No default c'tor by design.

    // No automatic conversion of int to StrawIndex.
    explicit StrawIndex(int32_t idx):
      _idx(idx){
    }
    
    // Compiler generated versions are OK for:
    // copy c'tor, destructor, operator=

    // Return the value as an int.
    // Do not want automatic conversion to an int.
    int32_t asInt() const { return _idx;}

    bool operator==( StrawIndex const& rhs) const{
      return (_idx == rhs._idx);
    }

    bool operator<( StrawIndex const& rhs) const{
      return ( _idx < rhs._idx);
    }

  private:

    int32_t _idx;
  };

  inline std::ostream& operator<<( std::ostream& ost,
				   StrawIndex const& i){
    ost << i.asInt();
    return ost;
  }

  inline bool operator!=( StrawIndex const& lhs, 
			  StrawIndex const& rhs) {
      return !( lhs == rhs);
  }

  

}

#endif
