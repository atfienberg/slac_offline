#include "pulseFitter.hh"
#include "TTree.h"
#include <iostream>
#include "TApplication.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TSystem.h"

using namespace std;
const int DEFAULTSTRUCKCHANNEL = 6;

struct s_sis{
  unsigned long long timestamp[8];
  unsigned short trace[8][0x400];
  bool is_bad_event;
};

int main(int argc, char* argv[]){
  if(argc<2){
    cout << "need to input datafile." << endl;
    return -1;
  }
  
  int struckChannel = DEFAULTSTRUCKCHANNEL;
  if(argc == 3){
    struckChannel = atoi(argv[2]);
  }


  new TApplication("app", 0, nullptr);
  
  typedef struct {
    double energy;
    double chi2;
    double sum;
    double baseline;
  } fitResults;
  
  gSystem->Load("libTree");
  TFile infile(argv[1]);
  TTree* t = (TTree*) infile.Get("t");
  struct s_sis s;
  t->SetBranchAddress("sis", &s);

  fitResults fr;
  pulseFitter pf((char*)"configs/fuzzyConfig.json",true);

  TFile* outf = new TFile("outfile.root","recreate");
  TTree* outTree = new TTree("t","t");
  outTree->Branch("fitResults",&fr,"energy/D:chi2/D:sum/D:baseline/D");
  
  

  for(int i = 0; i <t->GetEntries(); ++i){
    t->GetEntry(i);
    pf.fitSingle(s.trace[struckChannel]);
    fr.energy = pf.getScale();
    fr.baseline = pf.getBaseline();
    fr.chi2 = pf.getChi2();
    fr.sum = -1*pf.getSum(s.trace[struckChannel],150,100);
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
  delete t;
}

  
  
  
