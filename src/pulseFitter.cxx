/* 
Aaron Fienberg
fienberg@uw.edu
Implementation for pulseFitter classes
*/  

#include <cmath> 
#include "pulseFitter.hh"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "TGraphErrors.h"
#include "TF1.h"
#include "TH1F.h"
#include "TAxis.h"
#include "TFile.h"
#include "TSpline.h"
#include "TFitResult.h"
#include "TCanvas.h"
#include "TSystem.h"
#include "Math/WrappedMultiTF1.h"
#include <vector>
#include <algorithm>


using namespace std;


/*
//lpg[2] device ramp
//lpg[3] device decay
//lpg[4] src ramp
//lpg[5] src decay
double pulseFitFunction::evalPulse(double t, double t0){
  double pulse;
  if(t < t0){
    pulse = 0;
  }
  else{
    double term1 = exp(-1*(t-t0)/lpg[3])*lpg[5]*lpg[3]/(lpg[3]-lpg[5])/(-1*lpg[5]*lpg[4]+lpg[5]*lpg[3]+lpg[4]*lpg[3]);
   
    double term2 = exp(-1*(t-t0)/lpg[5])*lpg[5]*lpg[3]/(lpg[5]-lpg[3])/(-1*lpg[2]*lpg[3]+lpg[5]*(lpg[3]+lpg[2]));
   
    double term3 = exp(-1*(t-t0)*(lpg[3]+lpg[2])/lpg[3]/lpg[2])*lpg[5]*lpg[3]*lpg[2]*lpg[2]/(-1*lpg[3]*lpg[2]+lpg[5]*(lpg[3]+lpg[2]))/(lpg[4]*lpg[3]*lpg[2]-1*lpg[5]*(-1*lpg[3]*lpg[2]+lpg[4]*(lpg[3]+lpg[2])));

    double term4 = exp(-1*(t-t0)*(lpg[5]+lpg[4])/lpg[5]/lpg[4])*lpg[4]*(1/(-1*lpg[5]*lpg[4]+(lpg[5]+lpg[4])*lpg[3])+lpg[2]/(-1*lpg[4]*lpg[3]*lpg[2]+lpg[5]*(-1*lpg[3]*lpg[2]+lpg[4]*(lpg[3]+lpg[2]))));

    pulse = (term1 + term2 + term3 + term4);
  }

  return pulse;  
}*/

/*
//lpg[2] is light width
//lpg[3] is device decay constant
//lpg[4] is device device ramp constant
double pulseFitFunction::evalPulse(double t, double t0){
  double term1 = exp((lpg[2]*lpg[2]+2.0*t0*lpg[3]-2.0*t*lpg[3])/(2*lpg[3]*lpg[3]))*
    erfc((lpg[2]*lpg[2]+(t0-t)*lpg[3])/(1.41421*lpg[2]*lpg[3]));
  
  double term2 = -1*exp((lpg[4]+lpg[3])*(2.0*(t0-t)*lpg[4]*lpg[3]+lpg[2]*lpg[2]*(lpg[4]+lpg[3]))/(2.0*lpg[4]*lpg[4]*lpg[3]*lpg[3]))*
    erfc(((t0-t)*lpg[4]*lpg[3]+lpg[2]*lpg[2]*(lpg[4]+lpg[3]))/(1.41421*lpg[2]*lpg[4]*lpg[3]));

  return (term1+term2);
}
*/

//template fit
double pulseFitFunction::evalPulse(double t, double t0){
  if((t-t0)>0&&(t-t0)<templateLength)
    return templateSpline->Eval(t-t0);
  else
    return 0;
}


pulseFitFunction::pulseFitFunction(char* config, bool templateFit):
  lpg(),
  isGoodPoint()
{
  //read config file
  boost::property_tree::ptree conf;
  read_json(config, conf);
  auto fitConfig = conf.get_child("fit_specs");
  auto digConfig = conf.get_child("digitizer_specs");  
  sampleRate = digConfig.get<double>("sample_rate");
  nParameters = fitConfig.get<int>("n_parameters");
  pulseFitStart = fitConfig.get<int>("fit_start"); 
  fitLength = fitConfig.get<int>("fit_length");
  traceLength = digConfig.get<int>("trace_length");
  clipCutHigh = digConfig.get<int>("clip_cut_high");
  clipCutLow = digConfig.get<int>("clip_cut_low");
  separateBaselineFit = fitConfig.get<bool>("separate_baseline_fit");

  for(int i = 0; i<nParameters;++i){
    lpg.push_back(0);
  }
  for(int i = 0; i < fitLength; ++i){
    isGoodPoint.push_back(false);
  }
  
  
  scale = 0;
  baseline = 0;
  
  if(templateFit){
    templateFile = new TFile("fuzzyTemplateOut.root");
    templateSpline = (TSpline3*)templateFile->Get("masterSpline");
    errorSpline = (TSpline3*)templateFile->Get("errorSpline");
  }
  else{
    templateFile = NULL;
    errorSpline = NULL;
    templateSpline = NULL;
  }
}


pulseFitFunction::~pulseFitFunction(){
  if(templateFile!=NULL){
    delete templateSpline;
    delete errorSpline;
    templateFile->Close();
    delete templateFile;
  }
}

pulseFitter::pulseFitter(char* config, bool templateFit):
  func(config, templateFit),
  f(),
  xPoints(0),
  initialParGuesses(0),
  parSteps(0),
  parMins(0),
  parMaxes(0),
  parNames(0),
  freeParameter(0)
{
  
  pulseFitStart = func.getPulseFitStart();
  fitLength = func.getFitLength();
  xPoints.resize(func.getTraceLength());
  for(int i = 0; i<func.getTraceLength(); ++i){
    xPoints[i] = static_cast<float>(i);
  }
  
  boost::property_tree::ptree conf;
  read_json(config, conf);
  auto fitConfig = conf.get_child("fit_specs");
  drawFit = fitConfig.get<bool>("draw");

  waveform = new TF1("fit", &func, 0, func.getTraceLength(), func.getNParameters());
 
  waveform->SetNpx(10000);
  wwaveform = new ROOT::Math::WrappedMultiTF1(*waveform,1);
  f.SetFunction(*wwaveform);

  auto nameTree = fitConfig.get_child("parameter_names");
  for(const auto& tree : nameTree){
    parNames.push_back(tree.second.get<string>(""));
  }
  auto guessTree = fitConfig.get_child("parameter_guesses");
  for(const auto& tree : guessTree){
    initialParGuesses.push_back(tree.second.get<double>(""));
  }
  auto stepTree = fitConfig.get_child("parameter_steps");
  for(const auto& tree : stepTree){
    parSteps.push_back(tree.second.get<double>(""));
  }
 auto minTree = fitConfig.get_child("parameter_mins");
  for(const auto& tree : minTree){
    parMins.push_back(tree.second.get<double>(""));
  }
  auto maxTree = fitConfig.get_child("parameter_maxes");
  for(const auto& tree : maxTree){
    parMaxes.push_back(tree.second.get<double>(""));
  }
  auto fixedTree = fitConfig.get_child("free_parameter");
  for(const auto& tree : fixedTree){
    freeParameter.push_back(tree.second.get<bool>(""));
  }
  f.Config().MinimizerOptions().SetPrintLevel(0);
  //f.Config().MinimizerOptions().SetTolerance(1);
  
}

pulseFitter::~pulseFitter(){
  delete waveform;
  delete wwaveform;
}


double pulseFitter::fitDouble(float* const trace, double error){
  func.setDoubleFit(true);
  return fitPulse(trace,error,false);
}

double pulseFitter::fitDouble(unsigned short* const trace, double error){
  vector<float> floatTrace(func.getTraceLength());
  for(int i = 0; i < func.getTraceLength(); ++i){
    floatTrace[i] = static_cast<float>(trace[i]);
  }
  func.setDoubleFit(true);
  return fitPulse(&floatTrace[0],error,false);
}

double pulseFitter::fitSingle(float* const trace, double error){
  func.setDoubleFit(false);
  return fitPulse(trace,error,true);
}
double pulseFitter::fitSingle(unsigned short* const trace, double error){
  vector<float> floatTrace(func.getTraceLength());
  for(int i = 0; i < func.getTraceLength(); ++i){
    floatTrace[i] = static_cast<float>(trace[i]);
}
  func.setDoubleFit(false);
  return fitPulse(&floatTrace[0],error,true);
}

double pulseFitter::fitPulse(float* const trace, double error, 
			     bool isSingleFit){

  func.setError(error);  

  for(int i = 0; i<func.getNParameters(); ++i){
    if((i!=1)&&freeParameter[i]){
      f.Config().ParSettings(i).Set(parNames[i].c_str(),
				    initialParGuesses[i],
				    parSteps[i],
				    parMins[i],
				    parMaxes[i]);
    }
    else if(i==1){
      if(isSingleFit){
	f.Config().ParSettings(1).Set("Delta T",0);
      }
      else{
	f.Config().ParSettings(i).Release();
	f.Config().ParSettings(i).Set(parNames[i].c_str(),
				      initialParGuesses[i],
				      parSteps[i],
				      parMins[i],
				      parMaxes[i]);
      }
    }
    else{
      f.Config().ParSettings(i).Set(parNames[i].c_str(),initialParGuesses[i]);
    }      
  }

  f.FitFCN(func.getNParameters(),func,0,func.setTrace(trace),true);

  
  ROOT::Fit::FitResult fitRes = f.Result();

  wasValid = fitRes.IsValid();

  chi2 = fitRes.Chi2()/fitRes.Ndf();
  waveform->SetParameters(fitRes.GetParams());

  //for outputting and drawing (for debugging purposes)
  if(drawFit){
    TGraphErrors* traceGraph = new TGraphErrors(func.getTraceLength(), &xPoints[0], trace,NULL,NULL);
    TFile* outf = new TFile("fitTrace.root","recreate");
    fitRes.Print(cout);
    if(isSingleFit){
      cout << "Scale: " << getScale() << endl;
      cout << "Baseline: " << getBaseline() << endl;
      // cout << "Integral: " << getIntegral(0,func.getTraceLength()) << endl;
      cout << "Analogue sum: " << getSum(trace, pulseFitStart, fitLength+20) << endl;
    }
    
    if(!isSingleFit){
      cout << "Scale: " << getScale() << endl;
      cout << "Baseline: " << getBaseline() << endl;
      cout << "Ratio: " << getRatio() << endl;
      cout << "Integral: " << getIntegral(0,func.getTraceLength()) << endl;
      cout << "Analogue sum: " << getSum(trace, pulseFitStart, fitLength+20) << endl;
    }
    TCanvas* c1 = new TCanvas("c1", "c1",0,0,1800,900);
    traceGraph->SetMarkerStyle(20);
    waveform->SetTitle(TString::Format(";time [%.2f nsec]; ADC Counts", 1.0/func.getSampleRate()));
    waveform->GetXaxis()->SetRangeUser(pulseFitStart,pulseFitStart+fitLength);
    cout << "Param: " << waveform->GetParameter(0) << endl;
    waveform->Draw();
    traceGraph->Draw("psame");
    c1->Draw();
    c1->Modified();
    c1->Update();
    gSystem->ProcessEvents();
    traceGraph->Write();
    waveform->Write();
    c1->Write();
    outf->Write();
    cin.ignore();
    delete c1;
    delete traceGraph;
    delete outf; 
  }
  return chi2;
}

double pulseFitter::getIntegral(double start, double length) const{
  if(start<0||((start+length)>func.getTraceLength())){
    cout << "Error in integral: invalid limits. " << endl;
    return 0;
  }
  return waveform->Integral(start,start+length)-getBaseline()*length;
}

double pulseFitter::getSum(float* const trace, int start, int length){
  return func.getSum(trace, start, length);
}

double pulseFitter::getSum(unsigned short* const trace, int start, int length){
  vector<float> floatTrace(func.getTraceLength());
  for(int i = 0; i < func.getTraceLength(); ++i){
    floatTrace[i] = static_cast<float>(trace[i]);
  }
  return func.getSum(&floatTrace[0],start,length);
}

double pulseFitFunction::getSum(float* const trace, int start, int length){
  setTrace(trace);
  if(start<0||((start+length)>getTraceLength())){
    cout << "Error in sum: invalid limits. " << endl;
    return 0;
  }
  double runningSum = 0;
  for(int i = 0; i < length; ++i){
    runningSum = runningSum + currentTrace[start+i] - baseline;
  }
  return runningSum;
}

//the function used to define TF1's in root
double pulseFitFunction::operator() (double* x, double* p){
  double pulse;
 
  pulse = scale*evalPulse(x[0], p[0]);
  if(isDoubleFit)
    pulse = pulse + pileUpScale*evalPulse(x[0],p[0]+p[1]);
  
 
  pulse = pulse + baseline;

  return pulse;
  
}

//the chi2 function to be minimized
double pulseFitFunction::operator() (const double* p){
  //check if parameters have been updated
  //if they have, update the scale parameters
  bool updatedParameter = false;
  for(int i = 0; (!updatedParameter)&&i<nParameters; ++i){
    updatedParameter = !(lpg[i] == p[i]);
  }

  if(updatedParameter){
    for(int i = 0; i <nParameters; ++i){
      lpg[i] = p[i];
    }
    
    if(separateBaselineFit){
      updateScale();
    }
    else{
      updateScaleandPedestal();
    }
  }

  double runningSum = 0;
  double diff;
  double x[1];
  double thisPoint;
  for(int i = 0; i <fitLength; ++i){
    x[0] = pulseFitStart+i;
    thisPoint = currentTrace[pulseFitStart+i];
    if(isGoodPoint[i]){
      diff = thisPoint-(*this)(x,&lpg[0]);
      // double thisError = scale*errorSpline->Eval(x[0]-lpg[0]);
      //runningSum = runningSum + diff*diff/thisError/thisError;
      runningSum = runningSum + diff*diff/error/error;
    }
  }		       

  return runningSum;
}

double pulseFitFunction::dotProduct(const vector<double>& v1, const vector<double>& v2){
  double runningSum = 0;
  for(int i = 0; i < fitLength; ++i){
    runningSum = runningSum + v1[i]*v2[i];
  }
  return runningSum;
}

double pulseFitFunction::componentSum(const vector<double>& v){
  double runningSum = 0;
  for(int i = 0; i < fitLength; ++i){
    runningSum = runningSum + v[i];
  }
  return runningSum;
}

int pulseFitFunction::checkPoints(){
  int nGoodPoints = 0;
  for(int i = 0; i<fitLength; ++i){
    float thisPoint = currentTrace[i+pulseFitStart];
    bool goodPoint = (thisPoint<clipCutHigh)&&(thisPoint>clipCutLow);
    isGoodPoint[i] = goodPoint;
    if(goodPoint)
      nGoodPoints++;
  }
  nPoints = nGoodPoints;
  return nGoodPoints;
}


//this function finds values for the scales of the first and second pulses
//that minimizes the chi^2 for the current time guesses
void pulseFitFunction::updateScaleandPedestal(){
  vector<double> p(fitLength);
  vector<double> t(fitLength);

  for(int i = 0; i < fitLength; ++i){
    if(isGoodPoint[i]){
	p[i] = currentTrace[pulseFitStart+i];
	t[i] = evalPulse(pulseFitStart+i, lpg[0]);
    }
    else{
      p[i] = 0;
      t[i] = 0;
    }
  }
  
  if(!isDoubleFit){
    double a = dotProduct(t,t);
    double d = dotProduct(p,t);
    double f = componentSum(p);
    double g = componentSum(t);
    scale = (f*g-d*nPoints)/(g*g-a*nPoints);
    baseline = (a*f-d*g)/(a*nPoints-g*g);
  }
  
  else{
    vector<double> t2(fitLength);
    for(int i = 0; i < fitLength; ++i){
      if(isGoodPoint[i])
	t2[i] = evalPulse(pulseFitStart+i,lpg[0]+lpg[1]);
      else
	t2[i] = 0;
    }
    double a = dotProduct(t,t);
    double b = dotProduct(t,t2);
    double c = dotProduct(t2,t2);
    double d = dotProduct(p,t);
    double e = dotProduct(p,t2);
    double f = componentSum(p);
    double g = componentSum(t);
    double h = componentSum(t2);
    
    scale = (c*f*g-b*f*h-e*g*h+d*h*h-c*d*nPoints+b*e*nPoints)/
      (c*g*g-2*b*g*h+a*h*h+b*b*nPoints-a*c*nPoints);

    pileUpScale = (e*g*g-b*f*g+a*f*h-d*g*h+b*d*nPoints-a*e*nPoints)/
      (a*h*h+b*b*nPoints-2*b*g*h+c*(g*g-a*nPoints));
   
    baseline = (b*b*f-a*c*f+c*d*g+a*e*h-b*(e*g+d*h))/
      (-2*b*g*h+a*h*h+b*b*nPoints+c*(g*g-a*nPoints));

  }
}

//this function finds values for the scales and pedestal of the first and second pulses
//that minimizes the chi^2 for the current time guesses
//both functions exist in meantime for testing purposes
void pulseFitFunction::updateScale(){
  vector<double> p(fitLength);
  vector<double> t(fitLength);

  for(int i = 0; i < fitLength; ++i){
    if(isGoodPoint[i]){
      p[i] = (currentTrace[pulseFitStart+i]-baseline)/*/errorSpline->Eval(pulseFitStart+i-lpg[0])*/;
      t[i] = evalPulse(pulseFitStart+i, lpg[0])/*/errorSpline->Eval(pulseFitStart+i-lpg[0])*/;
    }
    else{
      p[i] = 0;
      t[i] = 0;
    }
  }
  
  if(!isDoubleFit){
    scale = dotProduct(p,t)/dotProduct(t,t);
  }
  
  else{
    vector<double> t2(fitLength);
    for(int i = 0; i < fitLength; ++i){
      if(isGoodPoint[i])
	t2[i] = evalPulse(pulseFitStart+i,lpg[0]+lpg[1]);
      else
	t2[i] = 0;
    }
    double a = dotProduct(t,t);
    double b = dotProduct(t,t2);
    double c = dotProduct(t2,t2);
    double d = dotProduct(p,t);
    double e = dotProduct(p,t2);
    scale = (b*e-c*d)/(b*b-a*c);
    pileUpScale = (b*d-a*e)/(b*b-a*c);
  }
}


//for finding the baseline separate from the pulse
void pulseFitFunction::findBaseline(){
  int effectiveFitLength = fitLength;
  double runningSum = 0;
  for(int i = 0; i <fitLength; ++i){
    int thisIndex= pulseFitStart-fitLength-10+i; 
    if(thisIndex>=0&&thisIndex<=traceLength)
      runningSum = runningSum + currentTrace[thisIndex];
    else
      effectiveFitLength--;
  }
  baseline = runningSum/effectiveFitLength;
}
