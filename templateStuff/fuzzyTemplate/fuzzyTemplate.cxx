/*
Aaron Fienberg
fienberg@uw.edu
code for generating "fuzzy templates" based on digitized datasets
*/

#include <iostream>
#include <cmath>
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

const int TEMPLATELENGTH = 200;
const int NBINSPSEUDOTIME = 500;
const int NTIMEBINS = 20;
const int STRUCKCHANNEL = 6;
const int TRACELENGTH = 1024;
const int BASELINEFITLENGTH = 50;
const int BUFFERZONE = 40;

struct s_sis{
  unsigned long long timestamp[8];
  unsigned short trace[8][0x400];
  bool is_bad_event;
};

typedef struct traceSummary{
  double pseudoTime;
  int peakIndex;
  double baseline;
  double integral;
  double normalizedAmpl;
  bool bad;
}traceSummary;


traceSummary processTrace(unsigned short* trace);
double* correctTrace(unsigned short* trace, traceSummary summary);

int main(int argc, char* argv[]) {
  clock_t t1,t2;
  t1 = clock();
  
  if(argc!=2){
    cout << "need to input datafile." << endl;
    return -1;
  }
  
  //read input file
  gSystem->Load("libTree");
  TFile infile(argv[1]);
  TTree* t = (TTree*) infile.Get("t");
  struct s_sis s;
  t->SetBranchAddress("sis", &s);
  
  //process traces
  cout << "Processing traces... " << endl;
  vector<traceSummary> summaries(t->GetEntries());
  TH1D pseudoTimesHist("ptimes", "ptimes", NBINSPSEUDOTIME,0,1);
  TH1D normalizedMaxes("maxes","maxes",100,0.0,0.0);
  for(int i = 0; i < t->GetEntries(); ++i){
    t->GetEntry(i);
    summaries[i] = processTrace(s.trace[STRUCKCHANNEL]);
    pseudoTimesHist.Fill(summaries[i].pseudoTime);
    normalizedMaxes.Fill(summaries[i].normalizedAmpl);
    if(i % 1000 == 0){
      cout << "Trace " << i << " processed." << endl;
    }
  }
  pseudoTimesHist.Scale(1.0/pseudoTimesHist.Integral());
  
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
  
  //fill the timeslices and make the master fuzzy template
  TH2D masterFuzzyTemplate = TH2D("masterFuzzy", "Fuzzy Template", 
			     TEMPLATELENGTH*NTIMEBINS, 0, TEMPLATELENGTH,
				  400,-.2*binRangeMax,binRangeMax);
  //double timeSlices[NTIMEBINS][TEMPLATELENGTH];
  vector< vector<double> > timeSlices(NTIMEBINS, vector<double>(TEMPLATELENGTH, 0));
  cout << "Populating timeslices... " << endl;
  for(int i = 0; i < t->GetEntries(); ++i){
    t->GetEntry(i);
    double realTime = rtSpline.Eval(summaries[i].pseudoTime);
    int thisSlice = static_cast<int>(realTime*NTIMEBINS);
    if(thisSlice == NTIMEBINS) --thisSlice;
    double* ctrace = correctTrace(s.trace[STRUCKCHANNEL], summaries[i]);
    for(int j = 0; j<TEMPLATELENGTH; ++j){
      timeSlices[thisSlice][j] += ctrace[j];
      masterFuzzyTemplate.Fill(j-realTime+1, ctrace[j]);
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
  for(int i = 0; i < TEMPLATELENGTH*NTIMEBINS; ++i){
    TH1D* xBinHist = masterFuzzyTemplate.ProjectionY("binhist",i+1,i+1);
    xBinHist->Fit("gaus","0q");
    errorGraph.SetPoint(i,static_cast<float>(i)/NTIMEBINS,
			xBinHist->GetFunction("gaus")->GetParameter(2));
    masterGraph.SetPoint(i,static_cast<float>(i)/NTIMEBINS,
			 xBinHist->GetFunction("gaus")->GetParameter(1));
    masterGraph.SetPointError(i,0,xBinHist->GetFunction("gaus")->GetParameter(2));
    delete xBinHist;
  }
  cout << "Errors and Means Calculated" << endl;
  
  //save data
  TFile outf("testOutput.root","recreate");
  rtSpline.Write();
  pseudoTimesHist.Write();
  masterFuzzyTemplate.Write();
  errorGraph.Write();
  masterGraph.Write();
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

  //find minimum
  int mindex = 0;
  for(int i = 0; i < TRACELENGTH; ++i){
    mindex = trace[i] < trace[mindex] ? i : mindex;
  }
  results.peakIndex = mindex;
  
  //calculate pseudotime
  if(trace[mindex]==trace[mindex+1]) results.pseudoTime = 1;
  else{
    results.pseudoTime = 2.0/M_PI*atan(static_cast<float>(trace[mindex-1]-trace[mindex])/
				     (trace[mindex+1]-trace[mindex]));
  }
  
  //get the baseline 
  if(mindex-BASELINEFITLENGTH-BUFFERZONE<0){
    cout << "Baseline fit walked off the end of the trace!" << endl;
    results.bad = true;
    return results;
  }
  double runningBaseline = 0;
  for(int i = 0; i<BASELINEFITLENGTH; ++i){
    runningBaseline=runningBaseline+trace[mindex-BUFFERZONE-BASELINEFITLENGTH+i];
  }
  results.baseline = runningBaseline/BASELINEFITLENGTH;

  //get the normalization
  if(mindex-BUFFERZONE+TEMPLATELENGTH>TRACELENGTH){
    results.bad = true;
    return results;
  }
  double runningIntegral = 0;
  for(int i = 0; i<TEMPLATELENGTH; ++i){
    runningIntegral = runningIntegral+trace[mindex-BUFFERZONE+i]-results.baseline;
  }
  results.integral = runningIntegral;
  
  results.normalizedAmpl = (trace[mindex]-results.baseline)/results.integral;
  return results;
}
  
double* correctTrace(unsigned short* trace, traceSummary summary){
  double* correctedTrace = new double[TEMPLATELENGTH];
  if(summary.bad){
    for(int i = 0; i < TEMPLATELENGTH; ++i) correctedTrace[i] = 0;
    return correctedTrace;
  }
  for(int i = 0; i <TEMPLATELENGTH; ++i){
    correctedTrace[i] = (trace[summary.peakIndex-BUFFERZONE+i]-summary.baseline)/(summary.integral);
  }
  return correctedTrace;
}
