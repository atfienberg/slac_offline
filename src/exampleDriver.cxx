#include "pulseFitter.hh"
#include "TTree.h"
#include <iostream>
#include "TApplication.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TFile.h"
#include <time.h>

using namespace std;

int main(){
  new TApplication("app", 0, nullptr);

  //read inputfile
  TFile datafile("datafiles/clippedPulses.root");
  TTree* tree = (TTree*) datafile.Get("t");
  float trace[1024];
  tree->SetBranchAddress("trace", trace);

  pulseFitter pf((char*)"configs/exampleSipmConfig.json");
  TFile outf("sampleOutputFile.root","recreate");

  typedef struct {
    double energy;
    double chi2;
    double sum;
    double baseline;
    double time;
    bool valid;
  } fitResults;
  
  fitResults fr;

  TTree outTree("t","t");
  outTree.Branch("fitResults",&fr,"energy/D:chi2/D:sum/D:baseline/D:time/D:valid/O");

  clock_t t1,t2;
  t1 = clock();
  for(int i = 0; i <tree->GetEntries(); ++i){
    tree->GetEntry(i);

    fr.sum = pf.getSum(trace, 390, 100);
    
    pf.fitSingle(trace);
    
    fr.energy = pf.getScale();
    fr.chi2 = pf.getChi2();
    fr.baseline = pf.getBaseline();
    fr.time = pf.getTime();
    fr.valid = pf.wasValidFit();
    
    outTree.Fill();
  }
  t2 = clock();

  float diff ((float)t2-(float)t1);
  cout << "Time elapsed: " << diff/CLOCKS_PER_SEC << "s" << endl;

  outTree.Write();
  delete tree; 
  outf.Close();
  datafile.Close();

}

  
  
  
