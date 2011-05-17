//
// Module which starts the event display, and transmits the data of each event to the event display.
//
// $Id: EventDisplay_plugin.cc,v 1.7 2011/05/17 15:35:59 greenc Exp $
// $Author: greenc $ 
// $Date: 2011/05/17 15:35:59 $
//

#include <iostream>
#include <string>
#include <memory>

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Persistency/Common/Handle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "ToyDP/inc/StepPointMCCollection.hh"
#include "ToyDP/inc/StrawHitCollection.hh"

#include "TApplication.h"
#include "TGMsgBox.h"

#include "EventDisplayFrame.h"

namespace mu2e 
{
  class EventDisplay : public art::EDAnalyzer 
  {
    public:
    explicit EventDisplay(fhicl::ParameterSet const&);
    virtual ~EventDisplay() { }
    virtual void beginJob(art::EventSetup const&);
    void endJob();
    void analyze(const art::Event& e, art::EventSetup const&);

    private:
    template<class collectionType> void checkMinimumHits(const art::Event &event, 
                                                         const std::string &classNameToCheck,
                                                         const mu2e_eventdisplay::EventDisplayFrame *_frame, 
                                                         bool &showEvent);
    mu2e_eventdisplay::EventDisplayFrame *_frame;
    bool _firstLoop;
  };

  EventDisplay::EventDisplay(fhicl::ParameterSet const& ) 
  {
    _firstLoop=true;
  }

  void EventDisplay::beginJob(art::EventSetup const& )
  {
    //Bare pointer are needed for gApplication and _mainframe to avoid
    //that the destructor for these two objects gets called, i.e.
    //there cannot be a "delete" for these two objects.
    //The reason is that at least one of the ROOT destructors 
    //(which a destructor call for these two objects would trigger) 
    //has a bug (perhaps deletes things twice), so that the program 
    //would crash.
    if (!gApplication) gApplication = new TApplication("EventDisplay",0,0);

    //don't create the eventdisplay here to avoid an empty (and not-responding)
    //eventdisplay window, because it usually takes a while until the first event gets pushed through
  }

  void EventDisplay::analyze(const art::Event& event, art::EventSetup const&) 
  {
    if(_firstLoop)
    {
      _frame = new mu2e_eventdisplay::EventDisplayFrame(gClient->GetRoot(), 800, 550);
      if(!_frame->isClosed()) _frame->fillGeometry();
    }
    if(!_frame->isClosed())
    {
      bool findEvent=false;
      int eventToFind=_frame->getEventToFind(findEvent);
      if(findEvent)
      {
        int eventNumber=event.id().event();
        if(eventNumber==eventToFind) _frame->setEvent(event);
        else std::cout<<"event skipped, since this is not the event we are looking for"<<std::endl;
      }
      else
      {
        bool showEvent=true;
        checkMinimumHits<mu2e::StepPointMCCollection>(event, "std::vector<mu2e::StepPointMC>", _frame, showEvent);
        checkMinimumHits<mu2e::StrawHitCollection>(event, "std::vector<mu2e::StrawHit>", _frame, showEvent);
        if(showEvent) _frame->setEvent(event,_firstLoop);
      }
    }
    _firstLoop=false;
  }

  template<class collectionType>
  void EventDisplay::checkMinimumHits(const art::Event &event, const std::string &classNameToCheck,
                                      const mu2e_eventdisplay::EventDisplayFrame *_frame, bool &showEvent)
  {
    std::string className, moduleLabel, productInstanceName;
    bool hasSelectedHits=_frame->getSelectedHitsName(className, moduleLabel, productInstanceName);
    if(hasSelectedHits && className.compare(classNameToCheck)==0)
    {
      art::Handle<collectionType> hits;
      if(event.getByLabel(moduleLabel,productInstanceName,hits))
      {
        if(static_cast<int>(hits->size()) < _frame->getMinimumHits())
        {
          std::cout<<"event skipped, since it doesn't have enough hits"<<std::endl;
          showEvent=false;
        }
      }
    }
  }

  void EventDisplay::endJob()
  {
    if(!_frame->isClosed())
    {
      bool findEvent=false;
      int eventToFind=_frame->getEventToFind(findEvent);
      if(findEvent)
      {
        char msg[300];
        sprintf(msg,"The end of file has been reached, but the event #%i has not been found.",eventToFind);
        TGMsgBox *eventNotFoundBox;
        eventNotFoundBox = new TGMsgBox(gClient->GetRoot(),gClient->GetRoot(),"Event Not Found",msg,kMBIconExclamation,kMBOk);
      }
      _frame->CloseWindow();
    }
  }
}

using mu2e::EventDisplay;
DEFINE_ART_MODULE(EventDisplay);
