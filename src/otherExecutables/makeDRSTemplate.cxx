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

const int TEMPLATELENGTH = 200;
const int NBINSPSEUDOTIME = 100;
const int NTIMEBINS = 1;
const int TRACELENGTH = 1024;
const int BASELINEFITLENGTH = 20;
const int BUFFERZONE = 60;

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
void filterTrace(unsigned short* trace);

int main(int argc, char* argv[]) {
  clock_t t1,t2;
  t1 = clock();
  
  if(argc<3){
    cout << "need to input datafile and output file." << endl;
    return -1;
  }

  //read input file
  gSystem->Load("libTree");
  TFile infile(argv[1]);
  TTree* t = (TTree*) infile.Get("WFDTree");
  TBranch* branch = t->GetBranch("Channel1");
  unsigned short ch1[1024];
  branch->SetAddress(&ch1);
  cout << branch -> GetEntries() << " pulses." << endl;
  
  
  //process traces
  cout << "Processing traces... " << endl;
  vector<traceSummary> summaries(t->GetEntries());
  TH1D pseudoTimesHist("ptimes", "ptimes", NBINSPSEUDOTIME,0,1);
  TH1D normalizedMaxes("maxes","maxes",100,0.0,0.0);
  TH1D integralHist("integrals","integrals",100,0.0,0.0);
  for(int i = 0; i < t->GetEntries(); ++i){
    t->GetEntry(i);
    filterTrace(ch1);
    summaries[i] = processTrace(ch1);
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
    filterTrace(ch1);
    if(summaries[i].bad){
      continue;
    }
    double realTime = rtSpline.Eval(summaries[i].pseudoTime);
    int thisSlice = static_cast<int>(realTime*NTIMEBINS);
    if(thisSlice == NTIMEBINS) --thisSlice;
    double* ctrace = correctTrace(ch1, summaries[i], meanIntegral);
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
  TFile outf(Form("%s.root",argv[2]),"recreate");
  rtSpline.Write();
  pseudoTimesHist.Write();
  masterFuzzyTemplate.Write();
  errorGraph.Write();
  masterGraph.Write();
  masterSpline.Write();
  errorSpline.Write();
  errorVsMean.Write();
  integralHist.Write();
  normalizedMaxes.Write();
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

//try to filter out weird drs noise
void filterTrace(unsigned short* trace){
  for(int i = 0; i < 1024/2; ++ i){
    trace[2*i] = (trace[2*i]+trace[2*i+1])*1/2;
  }
  for(int i = 0; i < 1024/2; ++i){
    if(i!=1024/2-1)
      trace[2*i+1] = (trace[2*i]+trace[2*i+2])/2;
    else
      trace[2*i+1] = trace[2*i];
  }
}  
