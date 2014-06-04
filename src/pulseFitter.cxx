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



using namespace std;


//A convolution of two exponential ramps and decays, one for the source and one for the device
//lpg[2] device ramp
//lpg[3] device decay
//lpg[4] src ramp
//lpg[5] src decay
double pulseFitter::pulseFitFunction::evalPulse(double t, double t0){
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
}

/*
//A convolution of an exponential ramp and decay (device) with a gaussian (source)
//lpg[2] is light width
//lpg[3] is device decay constant
//lpg[4] is device device ramp constant
double pulseFitter::pulseFitFunction::evalPulse(double t, double t0){
  double term1 = exp((lpg[2]*lpg[2]+2.0*t0*lpg[3]-2.0*t*lpg[3])/(2*lpg[3]*lpg[3]))*
    erfc((lpg[2]*lpg[2]+(t0-t)*lpg[3])/(1.41421*lpg[2]*lpg[3]));
  
  double term2 = -1*exp((lpg[4]+lpg[3])*(2.0*(t0-t)*lpg[4]*lpg[3]+lpg[2]*lpg[2]*(lpg[4]+lpg[3]))/(2.0*lpg[4]*lpg[4]*lpg[3]*lpg[3]))*
    erfc(((t0-t)*lpg[4]*lpg[3]+lpg[2]*lpg[2]*(lpg[4]+lpg[3]))/(1.41421*lpg[2]*lpg[4]*lpg[3]));

  return (term1+term2);
}
*/

/*
//template fit
double pulseFitter::pulseFitFunction::evalPulse(double t, double t0){
  if((t-t0)>0&&(t-t0)<templateLength)
    return templateSpline->Eval(t-t0);
  else
    return 0;
}
*/

//constructor for the pulseFitFunctionClass, must be constructed with a config file
pulseFitter::pulseFitFunction::pulseFitFunction(char* config, bool templateFit){
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

  //initialize some member variables
  lpg.resize(nParameters);
  isGoodPoint.resize(fitLength);  
  scale = 0;
  baseline = 0;
  
  //If it's a template fit, read the template file
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

//Destructor to relase dynamically allocated memory
pulseFitter::pulseFitFunction::~pulseFitFunction(){
  if(templateFile!=NULL){
    delete templateSpline;
    delete errorSpline;
    templateFile->Close();
    delete templateFile;
  }
}

//constructor for the pulseFitter class, must be constructed with a config file
pulseFitter::pulseFitter(char* config, bool templateFit):
  func(config, templateFit)
{

  pulseFitStart = func.getPulseFitStart();
  fitLength = func.getFitLength();

  //xpoints is just a vector of doubles from 0 to traceLength-1, used to make TGraphs
  xPoints.resize(func.getTraceLength());
  for(int i = 0; i<func.getTraceLength(); ++i){
    xPoints[i] = static_cast<double>(i);
  }
					       
  //read config file
  boost::property_tree::ptree conf;
  read_json(config, conf);
  auto fitConfig = conf.get_child("fit_specs");
  drawFit = fitConfig.get<bool>("draw");

  waveform = new TF1("fit", &func, 0, func.getTraceLength(), func.getNParameters());
 
  waveform->SetNpx(10000);
  wwaveform = new ROOT::Math::WrappedMultiTF1(*waveform,1);
  f.SetFunction(*wwaveform);

  //read arrays from the config file
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

//pulseFitter destructor
pulseFitter::~pulseFitter(){
  delete waveform;
  delete wwaveform;
}

//tries to fit a double pulse 
double pulseFitter::fitDouble(double* const trace, double error){
  func.setDoubleFit(true);
  return fitPulse(trace,error,false);
}

//overloaded to work with unsigned shorts
double pulseFitter::fitDouble(unsigned short* const trace, double error){
  vector<double> doubleTrace(func.getTraceLength());
  for(int i = 0; i < func.getTraceLength(); ++i){
    doubleTrace[i] = static_cast<double>(trace[i]);
  }
  func.setDoubleFit(true);
  return fitPulse(&doubleTrace[0],error,false);
}

//tries to fit a single pulse
double pulseFitter::fitSingle(double* const trace, double error){
  func.setDoubleFit(false);
  return fitPulse(trace,error,true);
}

//overloaded for unsigned shorts
double pulseFitter::fitSingle(unsigned short* const trace, double error){
  vector<double> doubleTrace(func.getTraceLength());
  for(int i = 0; i < func.getTraceLength(); ++i){
    doubleTrace[i] = static_cast<double>(trace[i]);
}
  func.setDoubleFit(false);
  return fitPulse(&doubleTrace[0],error,true);
}

//function that sets initial parameters for ROOT fitter and then calls the fitter
double pulseFitter::fitPulse(double* const trace, double error, 
			     bool isSingleFit){

  func.setError(error);  

  //set initial parameters based on whether it is a single or a double fit. 
  //parameter 1 should be delta t
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

  //call the minimizer
  f.FitFCN(func.getNParameters(),func,0,func.setTrace(trace),true);

  ROOT::Fit::FitResult fitRes = f.Result();

  //grab the results
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

//numerically integrate the fitted function
double pulseFitter::getIntegral(double start, double length) const{
  if(start<0||((start+length)>func.getTraceLength())){
    cerr << "Error in integral: invalid limits. " << endl;
    return 0;
  }
  return waveform->Integral(start,start+length)-getBaseline()*length;
}

//take the baseline corrected sum of digitized points in specified range
//this function is a wrapper that serves to pass the trace information to
//the pulseFitFunction
double pulseFitter::getSum(double* const trace, int start, int length){
  return func.getSum(trace, start, length);
}

//overloaded to work with unsigned shorts
double pulseFitter::getSum(unsigned short* const trace, int start, int length){
  vector<double> doubleTrace(func.getTraceLength());
  for(int i = 0; i < func.getTraceLength(); ++i){
    doubleTrace[i] = static_cast<double>(trace[i]);
  }
  return func.getSum(&doubleTrace[0],start,length);
}

//this function actually executes the sum
double pulseFitter::pulseFitFunction::getSum(double* const trace, int start, int length){
  //check the bounds
  if(start<0||((start+length)>getTraceLength())){
    cerr << "Error in sum: invalid limits. " << endl;
    return 0;
  }
  
  setTrace(trace);
  double runningSum = 0;
  for(int i = 0; i < length; ++i){
    runningSum = runningSum + currentTrace[start+i] - baseline;
  }
  return runningSum;
}

//the function used to define TF1's in root
double pulseFitter::pulseFitFunction::operator() (double* x, double* p){
  double pulse;
 
  pulse = scale*evalPulse(x[0], p[0]);
  if(isDoubleFit)
    pulse = pulse + pileUpScale*evalPulse(x[0],p[0]+p[1]);
  
 
  pulse = pulse + baseline;

  return pulse;
  
}

//the chi2 function to be minimized
double pulseFitter::pulseFitFunction::operator() (const double* p){
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

  //evaluate the chi2
  double runningSum = 0;
  double diff;
  double x[1];
  double thisPoint;
  for(int i = 0; i <fitLength; ++i){
    x[0] = pulseFitStart+i;
    thisPoint = currentTrace[pulseFitStart+i];
    if(isGoodPoint[i]){
      diff = thisPoint-(*this)(x,&lpg[0]);
      runningSum = runningSum + diff*diff/error/error;
    }
  }		       

  return runningSum;
}

//take a dot product of two vectors
double pulseFitter::pulseFitFunction::dotProduct(const vector<double>& v1, const vector<double>& v2){
  double runningSum = 0;
  for(int i = 0; i < fitLength; ++i){
    runningSum = runningSum + v1[i]*v2[i];
  }
  return runningSum;
}

//take a sum of the components of a vector
double pulseFitter::pulseFitFunction::componentSum(const vector<double>& v){
  double runningSum = 0;
  for(int i = 0; i < fitLength; ++i){
    runningSum = runningSum + v[i];
  }
  return runningSum;
}

//check that each point in the trace is within the defined limits
//if not, flag it as as bad point
int pulseFitter::pulseFitFunction::checkPoints(){
  int nGoodPoints = 0;
  for(int i = 0; i<fitLength; ++i){
    double thisPoint = currentTrace[i+pulseFitStart];
    bool goodPoint = (thisPoint<clipCutHigh)&&(thisPoint>clipCutLow);
    isGoodPoint[i] = goodPoint;
    if(goodPoint)
      nGoodPoints++;
  }
  nPoints = nGoodPoints;
  return nGoodPoints;
}


//this function finds values for the scales of the first and second pulses
//that minimizes the chi^2 for the current parameter guesses
void pulseFitter::pulseFitFunction::updateScaleandPedestal(){
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
  
  //if it is a double pulse fit
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

//this function finds values for the function scales 
//that minimizes the chi^2 for the current parameter guesses
void pulseFitter::pulseFitFunction::updateScale(){
  vector<double> p(fitLength);
  vector<double> t(fitLength);

  for(int i = 0; i < fitLength; ++i){
    if(isGoodPoint[i]){
      p[i] = (currentTrace[pulseFitStart+i]-baseline);
      t[i] = evalPulse(pulseFitStart+i, lpg[0]);
    }
    else{
      p[i] = 0;
      t[i] = 0;
    }
  }
  
  if(!isDoubleFit){
    scale = dotProduct(p,t)/dotProduct(t,t);
  }
  
  //if it is a double pulse fit
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


//for finding the baseline separate from the pulse,
//it fits an island [pulsefitStart-fitLength-10,pulseFitStart-10)
//the 10 is currently hardcoded
void pulseFitter::pulseFitFunction::findBaseline(){
  int effectiveFitLength = fitLength;
  double runningSum = 0;
  for(int i = 0; i <fitLength; ++i){
    int thisIndex= pulseFitStart-fitLength-10+i; 
    
    //make sure I don't try to access out of bounds 
    if(thisIndex>=0&&thisIndex<=traceLength)
      runningSum = runningSum + currentTrace[thisIndex];
    else
      effectiveFitLength--;
  }
  baseline = runningSum/effectiveFitLength;
}
