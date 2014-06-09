// Aaron Fienberg
// fienberg@uw.edu
// example code for looping over devices from a config file
// and fitting them accordingly

//std includes
#include <iostream>
#include <time.h>
#include <vector>

//ROOT includes
#include "TApplication.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"

//boost includes
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//project includes
#include "pulseFitter.hh"

using namespace std;

//define some datastructures
typedef struct {
    double energy;
    double chi2;
    double sum;
    double baseline;
    double time;
    double max;
    bool valid;
  } fitResults;

typedef struct{
    unsigned long timestamp;
    unsigned short trace[8][1024];
    bool is_bad_event;
  } sis;

typedef struct {
  string name;
  string digitizer;
  int channel;
  bool fit;
} deviceInfo;

int main(){
  new TApplication("app", 0, nullptr);

  //read in run config file
  boost::property_tree::ptree conf;
  read_json((char*)"runConfigs/exampleRunConfig.json", conf);

  //setup deviceInfo vector
  vector<deviceInfo> devices;
  for(const auto& tree : conf){
     
    if (tree.first == string("run_number")){
      cout << "Run number " << tree.second.get<int>("") << endl;
     }
     
    else {
      deviceInfo thisDevice;
      cout << tree.first << endl;
      thisDevice.name = tree.first;
      thisDevice.digitizer = tree.second.get<string>("digitizer");
      thisDevice.channel = tree.second.get<int>("channel");
      thisDevice.fit = tree.second.get<bool>("fit");
  
      devices.push_back(thisDevice);
      cout << "---> " << thisDevice.name << endl;
    }
  } 
     
  //read inputfile
  sis s;
  TFile datafile("datafiles/newExampleDatafile.root");
  TTree* tree = (TTree*) datafile.Get("t");
  tree->SetBranchAddress("sis", &s);
 
  //set up output file and output tree
  TFile outf("exampleOut.root", "recreate");
 
  vector<fitResults> fr(devices.size());
  
  TTree outTree("t","t");
  vector<pulseFitter*> fitters;
  cout << "size " << devices.size() << endl;
 
  for(unsigned int i = 0; i < devices.size(); ++i){
    outTree.Branch(devices[i].name.c_str(),&fr[i],"energy/D:chi2/D:sum/D:baseline/D:time/D:valid/O");

    //intialize fitters
    string config = string("configs/") + devices[i].name + string(".json");
    cout << config << endl;
    cout << devices[i].name << endl;
    fitters.push_back(new pulseFitter((char*)config.c_str()));
  }

  clock_t t1,t2;
  t1 = clock();
  for(unsigned int i = 0; i <tree->GetEntries(); ++i){
    tree->GetEntry(i);
    
    for(unsigned int j = 0; j < devices.size(); ++j){
    
      fr[j].sum = fitters[j]->getSum(s.trace[0],
				    fitters[j]->getFitStart(),
				    fitters[j]->getFitLength());

      fr[j].max = fitters[j]->getMax(fitters[j]->getFitStart(), 
				    fitters[j]->getFitLength());

      fitters[j]->fitSingle(s.trace[devices[j].channel]);
    
      fr[j].energy = fitters[j]->getScale();
      fr[j].chi2 = fitters[j]->getChi2();
      fr[j].baseline = fitters[j]->getBaseline();
      fr[j].time = fitters[j]->getTime();
      fr[j].valid = fitters[j]->wasValidFit();
    }

    outTree.Fill();
  }
  t2 = clock();

  float diff ((float)t2-(float)t1);
  cout << "Time elapsed: " << diff/CLOCKS_PER_SEC << "s" << endl;

  for (const auto& fitter : fitters){
    delete fitter;
  }

  outTree.Write();
  delete tree; 
  outf.Close();
  datafile.Close();

}

  
  
  
