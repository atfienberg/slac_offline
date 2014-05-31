/* 
Aaron Fienberg
fienberg@uw.edu
class for fitting traces
*/  

#ifndef PULSEFIT
#define PULSEFIT
#include "TF1.h"
#include "Math/WrappedMultiTF1.h"
#include "Fit/Fitter.h"
#include "TFile.h"
#include "TSpline.h"
#include <vector>
#include <string>
  
/*class used for fitting traces. as of now, the fitting window is hard coded in. all traces are expected to be of length
  1024. The class must be constructed with a valid config file (.json format) 
*/
class pulseFitter{
public:
  pulseFitter(char* config, bool templateFit = false);
  ~pulseFitter();
  
  /*attempts to fit trace with a single pulse.
    Error is the uncertainty on each point in the trace (right now this only works if all errors are the same) */
  double fitSingle(float* const trace, double error = 1);
  double fitSingle(unsigned short* const trace, double error = 1);

  //same as above, except for a double pulse fit.
  double fitDouble(float* const trace, double error = 1);
  double fitDouble(unsigned short* const trace, double error = 1);


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
  double getMaximum() const { return waveform->GetMaximum(0,func.getTraceLength()); }
  double getMinimum() const { return waveform->GetMinimum(0,func.getTraceLength()); }

  //to get analogue sum without doing a fit first
  double getSum(float* const trace, int start, int length);
  double getSum(unsigned short* const trace, int start, int length);

private:

  /* the function that the pulseFitter uses to fit traces (with the defined () operator)  */
  class pulseFitFunction {
  public:
    pulseFitFunction(char* config, bool templateFit);
    ~pulseFitFunction();
    double operator() (double* x, double* p);
    double operator() (const double* p);
  
    //returns number of good data points in the range
    int setTrace(float* const trace) {currentTrace = trace; findBaseline(); return checkPoints();}
  
  
    void setDoubleFit(bool isDouble) {isDoubleFit = isDouble;}
    void setError(double err) {error = err;}
    int getPulseFitStart() const {return pulseFitStart;}
    int getFitLength() const {return fitLength;}
    int getNParameters() const {return nParameters;}
    int getTraceLength() const {return traceLength;}
    double getScale() const { return scale; }
    double getPileUpScale() const { return pileUpScale; }
    double getRatio() const { return pileUpScale/scale; }
    double getBaseline() const { return baseline; }
    double getSampleRate() const { return sampleRate; }

    double getSum(float* const trace, int start, int length); 
  private:
    //private helper functions
    double evalPulse(double t, double t0);
    double dotProduct(const std::vector<double>& v1, const std::vector<double>& v2);
    double componentSum(const std::vector<double>& v);
    int checkPoints();
    void updateScale();
    void findBaseline();
    void updateScaleandPedestal();

    double sampleRate;
 
    int nParameters;
    std::vector<double> lpg; //last parameter guesses

    double error;
    double scale;
    double pileUpScale;
    double baseline;

    bool separateBaselineFit;
    bool isDoubleFit;

    float* currentTrace;
    std::vector<bool> isGoodPoint;
    int traceLength, pulseFitStart, fitLength;
    int clipCutHigh;
    int clipCutLow;
    int nPoints; //number of points used in the fit

    TFile* templateFile;
    TSpline3* templateSpline;
    TSpline3* errorSpline;
    double templateLength = 200.0;
  };
  
  pulseFitFunction func;
  ROOT::Fit::Fitter f;

  double fitPulse(float* const trace, double error, 
		  bool isSingleFit);

  std::vector<float> xPoints;
  int pulseFitStart, fitLength;

  bool drawFit;

  std::vector<double> initialParGuesses;
  std::vector<double> parSteps;
  std::vector<double> parMins;
  std::vector<double> parMaxes;
  std::vector<std::string> parNames;
  std::vector<bool> freeParameter;
  double chi2;
  bool wasValid;

  TF1* waveform;
  ROOT::Math::WrappedMultiTF1* wwaveform;
  
};

#endif
