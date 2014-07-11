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

typedef struct {
  unsigned long system_clock;
  unsigned long device_clock[8];
  unsigned short trace[16][1024];
} drs;

void filterMacro(){
  TFile infile("../datafiles/run_00111.root");
  TTree* t = (TTree*) infile.Get("t");
  TBranch* branch = t->GetBranch("caen_drs_0");
  drs d;
  int xPoints[1024];
  for(int i = 0; i <1024; ++i){
    xPoints[i] = i;
  }
  branch->SetAddress(&d);
  cout << branch -> GetEntries() << " pulses." << endl;
  cout << "Entry #: " << endl;
  int entry;
  cin >> entry;
  t->GetEntry(entry);
  int unFiltered[1024];
  for(int i = 0; i < 1024; ++i){
    unFiltered[i] = d.trace[0][i];
  }
  TGraph* unfiltered = new TGraph(1024, xPoints, unFiltered);
  unfiltered->Draw();

  filterTrace(d.trace[0][i]);
  int filtered[1024];
  for(int i = 0; i < 1024; ++i){
    filtered[i] = ch1[i];
  }
  TGraph* filteredGraph = new TGraph(1024, xPoints, filtered);
  filteredGraph->SetMarkerStyle(20);
  filteredGraph->SetLineColor(kRed);
  filteredGraph->Draw("same");
  
  
  
}

