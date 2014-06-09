// Aaron Fienberg
// fienberg@uw.edu
// example code for looping over devices from a config file
// and fitting them accordingly

//std includes
#include <iostream>
#include <time.h>
#include <vector>
#include <memory>

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

//read the run config file, store info in devInfo
void readRunConfig(vector<deviceInfo>& devInfo, char* runConfig);

//do the fits and fill the output tree
void crunch(const vector<deviceInfo>& devices, 
		   TTree* inTree, 
		   TTree& outTree);

int main(){
  new TApplication("app", 0, nullptr);
  
  //read in the run config file
  vector<deviceInfo> devices;
  readRunConfig(devices, (char*)"runConfigs/exampleRunConfig.json");  
     
  //read inputfile
  TFile datafile("datafiles/newExampleDatafile.root");
  TTree* inTree = (TTree*) datafile.Get("t");
 
  //set up output file and output tree
  TFile outf("exampleOut.root", "recreate");
  TTree outTree("t","t");
  
  //do the fits
  clock_t t1,t2;
  t1 = clock();
  crunch(devices, inTree, outTree);
  t2 = clock();

  float diff ((float)t2-(float)t1);
  cout << "Time elapsed: " << diff/CLOCKS_PER_SEC << "s" << endl;

  //write the data
  outTree.Write();
  delete inTree; 
  outf.Close();
  datafile.Close();

}

void readRunConfig(vector<deviceInfo>& devInfo, char* runConfig){
  //read in run config file
  boost::property_tree::ptree conf;
  read_json(runConfig, conf);

  //setup deviceInfo vector
  for(const auto& tree : conf){
     
    if (tree.first == string("run_number")){
      cout << "Run number " << tree.second.get<int>("") << endl;
     }
     
    else {
      deviceInfo thisDevice;
      thisDevice.name = tree.first;
      thisDevice.digitizer = tree.second.get<string>("digitizer");
      thisDevice.channel = tree.second.get<int>("channel");
      thisDevice.fit = tree.second.get<bool>("fit");
  
      devInfo.push_back(thisDevice);
      
      cout << thisDevice.name << ": " <<
	thisDevice.digitizer << " channel " <<
	thisDevice.channel << endl;
    }
  }
}
  
void crunch(const vector<deviceInfo>& devices, 
	    TTree* inTree, 
	    TTree& outTree){
 
  sis s;
  inTree->SetBranchAddress("sis", &s);
  
  vector<fitResults> fr(devices.size());
  vector< unique_ptr<pulseFitter> > fitters;
 
  //initialize the fitters and the output tree
  for(unsigned int i = 0; i < devices.size(); ++i){
    outTree.Branch(devices[i].name.c_str(),&fr[i],"energy/D:chi2/D:sum/D:baseline/D:time/D:valid/O");

    //intialize fitters
    string config = string("configs/") + devices[i].name + string(".json");
    fitters.push_back(unique_ptr< pulseFitter >
		      (new pulseFitter((char*)config.c_str())));
  }

  //loop over each event 
  for(unsigned int i = 0; i < inTree->GetEntries(); ++i){
    inTree->GetEntry(i);
    
    //loop over each device 
    for(unsigned int j = 0; j < devices.size(); ++j){
      //get summary information from the trace
      fr[j].sum = fitters[j]->getSum(s.trace[0],
				     fitters[j]->getFitStart(),
				     fitters[j]->getFitLength());

      fr[j].max = fitters[j]->getMax(fitters[j]->getFitStart(), 
				     fitters[j]->getFitLength());
      
      //do the fits
      if(devices[j].fit){
	fitters[j]->fitSingle(s.trace[devices[j].channel]);
    
	fr[j].energy = fitters[j]->getScale();
	fr[j].chi2 = fitters[j]->getChi2();
	fr[j].baseline = fitters[j]->getBaseline();
	fr[j].time = fitters[j]->getTime();
	fr[j].valid = fitters[j]->wasValidFit();
      }
    }
    
    outTree.Fill();
  }

}
