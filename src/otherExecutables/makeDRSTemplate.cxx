/*
Aaron Fienberg
fienberg@uw.edu
code for generating "fuzzy templates" based on digitized datasets
*/

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "TSystem.h"
#include "TTree.h"
#include "TGraphErrors.h"
#include "TFile.h"
#include "TSpline.h"
#include "TH1.h"
#include "TF1.h"
#include "TH2.h"
#include "TString.h"
#include "time.h"
using namespace std;

const int TEMPLATELENGTH = 100;
const int NBINSPSEUDOTIME = 500;
const int NTIMEBINS = 1;
const int DEFAULTDRSCHANNEL = 0;
const int TRACELENGTH = 1024;
const int BASELINEFITLENGTH = 50;
const int BUFFERZONE = 40;

typedef struct {
  unsigned long system_clock;
  unsigned long device_clock[18];
  unsigned short trace[18][1024];
} drs;

typedef struct traceSummary{
  double pseudoTime;
  int peakIndex;
  double baseline;
  double integral;
  double normalizedAmpl;
  bool bad;
}traceSummary;


traceSummary processTrace(unsigned short* trace);
double* correctTrace(unsigned short* trace, traceSummary summary, double meanIntegral);

const int filterLength = 10;
void filterTrace(unsigned short* trace){
  for(int i = 0; i < TRACELENGTH-filterLength; ++i){
    int runningSum = 0;
    for(int j = 0; j < filterLength; ++j){
      runningSum+=trace[i+j];
    }
    trace[i] = runningSum/filterLength;
  }
}  

int main(int argc, char* argv[]) {
  clock_t t1,t2;
  t1 = clock();
  
  if(argc<5){
    cout << "usage: ./makeTemplate [inputfile] [outputfile] [drsmodule] [drsChannel]" << endl;
    return -1;
  }
  
  int drsChannel = DEFAULTDRSCHANNEL;
  drsChannel = atoi(argv[4]);
  
  //read input file
  gSystem->Load("libTree");
  TFile infile(argv[1]);
  TTree* t = (TTree*) infile.Get("t");
  drs s;
  if(argv[3][0] == '0'){
    t->SetBranchAddress("caen_drs_0", &s);
  }
  else{
    t->SetBranchAddress("caen_drs_1", &s);
  }
  
  //process traces
  cout << "Processing traces... " << endl;
  vector<traceSummary> summaries(t->GetEntries());
  TH1D pseudoTimesHist("ptimes", "ptimes", NBINSPSEUDOTIME,0,1);
  TH1D normalizedMaxes("maxes","maxes",100,0.0,0.0);
  TH1D integralHist("integrals","integrals",100,0.0,0.0);
  for(int i = 0; i < t->GetEntries(); ++i){
    t->GetEntry(i);
    filterTrace(s.trace[drsChannel]);
    summaries[i] = processTrace(s.trace[drsChannel]);
    pseudoTimesHist.Fill(summaries[i].pseudoTime);
    normalizedMaxes.Fill(summaries[i].normalizedAmpl);
    integralHist.Fill(summaries[i].integral);
    if(i % 1000 == 0){
      cout << "Trace " << i << " processed." << endl;
    }
  }
  integralHist.Fit("gaus","q0");
  double meanIntegral = integralHist.GetFunction("gaus")->GetParameter(1);
  pseudoTimesHist.Scale(1.0/pseudoTimesHist.Integral());
  
  //temporary hack to re-implement normalization
  meanIntegral = 1.0;
  

  //find max for fuzzy template bin range
  normalizedMaxes.Fit("gaus","q0");
  double binRangeMax = normalizedMaxes.GetFunction("gaus")->GetParameter(1) + 
    5*normalizedMaxes.GetFunction("gaus")->GetParameter(2);
  
  //create map to real time
  TGraph realTimes(0);
  realTimes.SetName("realTimeGraph");
  realTimes.SetPoint(0,0,0);
  for(int i = 0; i < NBINSPSEUDOTIME; ++i){
    realTimes.SetPoint(i, pseudoTimesHist.GetBinLowEdge(i+2),
			pseudoTimesHist.Integral(1,i+1));
  }
  TSpline3 rtSpline = TSpline3("realTimeSpline",&realTimes);
  rtSpline.SetName("realTimeSpline");
  
  //fill the timeslices and make the master fuzzy template
  TH2D masterFuzzyTemplate = TH2D("masterFuzzy", "Fuzzy Template", 
			     TEMPLATELENGTH*NTIMEBINS, -.5-BUFFERZONE, TEMPLATELENGTH-.5-BUFFERZONE,
				  1000,-.2*binRangeMax*abs(meanIntegral),binRangeMax*abs(meanIntegral));

  cout << "Populating timeslices... " << endl;
  for(int i = 0; i < t->GetEntries(); ++i){
    t->GetEntry(i);
    filterTrace(s.trace[drsChannel]);
    if(summaries[i].bad){
      continue;
    }
    double realTime = rtSpline.Eval(summaries[i].pseudoTime);
    int thisSlice = static_cast<int>(realTime*NTIMEBINS);
    if(thisSlice == NTIMEBINS) --thisSlice;
    double* ctrace = correctTrace(s.trace[drsChannel], summaries[i], meanIntegral);
    for(int j = 0; j<TEMPLATELENGTH; ++j){
      masterFuzzyTemplate.Fill(j-realTime+0.5-BUFFERZONE, ctrace[j]);
    }
    if(i % 1000 == 0){
      cout << "Trace " << i << " placed." << endl;
    }
    delete ctrace;
  }
  
  //step through fuzzy template to get errors and means
  cout << "Calculating errors and means... " << endl;
  TGraphErrors masterGraph(0);
  masterGraph.SetName("masterGraph");
  TGraph errorGraph(0);
  errorGraph.SetName("errorGraph");
  TGraph errorVsMean(0);
  errorVsMean.SetName("errorVsMean");
  for(int i = 0; i < TEMPLATELENGTH*NTIMEBINS; ++i){
    TH1D* xBinHist = masterFuzzyTemplate.ProjectionY("binhist",i+1,i+1);
    xBinHist->Fit("gaus","q0","",xBinHist->GetMean()-xBinHist->GetRMS()*3,
		  xBinHist->GetMean()+xBinHist->GetRMS()*3);
    // errorGraph.SetPoint(i,static_cast<float>(i)/NTIMEBINS,
    // 			xBinHist->GetRMS());
    // masterGraph.SetPoint(i,static_cast<float>(i)/NTIMEBINS,
    // 			 xBinHist->GetMean());
    double mean = xBinHist->GetFunction("gaus")->GetParameter(1);
    double sig = xBinHist->GetFunction("gaus")->GetParameter(2);
    errorGraph.SetPoint(i,static_cast<float>(i)/NTIMEBINS-BUFFERZONE-.5,
			sig);
    masterGraph.SetPoint(i,static_cast<float>(i)/NTIMEBINS-BUFFERZONE-.5,
			 mean);
    masterGraph.SetPointError(i,0,
			      sig);
    // errorVsMean.SetPoint(i,xBinHist->GetMean(),
    // 			 xBinHist->GetRMS());
    errorVsMean.SetPoint(i,mean,sig);
   delete xBinHist;
  }
  cout << "Errors and Means Calculated" << endl;
  
  TSpline3 masterSpline("masterSpline",&masterGraph);
  masterSpline.SetName("masterSpline");
  masterSpline.SetNpx(10000);
  TSpline3 errorSpline("errorSpline",&errorGraph);
  errorSpline.SetName("errorSpline");
  errorSpline.SetNpx(10000);

  //save data
  TFile outf(argv[2],"recreate");
  rtSpline.Write();
  pseudoTimesHist.Write();
  masterFuzzyTemplate.Write();
  errorGraph.Write();
  masterGraph.Write();
  masterSpline.Write();
  errorSpline.Write();
  errorVsMean.Write();
  outf.Write();
  outf.Close();

  //finish up
  delete t;
  t2 = clock();
  float diff ((float)t2-(float)t1);
  cout << "Time elapsed: " << diff/CLOCKS_PER_SEC << " seconds." << endl;
  return 0;
}

traceSummary processTrace(unsigned short* trace){
  traceSummary results;
  results.bad = false;

  //find maximum
  int maxdex = 0;
  for(int i = 0; i < TRACELENGTH; ++i){
    maxdex = trace[i] > trace[maxdex] ? i : maxdex;
  }
  results.peakIndex = maxdex;

  //calculate pseudotime
  if(trace[maxdex]==trace[maxdex+1]) results.pseudoTime = 1;
  else{
    results.pseudoTime = 2.0/M_PI*atan(static_cast<float>(trace[maxdex-1]-trace[maxdex])/
				     (trace[maxdex+1]-trace[maxdex]));
  }
  
  /* if(trace[maxdex]<4000 || trace[maxdex]>2000){
    results.bad = true;
    return results;
    }*/
    
  
  
  //get the baseline 
  if(maxdex-BASELINEFITLENGTH-BUFFERZONE<0){
    cout << "Baseline fit walked off the end of the trace!" << endl;
    results.bad = true;
    return results;
  }
  double runningBaseline = 0;
  for(int i = 0; i<BASELINEFITLENGTH; ++i){
    runningBaseline=runningBaseline+trace[maxdex-BUFFERZONE-BASELINEFITLENGTH+i];
  }
  results.baseline = runningBaseline/BASELINEFITLENGTH;

  //get the normalization
  if(maxdex-BUFFERZONE+TEMPLATELENGTH>TRACELENGTH){
    results.bad = true;
    return results;
  }
  double runningIntegral = 0;
  for(int i = 0; i<TEMPLATELENGTH; ++i){
    runningIntegral = runningIntegral+trace[maxdex-BUFFERZONE+i]-results.baseline;
  }
  results.integral = runningIntegral;

  results.normalizedAmpl = (trace[maxdex]-results.baseline)/results.integral;

  return results;
}
  
double* correctTrace(unsigned short* trace, traceSummary summary, double meanIntegral){
  double* correctedTrace = new double[TEMPLATELENGTH];
  if(summary.bad){
    for(int i = 0; i < TEMPLATELENGTH; ++i) correctedTrace[i] = 0;
    return correctedTrace;
  }
  for(int i = 0; i <TEMPLATELENGTH; ++i){
    correctedTrace[i] = (trace[summary.peakIndex-BUFFERZONE+i]-summary.baseline)*abs(meanIntegral)/(summary.integral);
  }
  return correctedTrace;
}
