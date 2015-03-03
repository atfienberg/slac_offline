/* 
Aaron Fienberg
fienberg@uw.edu
class for fitting traces
*/  

#ifndef PULSEFIT
#define PULSEFIT
#include "TF1.h"
#include "Math/Functor.h"
#include "Math/Minimizer.h"
#include "TFile.h"
#include "TSpline.h"
#include <vector>
#include <string>
  
/*class used for fitting traces. as of now, the fitting window is hard coded in. all traces are expected to be of length
  1024. The class must be constructed with a valid config file (.json format) 
*/
class pulseFitter{
public:
  pulseFitter(const std::string& config="configs/.defaultConfig.json");
  ~pulseFitter();
  
  /*attempts to fit trace with a single pulse.
    Error is the uncertainty on each point in the trace (right now this only works if all errors are the same) */
  double fitSingle(const double* trace, double error = 1);
  double fitSingle(const unsigned short* trace, double error = 1);
  double fitSingle(const float* trace, double error = 1);

  //same as above, except for a double pulse fit.
  double fitDouble(const double* trace, double error = 1);
  double fitDouble(const unsigned short* trace, double error = 1);
  double fitDouble(const float* trace, double error = 1);

  //get various fit results. Do not call these without doing a fit first
  double getNParameters() const { return func.getNParameters(); }
  double getParameter(int i) const { return waveform->GetParameter(i); }
  std::string getParName(int i) const { return parNames[i]; }
  double getScale() const { return func.getScale(); }
  double getTime() const { return waveform->GetParameter(0); }
  double getRatio() const { return func.getRatio(); }
  double getBaseline() const { return func.getBaseline(); }
  double getChi2() const { return chi2; }
  bool wasValidFit() const { return wasValid; }
  double getIntegral(double start, double length) const;

  double getFunctionMaximum() const { return waveform->GetMaximum(0,func.getTraceLength()); }
  double getFunctionMinimum() const { return waveform->GetMinimum(0,func.getTraceLength()); }

  //get back some fit parameters from the config file
  bool isFitConfigured() const { return fitConfigured; }
  int getFitStart() const { return func.getFitStart(); }
  int getFitLength() const { return func.getFitLength(); }
  std::string getFitType() const { return fitType; }

  //to get analogue sum without doing a fit first
  double getSum(const double* trace, int start, int length);
  double getSum(const unsigned short* trace, int start, int length);
  double getSum(const float* trace, int start, int length);
  
  //analogue sum using currently stored trace and baseline info
  double getSum(int start, int length);

  //to get max/min of trace in certain range without doing a fit first
  double getMax(const double* trace, int start, int length);
  double getMax(const unsigned short* trace, int start, int length);
  double getMax(const float* trace, int start, int length);
  double getMin(const double* trace, int start, int length);
  double getMin(const unsigned short* trace, int start, int length);
  double getMin(const float* trace, int start, int length);

  //max/min using currently stored trace and baseline info
  double getMax(int start, int length);
  double getMin(int start, int length);

  //fill fitTrace with chopped fit trace
  void fillFitTrace(double* fitTrace, int start, int length);

  //setters to update fit configuration dynamically
  void setFitStart(int start) {func.setFitStart(start);}
  void setFitLength(int length) {func.setFitLength(length);}
  void setParameterGuess(int parIndex, double guess) {initialParGuesses[parIndex] = guess;}
  void setParameterMin(int parIndex, double min) {parMins[parIndex] = min;}
  void setParameterMax(int parIndex, double max) {parMaxes[parIndex] = max;}
  

private:

  /* the function that the pulseFitter uses to fit traces (with the defined () operator)  */
  class pulseFitFunction {
  public:
    pulseFitFunction(const std::string& config);
    ~pulseFitFunction();
    double operator() (double* x, double* p);
    double chi2Function(const double* p);
  
    //returns number of good data points in the range
    int setTrace(const double* trace);
  
    //setters to update fit length and start dynamically
    void setFitStart(int start) {pulseFitStart = start;}
    void setFitLength(int length) {fitLength = length;}

    //finds baseline by looking at island before the trace
    void findBaseline();

    void setDoubleFit(bool isDouble) {isDoubleFit = isDouble;}
    void setError(double err) {error = err;}
    int getFitStart() const {return pulseFitStart;}
    int getFitLength() const {return fitLength;}
    int getNParameters() const {return lpg.size();}
    int getTraceLength() const {return traceLength;}
    double getScale() const { return scale; }
    double getPileUpScale() const { return pileUpScale; }
    double getRatio() const { return pileUpScale/scale; }
    double getBaseline() const { return baseline; }
    double getSampleRate() const { return sampleRate; }
    bool isSeparateBaselineFit() const { return separateBaselineFit; }

    double evalSum(int start, int length); 
    double findMax(int start, int length);
    double findMin(int start, int length);

    int getNGoodPoints() const { 
      int val = 0;
      for (unsigned int i = 0; i < isGoodPoint.size(); ++i){
	if (isGoodPoint[i]){
	  val++;
	}
      }
      return val;
    }
  private:
    //private helper functions
    double evalPulse(double t, double t0);
    double dotProduct(const std::vector<double>& v1, 
		      const std::vector<double>& v2); 
    double componentSum(const std::vector<double>& v);
    int checkPoints();
    void updateScale();
    void updateScaleandPedestal();
    
    //pointer to current fit function
    double (pulseFitter::pulseFitFunction::*currentFitFunction)(double, double);
    //pulse shape library
    double beamSource(double t, double t0);
    double laserSource(double t, double t0);
    double templateFit(double t, double t0);
    

    double sampleRate;
 
    std::vector<double> lpg; //last parameter guesses
     
    double error;
    double scale;
    double pileUpScale;
    double baseline;

    bool separateBaselineFit;
    bool isDoubleFit;

    const double* currentTrace;

    std::vector<bool> isGoodPoint;
    int traceLength, pulseFitStart, fitLength;
    int bFitLength, bFitBuffer; //baseline fit length, baseline fit buffer 
    int clipCutHigh;
    int clipCutLow;
    int nPoints; //number of points used in the fit

    TFile* templateFile;
    TSpline3* templateSpline;
    double templateLength;
    double templateBuffer;
  };  
  pulseFitFunction func;
  
  ROOT::Math::Minimizer* min; 
  ROOT::Math::Functor functor;

  double fitPulse(const double* trace, double error, 
		  bool isSingleFit);

  std::vector<double> xPoints;

  bool drawFit;
  bool fitConfigured;
  std::string fitType;

  std::vector<double> initialParGuesses;
  std::vector<double> parSteps;
  std::vector<double> parMins;
  std::vector<double> parMaxes;
  std::vector<std::string> parNames;
  std::vector<bool> freeParameter;
  double chi2;
  bool wasValid;

  //copy of trace as double in case it was entered as another data type 
  std::vector<double> doubleTrace;

  TF1* waveform;
  
};

#endif
