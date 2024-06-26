// usage
// root [0] .L macros/onlinemonitor/OnlineMonitor.cc+g
// root [1] OnlineMonitor* mon = new OnlineMonitor()
// root [2] mon->Run()
//
#include "OnlineMonitor.hh"

#include <iostream>
#include <csignal>

#include "TSystem.h"
#include "TPad.h"
#include "TCanvas.h"
#include "TDatime.h"
#include "TText.h"
#include "TClonesArray.h"

#include "TArtStoreManager.hh"
#include "TArtRawEventObject.hh"
#include "TArtRawSegmentObject.hh"
#include "TArtEventInfo.hh"

#include "CoinDataProcessor.hh"
#include "PlasticDataProcessor.hh"
#include "BDCDataProcessor.hh"
#include "FDC0DataProcessor.hh"
#include "FDC1DataProcessor.hh"
#include "FDC2DataProcessor.hh"
#include "NEBULADataProcessor.hh"
#include "HODPlaDataProcessor.hh"
#include "NINJADataProcessor.hh"

//_________________________________________________________________________________
// function to exit loop at keyboard interrupt.
bool stoploop = false;
void stop_interrupt(int) {
  printf("keyboard interrupt\n");
  stoploop = true;
}
//_________________________________________________________________________________
void OnlineMonitor::Run()// main 
{
  if (!fInitialize) {
    Init();
  }

  if (!fIsHistBooked) {
    BookHist();
    BookUserHist();
  }

  EventLoop();
}
//_________________________________________________________________________________
void OnlineMonitor::Init()
{
  // Path for Input files
  const char *TDCdist = "/home/tpohl/online_analysis/files_for_online_analysis/dcdist_0537.root";

  // change for your experiment
  fProcessorArray.push_back(new CoinDataProcessor);
//  fProcessorArray.push_back(new PlasticDataProcessor);
//  fProcessorArray.push_back(new BDCDataProcessor(TDCdist));
//  fProcessorArray.push_back(new FDC0DataProcessor(TDCdist));
//  fProcessorArray.push_back(new FDC1DataProcessor(TDCdist));
//  fProcessorArray.push_back(new FDC2DataProcessor(TDCdist));
//  fProcessorArray.push_back(new HODPlaDataProcessor);
//  fProcessorArray.push_back(new NEBULADataProcessor);
  fProcessorArray.push_back(new NINJADataProcessor);


  fnx=4;
  fny=4;
  fResetCount=20000;
  fDrawTimeInterval=2;// sec

  festore = new TArtEventStore();
  fNeve = 0;
  fNeve_monitor = 0;

  fInitialize = true;

}
//_________________________________________________________________________________
void OnlineMonitor::BookUserHist()
{
  fRootFile->cd();

  // add user histograms

  TH1* fhcoin = new TH1I("hcoin","COIN",8,0.5,8.5);
//  fhcoin->SetMinimum(0);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);
  fHistArray.push_back(fhcoin);

  fIsHistBooked = true;
}
//______________________________________________________________________________
void OnlineMonitor::FillUserHist()
{
  TArtStoreManager *sman = TArtStoreManager::Instance();
  //TClonesArray *a = sman->FindDataContainer("NEBULAPla");
}
//_________________________________________________________________________________
//_________________________________________________________________________________
//_________________________________________________________________________________
//_________________________________________________________________________________
OnlineMonitor::OnlineMonitor(TString RidfFile)
  : festore(0),
    fInitialize(false), fIsHistBooked(false),
    fRunName(RidfFile),
    fResetCount(20000), fDrawTimeInterval(10),
    fDoesReset(true),
    f_iplot(0)
{
  if (gPad==0) fCanvas = new TCanvas("c1","c1",1200,800);
  else         fCanvas = gPad->GetCanvas();

  std::cout<<"OnlineMonitor::Run()"<<std::endl;
}
//_________________________________________________________________________________
OnlineMonitor::~OnlineMonitor()
{
  fRootFile->Close();
  delete festore;
}
//_________________________________________________________________________________
void OnlineMonitor::BookHist()
{
  fRootFile = new TFile("OnlineMonitor.root","recreate");

  Int_t np = fProcessorArray.size();
  for (Int_t ip=0;ip<np;++ip){
    SAMURAIDataProcessor *p = fProcessorArray[ip];
    p->MakeHistograms(true);
    p->PrepareHistograms();

    vector<TH1*>* arr = p->GetHistArray();
    Int_t nhist = arr->size();
    for (Int_t ihist=0;ihist<nhist;++ihist){
      fHistArray.push_back( (*arr)[ihist] );
    }
  }
  
  fIsHistBooked = true;
}
//_________________________________________________________________________________
void OnlineMonitor::AnalyzeOneEvent()
{
  TArtStoreManager *sman = TArtStoreManager::Instance();
  if(festore->GetNextEvent()){

    TArtRawEventObject *rawevent = festore->GetRawEventObject();
    if (rawevent->GetEventNumber()<0) {
      TArtCore::Error("OnlineMonitor.cc","Invalid event ID=%d",rawevent->GetEventNumber());
    }


    Int_t np = fProcessorArray.size();

    for (int ip=0;ip<np;ip++) fProcessorArray[ip]->ClearData();
    for (int ip=0;ip<np;ip++) fProcessorArray[ip]->ReconstructData();

    for (int ip=0;ip<np;ip++){
      SAMURAIDataProcessor *p = fProcessorArray[ip];
      if (p->DoesMakeHistograms()) p->FillHistograms();
    }

    FillUserHist();

    //------------------------------------------
    festore->ClearData();
  }
}
//______________________________________________________________________________
void OnlineMonitor::EventLoop()
{
  signal(SIGINT, stop_interrupt);
  if (fRunName=="online"){
    festore->Open(0);
  }else if (fRunName=="nebula"){
    festore->Open(1);
  }else {
    festore->Open(fRunName.Data());
  }

  TDatime datime_old;
//  Double_t neve_rate = 0;
  while(true){
    gSystem->ProcessEvents();
    if (fNeve%100==0)
      std::cout<<"\r"<<fNeve_monitor<<" events for display, "
	       <<fNeve<<" events analyzed"<<std::flush;

    if (stoploop) {
      stoploop = false;
      break;
    }

    EEventType event = static_cast<EEventType>(fCanvas->GetEvent());
    if(event == kKeyPress){
      std::cout<<std::endl;
      int ret = GetKeyCommand();
      if (ret==-1) break;
    }

    TDatime datime;

    AnalyzeOneEvent();

    ++fNeve;
    ++fNeve_monitor;

    if (datime.GetTime() - datime_old.GetTime() > fDrawTimeInterval){
      //neve_rate = (Double_t)fNeve/(datime.GetTime() - datime_old.GetTime());

      Draw();
      //std::cout<<"\r    "<<fNeve_monitor<<" evts are accumulated (Ctrl+C for stop) "<<std::flush;
      fRootFile->Write();

      datime_old = datime;

      if (fDoesReset){
	if (fNeve_monitor>fResetCount){
	  ResetHist();
	  fNeve_monitor = 0;
	}
      }
    }//if (datime.GetTime() - datime_old.GetTime() > fDrawTimeInterval){

  }//while(true){

}
//______________________________________________________________________________
bool OnlineMonitor::Draw()
{
  bool retval = false;

  Int_t nxy = fnx*fny;

  Int_t npad=1;
  TCanvas *c1 = gPad->GetCanvas();
  c1->Clear();
  TDatime datime;
  TString title=fRunName + " (";

  title+=datime.AsString();
  title+=")";
  c1->SetTitle(title.Data());
  c1->Divide(fnx,fny);

  Int_t nhist = fHistArray.size();
  for(Int_t ihist=0;ihist<nxy;++ihist){
    c1->cd(npad);

    if ( f_iplot >= nhist ) f_iplot = 0;
    TH1* hist = fHistArray[f_iplot];
    if (hist->GetNbinsY()>0) hist->Draw("colz");
    else                     hist->Draw();

    npad++;
    if (npad>nxy) npad=1;
    f_iplot++;
    if (f_iplot==nhist) {
      //f_iplot=0;
      retval = true;
      break;
    }

  }

  gPad->Update();
  return retval;
}
//_________________________________________________________________________________
void OnlineMonitor::ResetHist()
{
  Int_t nhist = fHistArray.size();
  for(Int_t ihist=0;ihist<nhist;++ihist){
    TH1* hist = fHistArray[ihist];
    hist->Reset("M");
  }
}
//______________________________________________________________________________
void OnlineMonitor::Print(const char* fname)
{
  fCanvas->Print(Form("%s[",fname));
  f_iplot=0;
  bool IsEnd=false;
  while (!IsEnd){
    IsEnd = Draw();
    fCanvas->Print(Form("%s",fname));
  }
  fCanvas->Print(Form("%s]",fname));
}
//______________________________________________________________________________
int OnlineMonitor::GetKeyCommand()
{


  Int_t nhist = fHistArray.size();
  Int_t nxy = fnx*fny;

  while(true){
    gSystem->ProcessEvents();

    std::cout<<"\r [s]:Start, [q]:Quit, [n]:Next, [b]:Back, [r]:Reset, [p]:Print >"
	     <<std::flush;

    system("stty raw"); 
    char input = getchar(); 
    system("stty cooked"); 

    if(input =='q'){
      std::cout<<std::endl;
      return -1;

    } else if (input =='n'){
      Draw();

    } else if (input =='b'){
      Int_t num = gPad->GetNumber();
      f_iplot = nhist - num - nxy;
      if (f_iplot<0){
	Int_t remain = nhist%nxy;
	if (remain==0) f_iplot = nhist - nxy;
	else           f_iplot = nhist - remain;
      }
      Draw();

    } else if (input =='r'){
      ResetHist();

    } else if (input =='p'){
      TString fname;
      std::cout<<std::endl<<"file name >"<<std::flush;
      std::cin>>fname;
      std::cout<<"   making "<<fname.Data()<<" ..."<<std::endl;
      Print(fname.Data());
      std::cout<<" Done"<<std::endl;

    } else if (input =='s'){
      std::cout<<std::endl;
      break;
    }

  }
  return 0;
}
//______________________________________________________________________________
