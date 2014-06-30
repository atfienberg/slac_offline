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
} struckResults;

typedef struct {
  string name;
  string module;
  int channel;
} deviceInfo;

typedef struct{
  vector<deviceInfo> struckInfo;
  vector<deviceInfo> struckSInfo;
  vector<deviceInfo> adcInfo;
} runInfo;

//temp placeholders
typedef struct{
  bool q;
} struckSResults;
typedef int adcResults;

typedef struct {
  unsigned long timestamp;
  unsigned short trace[8][1024];
  bool is_bad_event;
} struck;


//read the run config file, store info in devInfo
void readRunConfig(runInfo& rInfo, char* runConfig);

//do the fits and fill the output tree
void crunch(const runInfo& rInfo, 
		   TTree* inTree, 
		   TTree& outTree);

//init for struck devices
void initStruck(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<struckResults>& sr,
		vector< unique_ptr<pulseFitter> >& sFitters);

//crunch through struck devices for a given event
void crunchStruck(struck& s, 
		  const vector<deviceInfo>& devices,
		  vector<struckResults>& sr,
		  vector< unique_ptr<pulseFitter> >& sFitters); 

//init slow struck
void initStruckS(TTree& outTree, 
		 const vector<deviceInfo>& devices,
		 vector<struckSResults>& srSlow,
		 vector< unique_ptr<pulseFitter> >& slFitters); 

//crunch slow struck, includes beam and laser flags
void crunchStruckS(struck& s,
		   const vector<deviceInfo>& devices,
		   vector<struckSResults>& srSlow,
		   vector< unique_ptr<pulseFitter> >& slFitters); 

//init adc
void initAdc(TTree& outTree,
	     const vector<deviceInfo>& devices,
	     vector<adcResults>& ar);

//crunch adc
void crunchAdc(const unsigned short* adc,
	       const vector<deviceInfo>& devices,
	       vector<adcResults>& ar);

//check if a file exists
bool exists(const string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

int main(int argc, char* argv[]) {
  new TApplication("app", 0, nullptr);
  
  if (argc<4){
    cout << "usage: ./slacAnalyzer [inputfile] [configfile] [outputfile]"
	 << endl;
    exit(EXIT_FAILURE);
  }

  //read in the run config file
  runInfo rInfo;
  readRunConfig(rInfo, argv[2]);  
     
  //read inputfile
  TFile datafile(argv[1]);
  TTree* inTree = (TTree*) datafile.Get("t");
 
  //set up output file and output tree
  TFile outf(argv[3], "recreate");
  TTree outTree("t","t");
  
  //do the fits
  clock_t t1,t2;
  t1 = clock();
  crunch(rInfo, inTree, outTree);
  t2 = clock();

  float diff ((float)t2-(float)t1);
  cout << "Time elapsed: " << diff/CLOCKS_PER_SEC << "s" << endl;

  //write the data
  outf.Write();
  delete inTree; 
  outf.Close();
  datafile.Close();

}

void checkDuplicates(const vector<deviceInfo>& devInfo){
   for (unsigned int i = 0; i < devInfo.size(); ++i){
    for (unsigned int j = i + 1; j < devInfo.size(); ++j){
      if (devInfo[i].name == devInfo[j].name){
	cerr << "Duplicate device name: " << devInfo[i].name << endl;
	exit(EXIT_FAILURE);
      }
    }
  }
}

void readRunConfig(runInfo& rInfo, char* runConfig){
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
	if(!subtree.second.get_child_optional("module")){
	  continue;
	}

	deviceInfo thisDevice;
	thisDevice.name = subtree.first;
	thisDevice.module = subtree.second.get<string>("module");
	thisDevice.channel = subtree.second.get<int>("channel");  
	thisDevice.channel = 0; //temp for fake datafile
	if (thisDevice.module == string("struck")){
	  rInfo.struckInfo.push_back(thisDevice);
	}
	else if (thisDevice.module == string("struckS")){
	  rInfo.struckSInfo.push_back(thisDevice);
	}
	else if (thisDevice.module == string("adc")){
	  rInfo.adcInfo.push_back(thisDevice);
	}
	else{
	  cerr << "Module type " << thisDevice.module
	       << " not recognize." << endl;
	  exit(EXIT_FAILURE);
	}
	 
	cout << thisDevice.name << ": " <<
	  thisDevice.module << " channel " <<
	  thisDevice.channel << endl;
      }//end loop over devices
    }//end else if
  }//end loop over config file trees
  
  //make sure there are no duplicate names
  checkDuplicates(rInfo.struckInfo);
  checkDuplicates(rInfo.struckSInfo);
  checkDuplicates(rInfo.adcInfo);
}
  
void crunch(const runInfo& rInfo, 
 	    TTree* inTree, 
	    TTree& outTree){
 
  struck s;
  inTree->SetBranchAddress("sis", &s);
  
  //initialization routines
  vector<struckResults> sr(rInfo.struckInfo.size());
  vector< unique_ptr<pulseFitter> > sFitters;
  initStruck(outTree, rInfo.struckInfo, sr, sFitters);
  
  vector<struckSResults> srSlow(rInfo.struckSInfo.size());
  vector< unique_ptr<pulseFitter> > slFitters;
  initStruckS(outTree, rInfo.struckSInfo, srSlow, slFitters);
  
  vector<adcResults> ar;
  unsigned short temp;
  initAdc(outTree, rInfo.adcInfo, ar);
  
  
  //loop over each event 
  for(unsigned int i = 0; i < inTree->GetEntries(); ++i){
    inTree->GetEntry(i);
    crunchStruckS(s, rInfo.struckSInfo, srSlow, slFitters);
    crunchStruck(s, rInfo.struckInfo, sr, sFitters);
    crunchAdc(&temp, rInfo.adcInfo, ar); 
    outTree.Fill();
  }
}

void initStruck(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<struckResults>& sr,
		vector< unique_ptr<pulseFitter> >& sFitters){
  //initialize output tree branches for struck devices
  for(unsigned int i = 0; i < devices.size(); ++i){
    outTree.Branch(devices[i].name.c_str(),&sr[i],
      Form("trace[%i]/D:fitTrace[%i]/D:energy/D:aSum/D:baseline/D:time/D:aAmpl/D:chi2/D:valid/O",
	   CHOPPED_TRACE_LENGTH, CHOPPED_TRACE_LENGTH));    
  }

  //intialize struck fitters
  for(unsigned int i = 0; i < devices.size(); ++i){
    string config = string("configs/") + devices[i].name + string(".json");

    //beam configs
    if (exists(config)){
      sFitters.push_back(unique_ptr< pulseFitter >
			(new pulseFitter((char*)config.c_str())));
    } 
    else{
      cout << config << " not found. "
	   << "Using default config." << endl;
      sFitters.push_back(unique_ptr< pulseFitter >
			(new pulseFitter()));
    }
    
    //laser configs
    config = string("configs/") + devices[i].name + string("Laser.json");
    if (exists(config)){
      sFitters.push_back(unique_ptr< pulseFitter >(new pulseFitter((char*)config.c_str())));
    }
    else{
      cout << config << " not found. "
	   << "Using default laser config." << endl;
      sFitters.push_back(unique_ptr< pulseFitter >
			 (new pulseFitter((char*)"configs/.defaultLaserConfig.json")));
    } 
  }
}

void crunchStruck(struck& s, 
		  const vector<deviceInfo>& devices,
		  vector<struckResults>& sr,
		  vector< unique_ptr<pulseFitter> >& sFitters){
  
  //temp
  int laserRun = 0;

  //loop over each device 
  for(unsigned int j = 0; j < devices.size(); ++j){
    //fill trace
    for( int k = 0; k < CHOPPED_TRACE_LENGTH; ++k){
      sr[j].trace[k] = 
	s.trace[devices[j].channel][sFitters[2*j+laserRun]->getFitStart()+k];
    }
     
    //get summary information srom the trace
    sr[j].aSum = sFitters[2*j+laserRun]->getSum(s.trace[devices[j].channel],
				    sFitters[2*j+laserRun]->getFitStart(),
				    sFitters[2*j+laserRun]->getFitLength());

    sr[j].aAmpl = sFitters[2*j+laserRun]->
      getMax(sFitters[2*j+laserRun]->getFitStart(), 
	     sFitters[2*j+laserRun]->getFitLength());
      
    //do the fits
    if(sFitters[2*j+laserRun]->isFitConfigured()){
      sFitters[2*j+laserRun]->fitSingle(s.trace[devices[j].channel]);
    
      sr[j].energy = sFitters[2*j+laserRun]->getScale();
      sr[j].chi2 = sFitters[2*j+laserRun]->getChi2();
      sr[j].time = sFitters[2*j+laserRun]->getTime();
      sr[j].valid = sFitters[2*j+laserRun]->wasValidFit();
	
      //fill fitTrace
      sFitters[2*j+laserRun]->fillFitTrace(sr[j].fitTrace,
			       sFitters[2*j+laserRun]->getFitStart(),
			       CHOPPED_TRACE_LENGTH);
       
    }
    sr[j].baseline = sFitters[2*j+laserRun]->getBaseline();      
  }
}

void initStruckS(TTree& outTree, 
		 const vector<deviceInfo>& devices,
		 vector<struckSResults>& srSlow,
		 vector< unique_ptr<pulseFitter> >& slFitters){
  
  for (unsigned int i = 0; i < devices.size(); ++i){
    outTree.Branch(devices[i].name.c_str(), &srSlow[i], Form("%s/O",devices[i].name.c_str()));
  }
}

//completely temporary implementation
void crunchStruckS(struck& s,
		   const vector<deviceInfo>& devices,
		   vector<struckSResults>& srSlow,
		   vector< unique_ptr<pulseFitter> >& slFitters){
 
  srSlow[0].q = static_cast<bool>(clock()%2);
  srSlow[1].q = !srSlow[0].q;
}
  

void initAdc(TTree& outTree,
	     const vector<deviceInfo>& devices,
	     vector<adcResults>& ar){
  //stub
}

void crunchAdc(const unsigned short* adc,
	       const vector<deviceInfo>& devices,
	       vector<adcResults>& ar){
  //stub
}
