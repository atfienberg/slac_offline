
void pmtFilterCalib(){
  const int numRuns = 7;
  const char* files[numRuns] = {
				"../crunchedFiles/run_00111_crunched_filtered.root",
				"../crunchedFiles/run_00112_crunched_filtered.root",
				"../crunchedFiles/run_00113_crunched_filtered.root",
				"../crunchedFiles/run_00114_crunched_filtered.root",
				"../crunchedFiles/run_00115_crunched_filtered.root",
				"../crunchedFiles/run_00116_crunched_filtered.root",
				"../crunchedFiles/run_00118_crunched_filtered.root"};
				
  
  gStyle->SetOptFit(1);

  vector<double> means[1];
  vector<double> sigs[1];
  vector<double> meanErrs[1];
  vector<double> sigErrs[1];

  for (int i = 0; i < numRuns; ++i){
    TFile* f = new TFile(files[i]);
    TTree* t = (TTree*) f->Get("t");
    for (int j = 0; j < 1; ++j){
      t->Draw(Form("pmt.energy>>h%i",j+2),"pmt.valid");
      TH1F* h = (TH1F*) gROOT->FindObject(Form("h%i",j+2));
      h->Fit("gaus","0");
      means[j].push_back(h->GetFunction("gaus")->GetParameter(1));
      sigs[j].push_back(h->GetFunction("gaus")->GetParameter(2));
      meanErrs[j].push_back(h->GetFunction("gaus")->GetParError(1));
      sigErrs[j].push_back(h->GetFunction("gaus")->GetParError(2));
    }
  }
  
  vector<double> variances[1];
  vector<double> varianceErrors[1];
  
  for(int i = 0; i < means[0].size(); ++i){
    for(int j = 0; j < 1; ++j){
      variances[j].push_back(sigs[j][i]*sigs[j][i]);
      varianceErrors[j].push_back(2*sigs[j][i]*sigErrs[j][i]);
    }
  }

  TCanvas* c1 = new TCanvas();
  // c1->Divide(4,2);

  cout << "Makin graphs" << endl;
  for(int j = 0; j < 1; ++j){
    // c1->cd(j+1);

    TGraphErrors* linGraph = new TGraphErrors(means[j].size(), &means[j][0], &variances[j][0], 
					      &meanErrs[j][0], &varianceErrors[j][0]);
    TGraphErrors* quadGraph = new TGraphErrors(means[0].size(), &means[j][0], &variances[j][0], 
					       &meanErrs[j][0], &varianceErrors[j][0]);
  
    linGraph->SetMarkerStyle(20);
    linGraph->Fit("pol1");
    linGraph->SetTitle(Form("pmt%i", j+2));
    linGraph->GetFunction("pol1")->SetLineColor(kBlue);
    linGraph->GetXaxis()->SetTitle("Mean");
    linGraph->GetYaxis()->SetTitle("#sigma^{2}");
    quadGraph->SetMarkerStyle(20);
    quadGraph->Fit("pol2");
    quadGraph->GetFunction("pol2")->SetLineColor(kRed);
 
    linGraph->Draw("ap");
    quadGraph->Draw("psame");
    new TCanvas();
  }
   
}
