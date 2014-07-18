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
#include "TROOT.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"

//boost includes
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//open mp
#include <omp.h>

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
  float cathodeX;
  float cathodeY;
  float anode;
  bool good;
} wireChamberResults;

typedef UShort_t flagResults;

typedef UShort_t adcResults;

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

typedef struct {
  ULong64_t system_clock;
  ULong64_t device_clock[4];
  UShort_t trace[4][1024];
} sis_fast;

typedef struct {
  ULong64_t system_clock;
  ULong64_t device_clock[8];
  UShort_t trace[8][1024];
} sis_slow;

typedef struct {
  ULong64_t system_clock;
  ULong64_t device_clock[16];
  UShort_t trace[16][1024];
} drs;

typedef struct {
  ULong64_t system_clock;
  ULong64_t device_clock[8];
  UShort_t value[8];
} adc;

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
		vector< shared_ptr<pulseFitter> >& sFitters);

//crunch through struck devices for a given event
void crunchStruck(vector< vector<sis_fast> >& data, 
		  const vector<deviceInfo>& devices,
		  vector< vector<fitResults> >& sr,
		  vector< vector<flagResults> > flResults, 
		  vector< shared_ptr<pulseFitter> >& sFitters);

//init for drs devices
void initDRS(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& drsR,
		vector< shared_ptr<pulseFitter> >& drsFitters);

//crunch through drs devices for a given event
void crunchDRS(vector< vector<drs> >& data, 
	       const vector<deviceInfo>& devices,
	       vector< vector<fitResults> >& drsR,
	       vector< vector<flagResults> > flResults,
	       vector< shared_ptr<pulseFitter> >& drsFitters);

//init slow struck
void initStruckS(TTree& outTree, 
		 const vector<deviceInfo>& devices,
		 vector<fitResults>& srSlow,
		 vector<flagResults>& flR,
		 vector< shared_ptr<pulseFitter> >& slFitters); 

//crunch slow struck, includes beam and laser flags
void crunchStruckS(vector< sis_slow >& data,
		   const vector<deviceInfo>& devices,
		   vector< vector<fitResults> >& srSlow,
		   vector< vector<flagResults> >& flResults,
		   vector< shared_ptr<pulseFitter> >& slFitters);
   

//init adc
void initAdc(TTree& outTree,
	     const vector<deviceInfo>& devices,
	     vector<adcResults>& ar,
	     wireChamberResults* wr);

//crunch adc
void crunchAdc(const vector< vector<adc> >& adc_data,
	       const vector<deviceInfo>& devices,
	       vector< vector<adcResults> >& ar,
	       vector< wireChamberResults >& wr,
	       vector< vector<flagResults> > flResults);

//check if a file exists
bool exists(const string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

int main(int argc, char* argv[]) {
  new TApplication("app", 0, nullptr);
  gROOT->ProcessLine("gErrorIgnoreLevel = kWarning");
  
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

  //fast struck
  vector<sis_fast> sFast(2);
  inTree->SetBranchAddress("sis_fast_0", &sFast[0]);
  inTree->SetBranchAddress("sis_fast_1", &sFast[1]);
  vector<fitResults> sr(rInfo.struckInfo.size());
  vector< shared_ptr<pulseFitter> > sFitters;
  initStruck(outTree, rInfo.struckInfo, sr, sFitters);

  //drs
  vector<drs> drsVec(2);
  inTree->SetBranchAddress("caen_drs_0", &drsVec[0]);
  inTree->SetBranchAddress("caen_drs_1", &drsVec[1]);
  vector<fitResults> drsR(rInfo.drsInfo.size());
  vector< shared_ptr<pulseFitter> > drsFitters;
  initDRS(outTree, rInfo.drsInfo, drsR, drsFitters);

  //slow struck
  sis_slow s;
  inTree->SetBranchAddress("sis_slow_0",&s);
  vector<fitResults> srSlow;
  vector<flagResults> flResults(rInfo.struckSInfo.size());
  vector< shared_ptr<pulseFitter> > slFitters;
  initStruckS(outTree, rInfo.struckSInfo, srSlow, flResults, slFitters);

  //adcs
  vector<adc> adcs(2);
  inTree->SetBranchAddress("caen_adc_0", &adcs[0]);
  inTree->SetBranchAddress("caen_adc_1", &adcs[1]);
  vector<adcResults> ar(rInfo.adcInfo.size());
  wireChamberResults wr;
  initAdc(outTree, rInfo.adcInfo, ar, &wr);
  
  //end initialization routines
  
  //load-crunch-dump loop, processes data in chunks of BATCH_SIZE
  unsigned int startEntry = 0;
  unsigned int endEntry = BATCH_SIZE < inTree->GetEntries() 
    ? BATCH_SIZE : inTree->GetEntries();
  
  while(startEntry!= inTree->GetEntries()){
    unsigned int thisBatchSize = endEntry - startEntry;

    //set up neccesary data structures
    vector< vector<fitResults> > sis_slow_fit_res;
    vector< sis_slow > sis_slow_data(thisBatchSize);
    vector< vector<flagResults> > sis_slow_results(thisBatchSize);
    vector< vector<sis_fast> > sis_fast_data(thisBatchSize);
    vector< vector<fitResults> > sis_fast_results(thisBatchSize);
    vector< vector<drs> > drs_data(thisBatchSize);
    vector< vector<fitResults> > drs_results(thisBatchSize);
    vector< vector<adc> > adc_data(thisBatchSize);
    vector< vector<adcResults> > adc_results(thisBatchSize);
    vector< wireChamberResults > wire_results(thisBatchSize);

    //load data
    for(unsigned int i = startEntry; i < endEntry; ++i){
      inTree->GetEntry(i);
      unsigned int dataIndex = i - startEntry;

      sis_slow_results[dataIndex].resize(rInfo.struckSInfo.size());
      drs_results[dataIndex].resize(rInfo.drsInfo.size());
      sis_fast_results[dataIndex].resize(rInfo.struckInfo.size());
      adc_results[dataIndex].resize(rInfo.adcInfo.size());
      sis_fast_data[dataIndex].resize(2);


      sis_slow_data[dataIndex] = s;

      for(unsigned int j = 0; j < sis_fast_data[dataIndex].size(); ++j){
	sis_fast_data[dataIndex][j] = sFast[j];
      }
    
      drs_data[dataIndex].resize(2);
      for(unsigned int j = 0; j < drs_data[dataIndex].size(); ++j){
	drs_data[dataIndex][j] = drsVec[j];
      }
    
      adc_data[dataIndex].resize(2);
      for(unsigned int j = 0; j < adc_data[dataIndex].size(); ++j){
	adc_data[dataIndex][j] = adcs[j];
      }
    }    

    //crunch data
    crunchStruckS(sis_slow_data, rInfo.struckSInfo, sis_slow_fit_res, sis_slow_results,
		  slFitters);
    crunchStruck(sis_fast_data, rInfo.struckInfo, sis_fast_results, sis_slow_results, sFitters);
    crunchDRS(drs_data, rInfo.drsInfo, drs_results, sis_slow_results, drsFitters);
    crunchAdc(adc_data, rInfo.adcInfo, adc_results, wire_results, sis_slow_results);

    //dump data
    for(unsigned int i = 0; i < thisBatchSize; ++i){
      sr = sis_fast_results[i];
      drsR = drs_results[i];
      wr = wire_results[i];
      ar = adc_results[i];
      flResults = sis_slow_results[i];
      outTree.Fill();
    }

    cout << "processed up to event " << endEntry << endl;

    startEntry = endEntry;
    endEntry = endEntry + BATCH_SIZE < inTree->GetEntries() 
    ? endEntry + BATCH_SIZE  : inTree->GetEntries();
  }
}

void initTraceDevice(TTree& outTree, 
		     const deviceInfo& device,
		     fitResults* fr,
		     vector< shared_ptr<pulseFitter> >& fitters){
  
  //initialize output tree branch for this device
  outTree.Branch(device.name.c_str(), fr,
      Form("trace[%i]/D:fitTrace[%i]/D:energy/D:aSum/D:baseline/D:time/D:aAmpl/D:chi2/D:valid/O",
	   CHOPPED_TRACE_LENGTH, CHOPPED_TRACE_LENGTH));    
 
  //intitalize fitter
  string config = string("configs/") + device.name + string(".json");
   //beam configs
  if (exists(config)){
    fitters.push_back(shared_ptr< pulseFitter >
		       (new pulseFitter((char*)config.c_str())));
  } 

  else{
    cout << config << " not found. "
	 << "Using default config." << endl;
    fitters.push_back(shared_ptr< pulseFitter >
		      (new pulseFitter()));
  }
    
  //laser configs
  config = string("configs/") + device.name + string("Laser.json");
  if (exists(config)){
    fitters.push_back(shared_ptr< pulseFitter >(new pulseFitter((char*)config.c_str())));
  }
    
  else{
    cout << config << " not found. "
	 << "Using beam config." << endl;
    config = string("configs/") + device.name + string(".json");
    fitters.push_back(fitters[fitters.size()-1]);
  } 
}

void fitDevice(UShort_t* trace, fitResults& fr, pulseFitter& fitter, const deviceInfo& device){
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
    if(device.moduleType == string("drs"))
      fitter.setFitStart(maxdex - 4*fitter.getFitLength()/5);  
    else
      fitter.setFitStart(maxdex - 3*fitter.getFitLength()/4);
   
  }
    
  //parametric
  else{
    if(device.moduleType == string("drs"))
      fitter.setFitStart(maxdex - fitter.getFitLength() + 3);  
    else
      fitter.setFitStart(maxdex - fitter.getFitLength() + 2);
  }
						       			       
  fr.aSum = fitter.getSum(trace,fitter.getFitStart(),fitter.getFitLength());
    
  fr.aAmpl = trace[maxdex] - fitter.getBaseline();

  //set fit config based on maxdex
  //template
  if(fitter.getFitType() == string("template")){
    fitter.setParameterGuess(0,maxdex);
    fitter.setParameterMin(0,maxdex-3);
    fitter.setParameterMax(0,maxdex+3);
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

  //fill trace
  for( int k = 0; k < CHOPPED_TRACE_LENGTH; ++k){
    fr.trace[k] = 
      trace[fitter.getFitStart()+k];
  }

  if (fr.energy<0){
    fr.energy=-1.0*fr.energy;
  }
  
  fr.baseline = fitter.getBaseline();   
}

void initStruck(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& sr,
		vector< shared_ptr<pulseFitter> >& sFitters){
  //initialize each device
  for(unsigned int i = 0; i < devices.size(); ++i){
    initTraceDevice(outTree, devices[i], &sr[i], sFitters);
  }
}

void filterTrace(UShort_t* trace, int length){
  int filterLength = length;
  for(int i = 0; i < TRACELENGTH-filterLength; ++i){
    int runningSum = 0;
    for(int j = 0; j < filterLength; ++j){
      runningSum+=trace[i+j];
    }
    trace[i] = runningSum/filterLength;
  }
} 

void crunchStruck(vector< vector<sis_fast> >& data, 
		  const vector<deviceInfo>& devices,
		  vector< vector<fitResults> >& sr,
		  vector< vector<flagResults> > flResults,
		  vector< shared_ptr<pulseFitter> >& sFitters){
  
  //temp
  
  //loop over each device 
  #pragma omp parallel for firstprivate(flResults)
  for(unsigned int j = 0; j < devices.size(); ++j){
    for( unsigned int i = 0; i < data.size(); ++i){
      UShort_t laserRun = flResults[i][1];
      filterTrace(data[i][devices[j].moduleNum].trace[devices[j].channel], 5);
      fitDevice(data[i][devices[j].moduleNum].trace[devices[j].channel],
		sr[i][j], 
		*sFitters[2*j+laserRun], devices[j]);
    }   
  }
}

void initDRS(TTree& outTree, 
		const vector<deviceInfo>& devices,
		vector<fitResults>& drsR,
		vector< shared_ptr<pulseFitter> >& drsFitters){
  
  //initialize each device
  for(unsigned int i = 0; i < devices.size(); ++i){
    initTraceDevice(outTree, devices[i], &drsR[i], drsFitters);
  }
} 

void crunchDRS(vector< vector<drs> >& data, 
	       const vector<deviceInfo>& devices,
	       vector< vector<fitResults> >& drsR,
	       vector< vector<flagResults> > flResults,
	       vector< shared_ptr<pulseFitter> >& drsFitters){
  
  //temp

  //loop over each device 
  #pragma omp parallel for firstprivate(flResults)
  for(unsigned int j = 0; j < devices.size(); ++j){
    for(unsigned int i = 0; i < data.size(); ++i){
      UShort_t laserRun = flResults[i][1];
      filterTrace(data[i][devices[j].moduleNum].trace[devices[j].channel], 10);
      fitDevice(data[i][devices[j].moduleNum].trace[devices[j].channel],
		drsR[i][j], 
		*drsFitters[2*j+laserRun], devices[j]);
    }
  }
}

/*void initStruckS(TTree& outTree, 
		 const vector<deviceInfo>& devices,
		 vector<struckSResults>& srSlow,
		 vector< shared_ptr<pulseFitter> >& slFitters){
  
  for (unsigned int i = 0; i < devices.size(); ++i){
    outTree.Branch(devices[i].name.c_str(), &srSlow[i], "aAmpl/D");
  }
  }*/


//beam flag MUST come first in config file
//fix to work with arbitrary order of things in runjson
void initStruckS(TTree& outTree, 
		 const vector<deviceInfo>& devices,
		 vector<fitResults>& srSlow,
		 vector<flagResults>& flR,
		 vector< shared_ptr<pulseFitter> >& slFitters){
  
  int flagIndex = 0;
  for (unsigned int i = 0; i < devices.size(); ++i){
    if(devices[i].name == "beamFlag" || devices[i].name == "laserFlag"){
      outTree.Branch(devices[i].name.c_str(), &flR[flagIndex], "flag/O");
      ++flagIndex;
      slFitters.push_back(NULL);
      slFitters.push_back(NULL);
    }
    /*else{
      initTraceDevice(outTree, devices[i], &srSlow[i], slFitters);
      }*/
  }
}

//completely temporary implementation
/*void crunchStruckS(sis_slow& s,
		   const vector<deviceInfo>& devices,
		   vector<struckSResults>& srSlow,
		   vector< shared_ptr<pulseFitter> >& slFitters){
  if(devices.size()>0){
    double baseline = accumulate(s.trace[4],s.trace[4]+20,0)/20;
    double ampl = *max_element(s.trace[4],s.trace[4]+TRACELENGTH)-baseline;
    srSlow[0].aAmpl = ampl;
  }
}
*/
 
void crunchStruckS(vector< sis_slow >& data,
		   const vector<deviceInfo>& devices,
		   vector< vector<fitResults> >& srSlow,
		   vector< vector<flagResults> >& flResults,
		   vector< shared_ptr<pulseFitter> >& slFitters){
  
  #pragma omp parallel for
  for(unsigned int j = 0; j < devices.size(); ++j){
    for(unsigned int i = 0; i < data.size(); ++i){
      UShort_t max = *max_element(data[i].trace[devices[j].channel], 
				    data[i].trace[devices[j].channel]+TRACELENGTH);
      if (max > 32000){
	flResults[i][j] = 1;
      }
      else{
	flResults[i][j] = 0;
      }
    }
  }
}

void initAdc(TTree& outTree,
	     const vector<deviceInfo>& devices,
	     vector<adcResults>& ar,
	     wireChamberResults* wr){
  for (unsigned int i = 0; i < devices.size(); ++i){
    
    if ( devices[i].name == "wireChamber" ){
      outTree.Branch(devices[i].name.c_str(), wr, 
		     "cathode_x/F:cathode_y/F:anode/F:good/O");
    }
    
    else{
      outTree.Branch(devices[i].name.c_str(), &ar[i], "value/s");
    }
    
  } 
  
}


//WIRE CHAMBER FUNCTIONS FROM PETE

float compute_cathode_position(UShort_t c[]){ // center of gravity approach

  // the array provided needs to be length six and have the channels
  // mapped so that the c[0] corresponds to the -20 position and c[5] corresponds to 20

    float cathode_position = 0;
    float cathode_sum = 0;
    for(int i=0;i<6;i++){
      if(c[i]<4000.){
	cathode_position += c[i]*(-20.+(i)*8.); // 20 and 8 are mm and come from the physical parameters of the device
	cathode_sum += c[i];
      }
    }
    cathode_position /= cathode_sum;

    if(cathode_sum>500)   return cathode_position;
    else {
      return -30;
    }
} // end compute_cathode_position

float compute_anode_position(UShort_t a1, UShort_t a2){ // charge division approach

  if(a1>600.&&a2>600.&&a1<4000.&&a2<4000.){ // cuts for valid signals
    // anode 1 calib 260 // anode 2 calib 350
    float cal = 350./260.; // scale to make a1 signal same size as a2
    float L = 21.; //half length scale of device in mm
    float anode_position = L * (cal*a1-a2)/(cal*a1+a2);
    return anode_position;
  } 
  else {
    return -30;
  }
} // end compute_anode_position


//END FUNCTIONS FROM PETE

//implementing pete's wire chamber analysis code
void computeWireChamber(const UShort_t* adc1,
			const UShort_t* adc2,
			wireChamberResults& wr){

  wr.good = true;

  UShort_t cathodeXVals[6] = {adc1[7], adc1[6], adc1[5],
			      adc1[4], adc1[3], adc1[2]};
  
  UShort_t cathodeYVals[6] = {adc2[7], adc2[6], adc2[5],
			      adc2[4], adc2[3], adc2[2]};
  
  wr.anode = compute_anode_position(adc1[1], adc1[0]);
  wr.cathodeX = compute_cathode_position(cathodeXVals);
  wr.cathodeY = compute_cathode_position(cathodeYVals);

  if(wr.anode == -30 || wr.cathodeX == -30 || wr.cathodeY == -30){
    wr.good = false;
  }
}

void crunchAdc(const vector< vector<adc> >& adc_data,
	       const vector<deviceInfo>& devices,
	       vector< vector<adcResults> >& ar,
	       vector< wireChamberResults >& wr,
	       vector< vector<flagResults> > flResults){

  #pragma omp parallel for firstprivate(flResults)
  for(unsigned int i = 0; i < devices.size(); ++i){
    for(unsigned int j = 0; j < adc_data.size(); ++j){
      if (devices[i].name == "wireChamber"){
	if(flResults[i][0]==1){
	  computeWireChamber(adc_data[j][0].value, adc_data[j][1].value, wr[j]);
	}
	else{
	  wr[i].good = false;
        }
      }
   
      else{
	ar[j][i] = adc_data[j][devices[i].moduleNum].value[devices[i].channel];
      }
    }
  }
}
