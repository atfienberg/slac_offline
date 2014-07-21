italianFilterCalib(){
  const int numRuns = 6;
  const char* files[numRuns] = {"../crunchedFiles/run_00240_crunched.root",
				"../crunchedFiles/run_00241_crunched.root",
				"../crunchedFiles/run_00242_crunched.root",
				"../crunchedFiles/run_00243_crunched.root",
				"../crunchedFiles/run_00244_crunched.root",
				"../crunchedFiles/run_00246_crunched.root"};
				
  
  gStyle->SetOptFit(1);

  vector<double> means;
  vector<double> sigs;
  vector<double> meanErrs;
  vector<double> sigErrs;

  for (int i = 0; i < numRuns; ++i){
    TFile* f = new TFile(files[i]);
    TTree* t = (TTree*) f->Get("t");
    TCanvas* c1 = new TCanvas();
    t->Draw("lensSipm1.energy>>h");
    h->Fit("gaus");
    means.push_back(h->GetFunction("gaus")->GetParameter(1));
    sigs.push_back(h->GetFunction("gaus")->GetParameter(2));
    meanErrs.push_back(h->GetFunction("gaus")->GetParError(1));
    sigErrs.push_back(h->GetFunction("gaus")->GetParError(2));
    h->SetTitle(Form("%s : Fit", files[i]));
  }
  
  vector<double> variances;
  vector<double> varianceErrors;
  
  for(int i = 0; i < means.size(); ++i){
    variances.push_back(sigs[i]*sigs[i]);
    varianceErrors.push_back(2*sigs[i]*sigErrs[i]);

  }

  new TCanvas();
  TGraphErrors* linGraph = new TGraphErrors(means.size(), &means[0], &variances[0], 
					    &meanErrs[0], &varianceErrors[0]);
  TGraphErrors* quadGraph = new TGraphErrors(means.size(), &means[0], &variances[0], 
					    &meanErrs[0], &varianceErrors[0]);
  
  linGraph->SetMarkerStyle(20);
  linGraph->Fit("pol1");
  linGraph->SetTitle("Fit");
  linGraph->GetFunction("pol1")->SetLineColor(kBlue);
  linGraph->GetXaxis()->SetTitle("Mean");
  linGraph->GetYaxis()->SetTitle("#sigma^{2}");
  quadGraph->SetMarkerStyle(20);
  quadGraph->Fit("pol2");
  quadGraph->GetFunction("pol2")->SetLineColor(kRed);
 
  linGraph->Draw("ap");
  quadGraph->Draw("psame");

}
