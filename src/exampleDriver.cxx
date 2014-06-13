// Aaron Fienberg
// fienberg@uw.edu
// example code for looping over devices from a config file
// and fitting them accordingly

//std includes
#include <iostream>
#include <time.h>
#include <vector>
#include <memory>
#include <cstdlib>

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
  double trace[50];
  double fitTrace[50];
  double energy;
  double sum;
  double baseline;
  double time;
  double ampl;
  double chi2;
  bool valid;
} fitResults;

typedef struct {
  unsigned long timestamp;
  unsigned short trace[8][1024];
  bool is_bad_event;
} sis;

typedef struct {
  string name;
  string digitizer;
  int channel;
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
  readRunConfig(devices, (char*)"runJsons/run00001.json");  
     
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
     
    if (tree.first == string("run_info")){
      cout << "Run number " << tree.second.get<int>("number") << endl;
     }
     
    else if (tree.first == string("devices")){
      
      //loop over devices
      for(const auto& subtree : tree.second){
	//check if it's a digitized device, if not skip to next one
	if(!subtree.second.get_child_optional("digi_type")){
	  continue;
	}

	deviceInfo thisDevice;
	thisDevice.name = subtree.first;
	thisDevice.digitizer = subtree.second.get<string>("digi_type");
	thisDevice.channel = subtree.second.get<int>("digi_channel");  
	thisDevice.channel = 0; //temp for fake datafile
	devInfo.push_back(thisDevice);
      
	cout << thisDevice.name << ": " <<
	  thisDevice.digitizer << " channel " <<
	  thisDevice.channel << endl;
      }//end loop over devices
    }//end else if
  }//end loop over config file trees
  
  //make sure there are no duplicate names
  for (unsigned int i = 0; i < devInfo.size(); ++i){
    for (unsigned int j = i + 1; j < devInfo.size(); ++j){
      if (devInfo[i].name == devInfo[j].name){
	cout << "Duplicate device name: " << devInfo[i].name << endl;
	exit(EXIT_FAILURE);
      }
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
    outTree.Branch(devices[i].name.c_str(),&fr[i],
     "trace[50]/D:fitTrace[50]/D:energy/D:sum/D:baseline/D:time/D:ampl/D:chi2/D:valid/O");

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
      //fill trace
      for( int k = 0; k < 50; ++k){
	fr[j].trace[k] = 
	  s.trace[devices[j].channel][fitters[j]->getFitStart()+k];
      }
	  
      //get summary information from the trace
      fr[j].sum = fitters[j]->getSum(s.trace[devices[j].channel],
				     fitters[j]->getFitStart(),
				     fitters[j]->getFitLength());

      fr[j].ampl = fitters[j]->getMax(fitters[j]->getFitStart(), 
				     fitters[j]->getFitLength());
      
      //do the fits
      if(fitters[j]->isFitConfigured()){
	fitters[j]->fitSingle(s.trace[devices[j].channel]);
    
	fr[j].energy = fitters[j]->getScale();
	fr[j].chi2 = fitters[j]->getChi2();
	fr[j].time = fitters[j]->getTime();
	fr[j].valid = fitters[j]->wasValidFit();
	
	//fill fitTrace
	fitters[j]->fillFitTrace(fr[j].fitTrace,
				 fitters[j]->getFitStart(),
				 50);
	
      }
      fr[j].baseline = fitters[j]->getBaseline();
      
    }
    
    outTree.Fill();
  }
}
