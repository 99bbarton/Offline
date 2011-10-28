//
// Class which builds the main frame for the event display, and provides functions to control the display, e.g. quit, moving to the next event, animations, storing the events into gif files (static and animated), detailed infos of tracks, hits, etc.
//
// $Id: EventDisplayFrame.h,v 1.21 2011/10/28 18:47:06 greenc Exp $
// $Author: greenc $
// $Date: 2011/10/28 18:47:06 $
//
// Original author Ralf Ehrlich
//

#ifndef EventDisplay_src_EventDisplayFrame_h
#define EventDisplay_src_EventDisplayFrame_h

#include <iostream>
#include <TGFrame.h>
#ifndef __CINT__
#include "art/Framework/Principal/Event.h"
#include "boost/shared_ptr.hpp"
#endif

class TBox;
class TGCheckButton;
class TGLabel;
class TGRadioButton;
class TGTextEntry;
class TPad;
class TPolyLine;
class TGCanvas;
class TRootEmbeddedCanvas;
class TText;
class TTimer;

namespace fhicl 
{
  class ParameterSet;
}

namespace mu2e_eventdisplay
{
  class ContentSelector;
  class DataInterface;
  class RootFileManager;
  class HistDraw;

  class EventDisplayFrame : public TGMainFrame
  {
    EventDisplayFrame();
    EventDisplayFrame(const EventDisplayFrame &);
    EventDisplayFrame& operator=(const EventDisplayFrame &);

    public:
    EventDisplayFrame(const TGWindow* p, UInt_t w, UInt_t h, fhicl::ParameterSet const &pset);
    virtual          ~EventDisplayFrame();
    void             fillGeometry();
#ifndef __CINT__     //hide art::Event from ROOTCint
    void             setEvent(const art::Event& event, bool firstLoop=false);
    boost::shared_ptr<RootFileManager> getRootFileManager() {return _rootFileManager;}
    std::vector<boost::shared_ptr<HistDraw> > &getHistDrawVector(){return _histDrawVector;}
#endif
    bool             isClosed() const;
    bool             getSelectedHitsName(std::string &className,
                                         std::string &moduleLabel,
                                         std::string &productInstanceName) const;
    int              getMinimumHits() const;
    int              getEventToFind(bool &findEvent) const;
    void             showInfo(TObject*);
    void             showHist(TObject*);
    void             keyboardInput();
    void             fillZoomAngleFields();
    void             addHistDraw();
    virtual void     CloseWindow(); //inherited from TGMainFrame

    private:
    void shrinkButton(TGTextButton *button);
    void fillEvent(bool firstLoop=false);
    void updateTimeIntervalFields(bool allTracks=false);
    void updateHitLegend(bool draw);
    void updateTrackLegend(bool draw);
    void prepareAnimation();
    void drawSituation();
    void drawEverything();
    void createAnimFile();
    //the following functions are inherited from TGMainFrame
    virtual Bool_t HandleConfigureNotify(Event_t *event);
    virtual Bool_t ProcessMessage(Long_t msg, Long_t param1, Long_t param2);
    virtual Bool_t HandleTimer(TTimer *); //inherited function
                                          //from TObject (gets called when
                                          //timer times out - knows about
                                          //this via TTimer::SetObject)

#ifndef __CINT__    //hide boost from ROOTCint
    boost::shared_ptr<DataInterface>   _dataInterface;
    boost::shared_ptr<ContentSelector> _contentSelector;
    boost::shared_ptr<RootFileManager> _rootFileManager;
    boost::shared_ptr<RootFileManager> _rootFileManagerAnim;
    std::vector<boost::shared_ptr<HistDraw> > _histDrawVector;
#endif
    double              _timeCurrent, _timeStart, _timeStop;
    int                 _minHits, _eventToFind;
    int                 _eventNumber, _runNumber;
    bool                _isClosed, _findEvent;
    bool                _saveAnim, _saveAnimRoot;
    int                 _saveAnimCounter;
    std::string         _saveAnimFile;
    //bare pointers needed since ROOT manages these objects
    TGHorizontalFrame   *_mainFrame, *_footLine;
    TGVerticalFrame     *_subFrame;
    TRootEmbeddedCanvas *_mainCanvas, *_infoEmbeddedCanvas;
    TGCanvas            *_infoCanvas;
    TPad                *_mainPad, *_infoPad;
    TText               *_clock, *_eventNumberText, *_runNumberText;
    TTimer              *_timer;
    TGCheckButton       *_unhitButton, *_unhitCrystalsButton;
    TGCheckButton       *_supportStructuresButton, *_otherStructuresButton;
    TGCheckButton       *_hitColorButton, *_trackColorButton, *_backgroundButton;
    TGCheckButton       *_repeatAnimationButton;
    TGTextEntry         *_timeIntervalField1, *_timeIntervalField2;
    TGTextEntry         *_minHitField, *_eventToFindField;
    TGTextEntry         *_minXField, *_minYField, *_minZField, *_maxXField, *_maxYField, *_maxZField;
    TGTextEntry         *_phiField, *_thetaField, *_psiField;
    TGRadioButton       *_perspectiveButton, *_parallelButton;
    TGLabel             **_eventInfo;
    TText               *_legendText[30], *_legendParticleGroup[30], *_legendParticleText[30];
    TBox                *_legendBox[30];
    TPolyLine           *_legendParticleLine[30];
    std::string         _g4ModuleLabel;

    ClassDef(EventDisplayFrame,0);
  };
}

#endif /* EventDisplay_src_EventDisplayFrame_h */
