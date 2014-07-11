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
} fitResults;

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
  vector<deviceInfo> drsInfo;
} runInfo;

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

typedef struct {
  unsigned long system_clock;
  unsigned long device_clock[16];
  unsigned short trace[16][1024];
} drs;

//read the run config file, store info in devInfo
void readRunConfig(runInfo& rInfo, char* runConfig);

//do the fits and fill the output tree
void crunch(const runInfo& rInfo, 
		   TTree* inTree, 
		   TTree& outTree);

//init for struck devices
void initStruck(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& sr,
		vector< unique_ptr<pulseFitter> >& sFitters);

//crunch through struck devices for a given event
void crunchStruck(vector<sis_fast>& sFast, 
		  const vector<deviceInfo>& devices,
		  vector<fitResults>& sr,
		  vector< unique_ptr<pulseFitter> >& sFitters); 

//init for drs devices
void initDRS(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& drsR,
		vector< unique_ptr<pulseFitter> >& drsFitters);

//crunch through drs devices for a given event
void crunchDRS(vector<drs>& drs, 
		  const vector<deviceInfo>& devices,
		  vector<fitResults>& drsR,
		  vector< unique_ptr<pulseFitter> >& drsFitters); 

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
	thisDevice.moduleNum = subtree.second.get<int>("module_num");
	if (thisDevice.moduleType == string("struck")) {
	  rInfo.struckInfo.push_back(thisDevice);
	}
	else if (thisDevice.moduleType == string("drs")){
	  rInfo.drsInfo.push_back(thisDevice);
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
 
  //initialization routines

  vector<sis_fast> sFast(2);
  inTree->SetBranchAddress("sis_fast_0", &sFast[0]);
  inTree->SetBranchAddress("sis_fast_1", &sFast[1]);
  vector<fitResults> sr(rInfo.struckInfo.size());
  vector< unique_ptr<pulseFitter> > sFitters;
  initStruck(outTree, rInfo.struckInfo, sr, sFitters);

  vector<drs> drs(2);
  inTree->SetBranchAddress("caen_drs_0", &drs[0]);
  inTree->SetBranchAddress("caen_drs_1", &drs[1]);
  vector<fitResults> drsR(rInfo.drsInfo.size());
  vector< unique_ptr<pulseFitter> > drsFitters;
  initDRS(outTree, rInfo.drsInfo, drsR, drsFitters);

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
    crunchDRS(drs, rInfo.drsInfo, drsR, drsFitters);
    crunchAdc(&temp, rInfo.adcInfo, ar); 
    if( i % 100 == 0){
      cout << i  << " processed" << endl;
    }
    outTree.Fill();
  }
}

void initTraceDevice(TTree& outTree, 
		     const deviceInfo& device,
		     fitResults* fr,
		     vector< unique_ptr<pulseFitter> >& fitters){
  
  //initialize output tree branch for this device
  outTree.Branch(device.name.c_str(), fr,
      Form("trace[%i]/D:fitTrace[%i]/D:energy/D:aSum/D:baseline/D:time/D:aAmpl/D:chi2/D:valid/O",
	   CHOPPED_TRACE_LENGTH, CHOPPED_TRACE_LENGTH));    
 
  //intitalize fitter
  string config = string("configs/") + device.name + string(".json");
   //beam configs
  if (exists(config)){
    fitters.push_back(unique_ptr< pulseFitter >
		       (new pulseFitter((char*)config.c_str())));
  } 

  else{
    cout << config << " not found. "
	 << "Using default config." << endl;
    fitters.push_back(unique_ptr< pulseFitter >
		      (new pulseFitter()));
  }
    
  //laser configs
  config = string("configs/") + device.name + string("Laser.json");
  if (exists(config)){
    fitters.push_back(unique_ptr< pulseFitter >(new pulseFitter((char*)config.c_str())));
  }
    
  else{
    cout << config << " not found. "
	 << "Using default laser config." << endl;
    fitters.push_back(unique_ptr< pulseFitter >
		       (new pulseFitter((char*)"configs/.defaultLaserConfig.json")));
  } 
}

void fitDevice(unsigned short* trace, fitResults& fr, pulseFitter& fitter, const deviceInfo& device){
  //fill trace
  for( int k = 0; k < CHOPPED_TRACE_LENGTH; ++k){
    fr.trace[k] = 
      trace[fitter.getFitStart()+k];
  }

  //get summary information from the trace
  int maxdex;
  if(!device.neg){
    maxdex = max_element(trace,trace+TRACELENGTH) - trace;
  }
  else{
    maxdex = min_element(trace, trace + TRACELENGTH) - trace;
  }
  
  //template
  if(fitter.getFitType() == string("template")){
    fitter.setFitStart(maxdex - fitter.getFitLength()/2);
  }
    
  //parametric
  else{
    if(device.moduleType == string("drs"))
      fitter.setFitStart(maxdex - fitter.getFitLength() + 3);  
    else if(device.moduleType == string("struck"))
      fitter.setFitStart(maxdex - fitter.getFitLength() + 2);
  }
						       			       
  fr.aSum = fitter.getSum(trace,fitter.getFitStart(),fitter.getFitLength());
    
  fr.aAmpl = trace[maxdex] - fitter.getBaseline();

  //set fit config based on maxdex
  //template
  if(fitter.getFitType() == string("template")){
    fitter.setParameterGuess(0,maxdex);
    fitter.setParameterMin(0,maxdex-1);
    fitter.setParameterMax(0,maxdex+1);
  }
      
  //parametric lsaer
  else if (fitter.getFitType() == string("laser")){
    if (device.moduleType == string("drs")){
      fitter.setParameterGuess(0,maxdex-4);
      fitter.setParameterMin(0,maxdex-6);
      fitter.setParameterMax(0,maxdex-2);
    }
    else if (device.moduleType == string("struck")){
      fitter.setParameterGuess(0,maxdex-2);
      fitter.setParameterMin(0,maxdex-3);
      fitter.setParameterMax(0,maxdex-1);
    }
  }
  //parametric beam
  else if (fitter.getFitType() == string("beam")){
    fitter.setParameterGuess(0,maxdex-3);
    fitter.setParameterMin(0,maxdex-4);
    fitter.setParameterMax(0,maxdex-2);
  }

  //do the fits
  if(fitter.isFitConfigured()){
    fitter.fitSingle(trace);
    
    if(fitter.getFitType() == string("template")){
      fr.energy = fitter.getScale();
    }
    else if(fitter.getFitType() == string("laser")){
      fr.energy = 2.0*fitter.getScale();
    }
    else if(fitter.getFitType() == string("beam")){
      fr.energy = fitter.getScale()*fitter.getParameter(5)*fitter.getParameter(5)*
	(1/(fitter.getParameter(5)+fitter.getParameter(4))*
	 (fitter.getParameter(3)+fitter.getParameter(5)));
    }
    fr.chi2 = fitter.getChi2();
    fr.time = fitter.getTime();
    fr.valid = fitter.wasValidFit();
	
    //fill fitTrace
    fitter.fillFitTrace(fr.fitTrace,
			 fitter.getFitStart(),
			 CHOPPED_TRACE_LENGTH);     
  }

  if (fr.energy<0){
    fr.energy=-1.0*fr.energy;
  }
  
  fr.baseline = fitter.getBaseline();   
}

void initStruck(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& sr,
		vector< unique_ptr<pulseFitter> >& sFitters){
  //initialize each device
  for(unsigned int i = 0; i < devices.size(); ++i){
    initTraceDevice(outTree, devices[i], &sr[i], sFitters);
  }
}

void crunchStruck(vector<sis_fast>& sFast, 
		  const vector<deviceInfo>& devices,
		  vector<fitResults>& sr,
		  vector< unique_ptr<pulseFitter> >& sFitters){
  
  //temp
  int laserRun = 0;
  
  //loop over each device 
  for(unsigned int j = 0; j < devices.size(); ++j){
    fitDevice(sFast[devices[j].moduleNum].trace[devices[j].channel],
	      sr[j], 
	      *sFitters[2*j+laserRun], devices[j]);
  }   
}

void initDRS(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& drsR,
		vector< unique_ptr<pulseFitter> >& drsFitters){
  
  //initialize each device
  for(unsigned int i = 0; i < devices.size(); ++i){
    initTraceDevice(outTree, devices[i], &drsR[i], drsFitters);
  }
}

void crunchDRS(vector<drs>& drs, 
		  const vector<deviceInfo>& devices,
		  vector<fitResults>& drsR,
		  vector< unique_ptr<pulseFitter> >& drsFitters){
  
  //temp
  int laserRun = 0;

  //loop over each device 
  for(unsigned int j = 0; j < devices.size(); ++j){
    fitDevice(drs[devices[j].moduleNum].trace[devices[j].channel],
	      drsR[j], 
	      *drsFitters[2*j+laserRun], devices[j]);
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
