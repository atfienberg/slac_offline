#include "pulseFitter.hh"
#include "TTree.h"
#include <iostream>
#include "TApplication.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TFile.h"


using namespace std;

int main(int argc, char* argv[]){
  if(argc<3){
    cout << "need datafile and output file" << endl;
    return -1;
  }

  new TApplication("app", 0, nullptr);
  pulseFitter pf((char*)"configs/templateConfig.json");
  //pulseFitter pf((char*)"configs/kazconfig.json");
  TFile* datafile = new TFile(argv[1]);
  TFile* outf = new TFile(Form("%s.root",argv[2]),"recreate");
  TTree* tree = (TTree*) datafile->Get("WFDTree");
  unsigned short trace[1024];

  tree->SetBranchAddress("Channel1", &trace);

  
  typedef struct {
    double energy;
    double chi2;
    double sum;
    double baseline;
    double time;
    bool valid;
  } fitResults;
  
  fitResults fr;

  TTree* outTree = new TTree("t","t");
  outTree->Branch("fitResults",&fr,"energy/D:chi2/D:sum/D:baseline/D");

  
  for(int i = 0; i <tree->GetEntries(); ++i){
    tree->GetEntry(i);
    pf.fitSingle(trace);
    fr.energy = -2*pf.getScale();
    fr.baseline = pf.getBaseline();
    fr.chi2 = pf.getChi2();
    fr.sum = -1*pf.getSum(trace,90,80);
    if(i%1000==0){
      cout << i << " done" << endl;
    }
    if(pf.wasValidFit()){
      outTree->Fill();
    }
  }
  cout << outTree->GetEntries() << " successful fits" << endl;
  outTree->Write();
  outf->Write();
  
  delete outTree;
  delete outf;
  delete tree;
  delete datafile;
}

  
  
  
