#makes template, assuming negative pulse. allows for rebinning because DRS needs it.
#version for readin in structs from slac
#Aaron Fienberg
#fienberg@uw.edu

from ROOT import *
import sys
import numpy as np
import matplotlib.pyplot as plt
import time


def getPseudoTime(trace):
    mindex = np.argmin(trace)
    #check for division by 0
    if trace[mindex]==trace[mindex+1]:
        return 1
    return 2/np.pi*np.arctan(float((trace[mindex-1]-trace[mindex]))/
                             (trace[mindex+1]-trace[mindex]))

def correctBaseline(trace):
    bufferZone = 40
    fitLength = 40
    #assuming negative pulse
    mindex = np.argmin(trace)
    m = trace[mindex]
    if mindex-fitLength-bufferZone<0:
        print("Baseline fit walked off the end of the trace!")
        return np.zeros(TEMPLATELENGTH)
    baseline = np.sum(trace[mindex-bufferZone-fitLength:mindex-bufferZone])/float(fitLength)
    shiftedTrace = np.array(trace[mindex-40:mindex+TEMPLATELENGTH-40],dtype='f')
    
    #make sure the array isn't too short, if it is toss it
    if len(shiftedTrace)<TEMPLATELENGTH:
        return np.zeros(TEMPLATELENGTH)
    #return the corrected trace
    return (shiftedTrace-baseline)/(m-baseline)

TEMPLATELENGTH  = 200    
NBINSPSEUDOTIME = 500
NTIMEBINS = 10
REBIN = False
def main():
    start_time = time.time()
    if len(sys.argv) != 2:
        print('need to input datafile.')
        sys.exit(1)
    
    #read in file
    gROOT.ProcessLine(\
        "struct s_sis{\
         unsigned long timestamp[8];\
         unsigned short trace[8*1024];\
         unsigned short is_bad_event;\
    };")
    from ROOT import s_sis
    
    sis = s_sis()
    infile = TFile(sys.argv[1])
    tree = infile.Get("t")
    tree.SetBranchAddress("sis",AddressOf(sis, "timestamp"))
    trace = np.zeros(1024,dtype=np.uint16)
    
    #evaluate pseudotimes
    pseudoTimesHist = TH1D("ptimes","ptimes",NBINSPSEUDOTIME,0,1)
    pseudoTimes = np.zeros(tree.GetEntries(),dtype=np.float)
    for i in xrange(0,tree.GetEntries()):
        tree.GetEntry(i)
        trace = np.array([sis.trace[j] for j in xrange(1024)], dtype=np.float) 
        pseudoTimes[i]=getPseudoTime(trace)
        pseudoTimesHist.Fill(pseudoTimes[i])
        if i % 1000 == 0:
            print(str(i) + " done") 
    pseudoTimesHist.Scale(1.0/pseudoTimesHist.Integral())   
    pseudoTimesHist.Draw()
    c2 = TCanvas("c2")
    
    #create conversion to real time
    realTimes = np.array([pseudoTimesHist.Integral(1,i+2) 
                                          for i in xrange(0,NBINSPSEUDOTIME)],dtype=np.float)
    realTimeGraph = TGraph(NBINSPSEUDOTIME,np.array([(i+1.0)/NBINSPSEUDOTIME
                                                      for i in xrange(0,NBINSPSEUDOTIME)],dtype=np.float),
                           realTimes)
    realTimeGraph.Draw("ap")
    rtSpline = TSpline3("realTimeSpline",realTimeGraph)
    rtSpline.Draw("same")
    c3 = TCanvas("c3")

    #populate timeslices
    timeslices = np.zeros((NTIMEBINS,TEMPLATELENGTH))
    for i in xrange(0,tree.GetEntries()):
        tree.GetEntry(i)
        trace = np.array([sis.trace[j] for j in xrange(1024)],dtype=np.float) 
        realTime = rtSpline.Eval(pseudoTimes[i])
        #find which timeslice this trace belongs in
        thisSlice = int(realTime*NTIMEBINS)
        if thisSlice == NTIMEBINS: thisSlice-=1
        ctrace = correctBaseline(trace)
        timeslices[thisSlice] = timeslices[thisSlice]+ctrace 
        if i % 1000 == 0:
            print(str(i) + " placed.")
    print("Timeslices populated.")
    
    #normalize timeslices
    for i in xrange(0,NTIMEBINS):
        integral = np.sum(timeslices[i])
        timeslices[i] = timeslices[i]/integral

    #rebin timeslices if neccesary
    rebinFactor = 1
    rtimeslices = np.zeros((NTIMEBINS,TEMPLATELENGTH/2))
    if REBIN:
        rebinFactor=2
        for i in xrange(0,NTIMEBINS):
            rtimeslices[i] = [(timeslices[i][j]+timeslices[i][j+1])/2 for j in xrange(0,TEMPLATELENGTH,2)]
        timeslices.resize(NTIMEBINS,TEMPLATELENGTH/2)
        timeslices = rtimeslices
    

    #build master template and output results 
    masterTemplate = np.zeros(NTIMEBINS*TEMPLATELENGTH/rebinFactor)
    for i in xrange(0,NTIMEBINS):
        masterTemplate[NTIMEBINS-1-i::NTIMEBINS]=timeslices[i]
    outf = TFile("structtemplate.root","recreate")
    masterGraph = TGraph(NTIMEBINS*TEMPLATELENGTH/rebinFactor,
                         np.array([i*1.0*rebinFactor/NTIMEBINS 
                                    for i in xrange(0,TEMPLATELENGTH*NTIMEBINS/rebinFactor)],dtype=np), 
           masterTemplate)
    masterGraph.Draw("ap")
    spline = TSpline3("master spline", masterGraph)
    spline.SetName("masterSpline")
    spline.Draw("same")
    masterGraph.Write()
    spline.Write()
    plt.plot(np.array([i*1.0*rebinFactor/NTIMEBINS 
                                    for i in xrange(0,TEMPLATELENGTH*NTIMEBINS/rebinFactor)],dtype=np.float),
             masterTemplate,)
    plt.xlabel('Time [2.0 ns]',x=1,horizontalalignment='right')
    plt.grid(True)
    plt.savefig('poleZeroTemplate.pdf',format='pdf')
    print time.time()-start_time, "seconds"
    var = raw_input("..... ")
    outf.Close()


#boilerplate to call main function
if __name__ == '__main__':
  main()
