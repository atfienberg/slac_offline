// Aaron Fienberg
// fienberg@uw.edu
// example code for looping over devices from a config file
// and fitting them accordingly

//std includes
#include <iostream>
#include <time.h>
#include <vector>
#include <memory>
#include <algorithm>
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

#define TRACELENGTH 1024

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
  string moduleType;
  int moduleNum;
  int channel;
  bool neg;
} deviceInfo;

typedef struct{
  vector<deviceInfo> struckInfo;
  vector<deviceInfo> struckSInfo;
  vector<deviceInfo> adcInfo;
} runInfo;

//temp placeholders
typedef struct{
  double aAmpl;
} struckSResults;
typedef int adcResults;

typedef struct {
  unsigned long system_clock;
  unsigned long device_clock[4];
  unsigned short trace[4][1024];
} sis_fast;

typedef struct {
  unsigned long system_clock;
  unsigned long device_clock[8];
  unsigned short trace[8][1024];
} sis_slow;

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
void crunchStruck(vector<sis_fast>& sFast, 
		  const vector<deviceInfo>& devices,
		  vector<struckResults>& sr,
		  vector< unique_ptr<pulseFitter> >& sFitters); 

//init slow struck
void initStruckS(TTree& outTree, 
		 const vector<deviceInfo>& devices,
		 vector<struckSResults>& srSlow,
		 vector< unique_ptr<pulseFitter> >& slFitters); 

//crunch slow struck, includes beam and laser flags
void crunchStruckS(sis_slow& s,
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
  if( !exists(argv[2]) ){
    cout << argv[2] << " is an invalid file." << endl;
    exit(EXIT_FAILURE);
  }
  runInfo rInfo;
  readRunConfig(rInfo, argv[2]);  
     
  //read inputfile
  if( !exists(argv[1]) ){
    cout << argv[1] << " is an invalid file." << endl;
    exit(EXIT_FAILURE);
  }
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
	thisDevice.moduleType = subtree.second.get<string>("module");
	thisDevice.channel = subtree.second.get<int>("channel");  
	if(subtree.second.get_child_optional("polarity")){
	  thisDevice.neg = true;
	}
	else{
	  thisDevice.neg = false;
	}
	if (thisDevice.moduleType == string("struck")){
	  thisDevice.moduleNum = subtree.second.get<int>("module_num");
	  rInfo.struckInfo.push_back(thisDevice);
	}
	else if (thisDevice.moduleType == string("struckS")){
	  rInfo.struckSInfo.push_back(thisDevice);
	}
	else if (thisDevice.moduleType == string("adc")){
	  rInfo.adcInfo.push_back(thisDevice);
	}
	else{
	  cerr << "Module type " << thisDevice.moduleType
	       << " not recognize." << endl;
	  exit(EXIT_FAILURE);
	}
	 
	cout << thisDevice.name << ": " <<
	  thisDevice.moduleType << thisDevice.moduleNum <<
	  " channel " <<
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
 
  vector<sis_fast> sFast(2);
  inTree->SetBranchAddress("sis_fast_0", &sFast[0]);
  inTree->SetBranchAddress("sis_fast_1", &sFast[1]);

  //initialization routines
  vector<struckResults> sr(rInfo.struckInfo.size());
  vector< unique_ptr<pulseFitter> > sFitters;
  initStruck(outTree, rInfo.struckInfo, sr, sFitters);
  
  sis_slow s;
  inTree->SetBranchAddress("sis_slow_0",&s);
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
    crunchStruck(sFast, rInfo.struckInfo, sr, sFitters);
    crunchAdc(&temp, rInfo.adcInfo, ar); 
    if( i % 100 == 0){
      cout << i  << " processed" << endl;
    }
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

void crunchStruck(vector<sis_fast>& sFast, 
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
	sFast[devices[j].moduleNum].trace[devices[j].channel]
	[sFitters[2*j+laserRun]->getFitStart()+k];
    }
     
    //get summary information from the trace
    int maxdex;
    if(!devices[j].neg){
      maxdex = max_element(sFast[devices[j].moduleNum].trace[devices[j].channel],
			   sFast[devices[j].moduleNum].trace[devices[j].channel]+
			   TRACELENGTH) - 
	sFast[devices[j].moduleNum].trace[devices[j].channel];
    }
    else{
      maxdex = min_element(sFast[devices[j].moduleNum].trace[devices[j].channel],
			   sFast[devices[j].moduleNum].trace[devices[j].channel]+
			   TRACELENGTH) - 
	sFast[devices[j].moduleNum].trace[devices[j].channel];
    }
    //template
    if(sFitters[2*j+laserRun]->getFitType() == string("template")){
      sFitters[2*j+laserRun]->setFitStart(maxdex-
					  sFitters[2*j+laserRun]->getFitLength()/2);
    }
    
    //parametric
    else{
      sFitters[2*j+laserRun]->setFitStart(maxdex-
					  sFitters[2*j+laserRun]->getFitLength()+2);
    }
						       			       
    sr[j].aSum = sFitters[2*j+laserRun]->
      getSum(sFast[devices[j].moduleNum].trace[devices[j].channel],
	     sFitters[2*j+laserRun]->getFitStart(),
	     sFitters[2*j+laserRun]->getFitLength());
    
    sr[j].aAmpl = sFast[devices[j].moduleNum].trace[devices[j].channel][maxdex]-
      sFitters[2*j+laserRun]->getBaseline();

    // sr[j].aAmpl = sFitters[2*j+laserRun]->
    //   getMax(sFitters[2*j+laserRun]->getFitStart(), 
    // 	     sFitters[2*j+laserRun]->getFitLength());
      
    //set fit config based on maxdex
    //template
    if(sFitters[2*j+laserRun]->getFitType() == string("template")){
      sFitters[2*j+laserRun]->setParameterGuess(0,maxdex);
      sFitters[2*j+laserRun]->setParameterMin(0,maxdex-1);
      sFitters[2*j+laserRun]->setParameterMax(0,maxdex+1);
    }
      
    //parametric
    else {
      sFitters[2*j+laserRun]->setParameterGuess(0,maxdex-2);
      sFitters[2*j+laserRun]->setParameterMin(0,maxdex-3);
      sFitters[2*j+laserRun]->setParameterMax(0,maxdex-1);
    }
    //do the fits
    if(sFitters[2*j+laserRun]->isFitConfigured()){
      sFitters[2*j+laserRun]->fitSingle(sFast[devices[j].moduleNum].trace[devices[j].channel]);
    
      if(sFitters[2*j+laserRun]->getFitType() == string("template")){
	sr[j].energy = sFitters[2*j+laserRun]->getScale();
      }
      else if(sFitters[2*j+laserRun]->getFitType() == string("laser")){
	sr[j].energy = 2.0*sFitters[2*j+laserRun]->getScale();
      }
      sr[j].chi2 = sFitters[2*j+laserRun]->getChi2();
      sr[j].time = sFitters[2*j+laserRun]->getTime();
      sr[j].valid = sFitters[2*j+laserRun]->wasValidFit();
	
      //fill fitTrace
      sFitters[2*j+laserRun]->fillFitTrace(sr[j].fitTrace,
			       sFitters[2*j+laserRun]->getFitStart(),
			       CHOPPED_TRACE_LENGTH);
       
    }
    if (sr[j].energy<0){
      sr[j].energy=-1.0*sr[j].energy;
    }
    sr[j].baseline = sFitters[2*j+laserRun]->getBaseline();      
  }
}

void initStruckS(TTree& outTree, 
		 const vector<deviceInfo>& devices,
		 vector<struckSResults>& srSlow,
		 vector< unique_ptr<pulseFitter> >& slFitters){
  
  for (unsigned int i = 0; i < devices.size(); ++i){
    outTree.Branch(devices[i].name.c_str(), &srSlow[i], "aAmpl/D");
  }
}

//completely temporary implementation
void crunchStruckS(sis_slow& s,
		   const vector<deviceInfo>& devices,
		   vector<struckSResults>& srSlow,
		   vector< unique_ptr<pulseFitter> >& slFitters){
  if(devices.size()>0){
    double baseline = accumulate(s.trace[4],s.trace[4]+20,0)/20;
    double ampl = *max_element(s.trace[4],s.trace[4]+TRACELENGTH)-baseline;
    srSlow[0].aAmpl = ampl;
  }
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
