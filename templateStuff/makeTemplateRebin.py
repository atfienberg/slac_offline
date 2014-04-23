#makes template, assuming negative pulse. allows for rebinning because DRS needs it.
#Aaron Fienberg
#fienberg@uw.edu

from ROOT import *
import math
from array import *
import sys

def getPseudoTime(trace):
    mindex = trace.index(min(trace))
    #check for division by 0
    if trace[mindex]-trace[mindex+1]==0:
        return 1
    return 2/math.pi*math.atan(float((trace[mindex]-trace[mindex-1]))/
                (trace[mindex]-trace[mindex+1]))

def correctBaseline(trace):
    bufferZone = 40
    fitLength = 40
    #assuming negative pulse
    m = min(trace)
    mindex = trace.index(m)
    if mindex-fitLength-bufferZone<0:
        print "Baseline fit walked off the end of the trace!"
        sys.exit(1)
    baseline = float(sum(trace[mindex-bufferZone-fitLength:mindex-bufferZone]))/fitLength
    shiftedTrace = trace[mindex-40:mindex+TEMPLATELENGTH-40]
    return map(lambda x: (x-baseline)/(m-baseline),shiftedTrace)

TEMPLATELENGTH  = 150    
NBINSPSEUDOTIME = 2000
NTIMEBINS = 1
REBIN = true
def main():
    if len(sys.argv) != 2:
        print 'need to input datafile.'
        sys.exit(1)
    
    #read in file
    infile = TFile(sys.argv[1])
    tree = infile.Get("WFDTree")
    trace = array('H',range(0,1024))
    tree.SetBranchAddress("Channel1",trace)
    
    #evaluate pseudotimes
    pseudoTimesHist = TH1F("ptimes","ptimes",NBINSPSEUDOTIME,0,1)
    pseudoTimes = []
    for i in range(0,tree.GetEntries()):
        tree.GetEntry(i)
        pseudoTimes.append(getPseudoTime(trace))
        pseudoTimesHist.Fill(pseudoTimes[i])
        if i % 1000 == 0:
            print str(i) + " done" 
    pseudoTimesHist.Scale(1.0/pseudoTimesHist.Integral())   
    pseudoTimesHist.Draw()
    c2 = TCanvas("c2")
    
    #create conversion to real time
    realTimes = array('f',[pseudoTimesHist.Integral(1,i+2) 
                                          for i in range(0,NBINSPSEUDOTIME) ])
    realTimeGraph = TGraph(NBINSPSEUDOTIME,array('f',[(i+1.0)/NBINSPSEUDOTIME
                                                      for i in range(0,NBINSPSEUDOTIME)]),
                           realTimes)
    realTimeGraph.Draw("ap")
    rtSpline = TSpline3("realTimeSpline",realTimeGraph)
    rtSpline.Draw("same")
    c3 = TCanvas("c3")

    #populate timeslices
    timeslices = [[0]*TEMPLATELENGTH for i in range(0,NTIMEBINS)]
    for i in range(0,tree.GetEntries()):
        tree.GetEntry(i)
        realTime = rtSpline.Eval(pseudoTimes[i])
        #find which timeslice this trace belongs in
        thisSlice = int(realTime*NTIMEBINS)
        if thisSlice == NTIMEBINS: thisSlice-=1
        ctrace = correctBaseline(trace)
        timeslices[thisSlice] = [timeslices[thisSlice][i]+ctrace[i] for i in range(0,TEMPLATELENGTH)]    

    #rebin timeslices if neccesary
    rebinFactor = 1
    rtimeslices = [[0]*(TEMPLATELENGTH/2) for i in range(0,NTIMEBINS)]
    if REBIN:
        rebinFactor=2
        for i in range(0,NTIMEBINS):
            rtimeslices[i] = [(timeslices[i][j]+timeslices[i][j+1])/2 for j in range(0,TEMPLATELENGTH,2)]
            timeslices[i] = rtimeslices[i]
    

    #build master template and output results 
    masterTemplate = [0]*(NTIMEBINS*TEMPLATELENGTH/rebinFactor)
    for i in range(0,NTIMEBINS):
        masterTemplate[NTIMEBINS-1-i::NTIMEBINS]=timeslices[i]
    outf = TFile("template.root","recreate")
    masterGraph = TGraph(NTIMEBINS*TEMPLATELENGTH/rebinFactor,
                         array('f',[i*1.0*rebinFactor/NTIMEBINS 
                                    for i in range(0,TEMPLATELENGTH*NTIMEBINS/rebinFactor)]),
                         array('f',masterTemplate))
    masterGraph.Draw("ap")
    spline = TSpline3("master spline", masterGraph)
    spline.SetName("masterSpline")
    spline.Draw("same")
    masterGraph.Write()
    spline.Write()

    var = raw_input("..... ")
    outf.Close()
if __name__ == '__main__':
  main()
