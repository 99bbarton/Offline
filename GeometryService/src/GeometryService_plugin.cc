//
// Plugin for the Geometry Service.
//
// $Id: GeometryService_plugin.cc,v 1.2 2011/05/17 15:36:00 greenc Exp $
// $Author: greenc $ 
// $Date: 2011/05/17 15:36:00 $
//
// Original author Rob Kutschke
//

#include "FWCore/ServiceRegistry/interface/ServiceMaker.h"
#include "GeometryService/inc/GeometryService.hh"

using mu2e::GeometryService;

DEFINE_ART_SERVICE(GeometryService);
