//This template maker rebins the traces by 2 before putting thme into the master template, seems to behave better with the drs


#include "TMath.h"
#include "TH1.h"
#include "TTree.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TString.h"
#include "TVector.h"
#include "TGraph.h"
#include "TSpline.h"
#include <iostream>
using namespace TMath;
using namespace std;

//returns pseudotime of trace
double getPseudoTime(unsigned short* trace);

void correctBaseline(unsigned short* trace, float* correctedTrace);

const int windowLength = 150;
void makeTemplateRebin(){
  const int nBinsPseudoTime = 2000;
  const int nTimeBins = 1;

  TFile* trFile = new TFile("../datafiles/nitrogen_laser_calibration_filter1.root");
  TTree* struckTree = (TTree*) trFile->Get("WFDTree");
  TBranch* branch = struckTree->GetBranch("Channel1");
  unsigned short ch1[1024];
  branch->SetAddress(&ch1);
  cout << branch -> GetEntries() << " pulses." << endl;
  
  TH1F* pseudoTimeHist = new TH1F("ptimes","Pseudo Times",nBinsPseudoTime,0,1);
  double* pseudoTimes = new double[branch->GetEntries()];
  for(int i = 0; i < branch->GetEntries(); ++i){
    branch->GetEntry(i);
    pseudoTimes[i] = getPseudoTime(ch1);
    pseudoTimeHist->Fill(pseudoTimes[i]);
  }
  
  pseudoTimeHist->Scale(1/pseudoTimeHist->Integral());
  pseudoTimeHist->Draw();
  
  new TCanvas();
  TGraph* realTimes = new TGraph();
  for(int i = 0; i <nBinsPseudoTime; ++i){
    realTimes->SetPoint(i,(i+1.0)/nBinsPseudoTime, pseudoTimeHist->Integral(1,i+2));
  }
  //realTimes->Draw("ap");
  TSpline3* spline = new TSpline3("spline", realTimes);
  spline->Draw();
  cout << spline->Eval(.99) << endl;;

  new TCanvas();
  TH1F* realTimeHist = new TH1F("rhist","Real Time Histogram",nTimeBins,0,1);
  for(int i = 0; i<branch->GetEntries(); ++i){
    realTimeHist->Fill(spline->Eval(pseudoTimes[i]));
  }
  realTimeHist->Scale(branch->GetEntries());
  double rTimeMax = realTimeHist->GetMaximum();
  realTimeHist->GetYaxis()->SetRangeUser(0,rTimeMax*1.1);
  realTimeHist->Draw();
  
  //template pieces
  TH1F* timeSlices[nTimeBins];
  for(int i = 0; i <nTimeBins; ++i){
    timeSlices[i] = new TH1F(Form("Slice%i", i), Form("Slice %i",i),windowLength,0,windowLength);
  }
  
  //fill the pieces
  float correctedTrace[1024];
  for(int i = 0; i<branch->GetEntries(); ++i){
    double trueTime = spline->Eval(pseudoTimes[i]);
    branch->GetEntry(i);
    correctBaseline(ch1, correctedTrace);
    for(int k=0; k<nTimeBins; ++k){
      if((trueTime<=(k+1)*1.0/nTimeBins)&&(trueTime>(k)*1.0/nTimeBins)){
	for(int j = 0; j < windowLength; ++j){
	  timeSlices[k]->AddBinContent(j+1,correctedTrace[j]);
	}  
     }
    }
  }
  
  for(int i = 0; i < nTimeBins; ++i){
    new TCanvas();
    timeSlices[i]->Scale(1.0/timeSlices[i]->Integral());
    timeSlices[i]->Rebin(2);
    timeSlices[i]->Draw();
  }
  
  TH1F* masterTemplate = new TH1F("master", "Master Template", windowLength*nTimeBins/2,0,windowLength);
  for(int i = 0; i <nTimeBins; ++i){
    for(int j = 0; j<windowLength/2; ++j){
      masterTemplate->SetBinContent(nTimeBins*j+nTimeBins-i,timeSlices[i]->GetBinContent(j+1));
    }
  }
  new TCanvas();
  masterTemplate->Draw();
  TSpline3* masterSpline = new TSpline3(masterTemplate);
  masterSpline->SetName("masterSpline");
  TFile* outf = new TFile("template.root","recreate");
  new TCanvas();
  masterSpline->Draw();
  masterSpline->Write();
  masterTemplate->Write();
  spline->Write();
  outf->Write();
  outf->Close();
  delete pseudoTimes;
}

double getPseudoTime(unsigned short* trace){
  TH1F* pulse = new TH1F("pulse","pulse",1024,0,1024);
  for(int i = 0; i < 1024; ++i){
    pulse->Fill(i,static_cast<float>(trace[i]));
  }
  pulse->Scale(-1);
  // pulse->Rebin(2);
  pulse->Draw();
  int maxBin = pulse->GetMaximumBin();
  double argument = (pulse->GetBinContent(maxBin)-pulse->GetBinContent(maxBin-1))/
    (pulse->GetBinContent(maxBin)-pulse->GetBinContent(maxBin+1));
  delete pulse;
  return 2/Pi()*ATan(argument);
}
  
void correctBaseline(unsigned short* trace, float* correctedTrace){
  for (unsigned int i = 0; i < 1024; ++i){
    correctedTrace[i] = static_cast<float>(trace[i]);
  }
  TH1F* trh = new TH1F(TVectorF(1024, correctedTrace));

  trh->Scale(-1);

  trh->GetXaxis()->SetRange(10,1000);
  int maxBin = trh->GetMaximumBin();
  int fit_end = maxBin-40;
  int fitLength = 100;
  trh->Fit("pol0", "sq0W","",fit_end-fitLength,fit_end);
  double maximum = trh->GetBinContent(maxBin)-trh->GetFunction("pol0")->GetParameter(0);


  for(unsigned int i = 0; i < windowLength; ++i){
    correctedTrace[i] = (-1.0*correctedTrace[maxBin-40+i]-trh->GetFunction("pol0")->GetParameter(0))/maximum;  
  }
  delete trh;
}

