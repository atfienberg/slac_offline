#include "pulseFitter.hh"
#include "TTree.h"
#include <iostream>
#include "TApplication.h"
#include "TFile.h"


using namespace std;

int main(){
  new TApplication("app", 0, nullptr);
  TFile* datafile = new TFile("datafiles/exampleDatafile.root");
  cout << "?" << endl;
  TTree* tree = (TTree*) datafile->Get("t");
  float trace[1024];
  
  cout << "..." << endl;
  tree->SetBranchAddress("trace", trace);

  pulseFitter pf((char*)"configs/exampleSipmConfig.json");
  
  cout << "Huh? " << endl;

  for(int i = 0; i <tree->GetEntries(); ++i){
    tree->GetEntry(i);
    cout << i << endl;
    pf.fitSingle(trace);
  }

  delete tree;
  delete datafile;
}

  
  
  
