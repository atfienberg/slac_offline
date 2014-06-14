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
#include <string>

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

const int CHOPPED_TRACE_LENGTH = 50;

//define some datastructures
typedef struct {
  double trace[CHOPPED_TRACE_LENGTH];
  double fitTrace[CHOPPED_TRACE_LENGTH];
  double energy;
  double aSum;
  double baseline;
  double time;
  double aAmpl;
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

//init for struck devices
void initStruck(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& fr,
		vector< unique_ptr<pulseFitter> >& fitters);

//crunch through struck devices for a given event
void crunchStruck(sis& s, 
		  const vector<deviceInfo>& devices,
		  vector<fitResults>& fr,
		  vector< unique_ptr<pulseFitter> >& fitters); 

//check if a file exists
bool exists(const string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

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
  
  //initialization routines
  vector<fitResults> fr(devices.size());
  vector< unique_ptr<pulseFitter> > fitters;
  initStruck(outTree, devices, fr, fitters);

  //loop over each event 
  for(unsigned int i = 0; i < inTree->GetEntries(); ++i){
    inTree->GetEntry(i);
    crunchStruck(s, devices, fr, fitters);
    outTree.Fill();
  }
}

void initStruck(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& fr,
		vector< unique_ptr<pulseFitter> >& fitters){
  //initialize output tree branches for struck devices
  for(unsigned int i = 0; i < devices.size(); ++i){
    outTree.Branch(devices[i].name.c_str(),&fr[i],
      Form("trace[%i]/D:fitTrace[%i]/D:energy/D:aSum/D:baseline/D:time/D:aAmpl/D:chi2/D:valid/O",
	   CHOPPED_TRACE_LENGTH, CHOPPED_TRACE_LENGTH));    
  }

  //intialize struck fitters
  for(unsigned int i = 0; i < devices.size(); ++i){
    string config = string("configs/") + devices[i].name + string(".json");

    if(exists(config)){
      fitters.push_back(unique_ptr< pulseFitter >
			(new pulseFitter((char*)config.c_str())));
    } 

    else{
      cout << config << " not found. "
	   << "Using default config." << endl;
      fitters.push_back(unique_ptr< pulseFitter >
			(new pulseFitter()));
    }
  }

}

void crunchStruck(sis& s, 
		  const vector<deviceInfo>& devices,
		  vector<fitResults>& fr,
		  vector< unique_ptr<pulseFitter> >& fitters){
  
    
  //loop over each device 
  for(unsigned int j = 0; j < devices.size(); ++j){
    //fill trace
    for( int k = 0; k < CHOPPED_TRACE_LENGTH; ++k){
      fr[j].trace[k] = 
	s.trace[devices[j].channel][fitters[j]->getFitStart()+k];
    }
     
    //get summary information from the trace
    fr[j].aSum = fitters[j]->getSum(s.trace[devices[j].channel],
				    fitters[j]->getFitStart(),
				    fitters[j]->getFitLength());

    fr[j].aAmpl = fitters[j]->getMax(fitters[j]->getFitStart(), 
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
			       CHOPPED_TRACE_LENGTH);
       
    }
    fr[j].baseline = fitters[j]->getBaseline();      
  }
}
