#include "pulseFitter.hh"
#include "TTree.h"
#include <iostream>
#include "TApplication.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TFile.h"


using namespace std;

int main(){
  new TApplication("app", 0, nullptr);
  TFile* datafile = new TFile("datafiles/exampleDatafile.root");
  
  TTree* tree = (TTree*) datafile->Get("t");
  float trace[1024];
  

  tree->SetBranchAddress("trace", trace);

  pulseFitter pf((char*)"configs/exampleSipmConfig.json");
  
  TH1F* scaleHist = new TH1F("scaleHist", "scale", 100,0.0,0.0);
  for(int i = 0; i <tree->GetEntries(); ++i){
    tree->GetEntry(i);
    pf.fitSingle(trace);
    scaleHist->Fill(pf.getScale());
  }
  TCanvas c("c1");
  scaleHist->Draw();
  c.Draw();
  c.Modified();
  c.Update();
  cin.ignore();

  delete tree;
  delete scaleHist;
  delete datafile;
}

  
  
  
