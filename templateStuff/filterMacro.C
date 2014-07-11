using namespace std;

const int TRACELENGTH = 1024;

/*
//not great
void filterTrace(int* trace){
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
*/


const int filterLength = 10;
void filterTrace(int* trace){
  for(int i = 0; i < TRACELENGTH-filterLength; ++i){
    cout << trace[i] << endl;
    int runningSum = 0;
    for(int j = 0; j < filterLength; ++j){
      runningSum+=trace[i+j];
    }
    trace[i] = runningSum/filterLength;
    cout << trace[i] << endl;
  }
}

typedef struct {
  ULong64_t system_clock;
  ULong64_t device_clock[16];
  UShort_t trace[16][1024];
} drs;

void filterMacro(){
  drs d;
  TFile infile("../datafiles/run_00119.root");
  TTree* intree = (TTree*) infile.Get("t");
  intree->SetBranchAddress("caen_drs_0", &d.system_clock);
  intree->GetEntry(5);
  int xPoints[1024];
  int trace[1024];
  int filteredTrace[1024];
  for(int i = 0; i < 1024; ++i){
    xPoints[i] = i;
    trace[i] = d.trace[1][i];
    filteredTrace[i] = d.trace[1][i];
  }
  filterTrace(filteredTrace);

  TGraph* testg = new TGraph(1024, xPoints, trace);
  TGraph* testg2 = new TGraph(1024, xPoints, filteredTrace);
  testg->Draw();
  testg2->Draw("same");
  testg2->SetLineColor(kRed);
  cout << d.trace[1][400] << endl;
  cout << d.trace[1][700] << endl;
}
  
