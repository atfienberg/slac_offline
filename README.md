Pulsefitter
===========
Aaron Fienberg
fienberg@uw.edu

This is a pulsefitting class I built to fit digitizer traces. It is intended to be useful for UW lab studies and test beam analysis.
It is intended to be realitively easy to adapt into a template or functional fitter with various types of functions

CONFIG FILES
------------

The fitter works with .json config files. See the example config files in the /configs folder.
The fitter is currently set up to work with the exampleSipmConfig.json. 

Other than the self explanatory entries, I will give special attention to the following:

* fit_type: The type of fit to be attempted. The current options are 'laser', 'beam', and 'template'. 

* clip_cut_high & clip_cut_low: any digitizer sample whose value is not between clip_cut_high and clip_cut_low will not be included in the fit. 

* free_parameter: whether this parameter will be varied in the minimization procedure. The second parameter should always be delta t (for double pulse fits). It will always be fixed if you call fitSingle() and it will always be free if you call fitDouble().

* separate_baseline_fit: whether the baseline is fit at the same time as the pulse or fit separately looking at a sample of trace before the defined fit_start of length base_fit_length

* base_fit_length: how many points to include in the separate baseline fit

* base_fit_buffer: how many points to leave between the end of the separate baseline fit region and the start of the pulse fit region

* draw: true means that the fitter will show you a plot of each fit and output the fit results as well as waiting for keyboard input before proceeding.


USING THE CLASS
---------------

As of now there is only one constructor and it takes one argument, the path to the config file.

See the src/exampleDriver.cxx for an example on how to use the fitter to fit a trace.

* fitSingle(trace) will fit a trace. Trace can be either an array of floats or an array of unsigned shorts. The array must be at least as long as the trace_length specified in the config file.

* fitSingle(trace, error) will do the same as above but assign each sample an error specified in the function call. The default is one if you don't specify.

* fitDouble(trace, error) is for fitting double pulses. It will free the Delta T parameter and attempt to fit for two pulses.

A couple handy getters:

* getFitStart(): returns the sample fit start that was read from the config file
* getFitLength(): returns the fit length that was read from the config file

After doing a fit, you can call the following functions to access fit results. 
**Do not call any of the following without first doing a fit**

* getParName(int i): returns name of i'th parameter.
* getParameter(int i): returns value of i'th parameter determined by the fitter.
* getScale(int i): returns the normalization factor of the pulse
* getTime(): returns time of pulse. Right now this is the same as getParameter(0);
* getBaseline(): returns the baseline of the trace.
* getChi2(): returns chi2/ndf.
* wasValidFit(): returns whether or not fitter converged successfuly
* getIntegral(double start, double length): numerical integral of fit function. This can be slow depending on your function
* getFunctionMaximum(): numerical maximum of fit function. This can be slow.
* getFunctionMinimum(): numerical minimum of fit function. This can be slow.

The fitter also has functionality to do a baseline corrected analogue sum of the trace:
* getSum(int start, int length): does baseline corrected analogue sum of "length" points starting with sample "start". Uses the last baseline estimation. 
* getSum(trace, int start, int length): calculates baseline and does baseline corrected analogue sum of "length" points starting with sample "start". Trace can be a float or an unsigned short.
* getMax(int start, int length): finds the largest sample between start and start+length and subtracts the baseline. Uses the last baseline estimation.
* getMax(trace, int start, int length): calculates the baseline and then returns baseline corrected maximum.
* getMin(int start, int length): same as above, except for min.
* getMin(trace, int start, int length): same as above, except for min.

A NOTE ON EXTRACTING ENERGY
---------------------------

There is no guarantee that any of the function parameters are a good energy proxy for the trace. The integral
is usually a good energy proxy, but it is also often slow to numerically integral the pulse.
Instead, it is better to determine how to extract a good enery proxy from your parameters.
The way to do this depends on your fit function.

In the case of a template fit, the scale ( getScale() ) is a good energy proxy.

If you are not doing a template fit, the normalization of your pulse may be correlated with the other
parameters that are also varying. This is a case where you have to be careful. I generally integrate the 
functional form of the pulse in advance to figure out how it depends on the parameters. This can often
be done easily with mathematica.  