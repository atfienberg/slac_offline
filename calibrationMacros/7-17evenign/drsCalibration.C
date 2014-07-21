
void drsCalibration(){
  const int numRuns = 8;
  const char* files[numRuns] = {
				"../crunchedFiles/run_00111_crunched.root",
				"../crunchedFiles/run_00112_crunched.root",
				"../crunchedFiles/run_00113_crunched.root",
				"../crunchedFiles/run_00114_crunched.root",
				"../crunchedFiles/run_00115_crunched.root",
				"../crunchedFiles/run_00116_crunched.root",
				"../crunchedFiles/run_00118_crunched.root",
  				"../crunchedFiles/run_00119_crunched.root"};
				
  
  gStyle->SetOptFit(1);

  vector<double> means[7];
  vector<double> sigs[7];
  vector<double> meanErrs[7];
  vector<double> sigErrs[7];

  for (int i = 0; i < numRuns; ++i){
    TFile* f = new TFile(files[i]);
    TTree* t = (TTree*) f->Get("t");
    for (int j = 0; j < 7; ++j){
      t->Draw(Form("sipm5%i.energy>>h%i",j+2,j),Form("sipm5%i.valid",j+2));
      TH1F* h = (TH1F*) gROOT->FindObject(Form("h%i",j));
      h->Fit("gaus","0");
      means[j].push_back(h->GetFunction("gaus")->GetParameter(1));
      sigs[j].push_back(h->GetFunction("gaus")->GetParameter(2));
      meanErrs[j].push_back(h->GetFunction("gaus")->GetParError(1));
      sigErrs[j].push_back(h->GetFunction("gaus")->GetParError(2));
    }
  }
  
  vector<double> variances[7];
  vector<double> varianceErrors[7];
  
  for(int i = 0; i < means[0].size(); ++i){
    for(int j = 0; j < 7; ++j){
      variances[j].push_back(sigs[j][i]*sigs[j][i]);
      varianceErrors[j].push_back(2*sigs[j][i]*sigErrs[j][i]);
    }
  }

  TCanvas* c1 = new TCanvas();
  //  c1->Divide(4,2);

  for(int j = 0; j < 7; ++j){
    //c1->cd(j+1);

    TGraphErrors* linGraph = new TGraphErrors(means[j].size(), &means[j][0], &variances[j][0], 
					      &meanErrs[j][0], &varianceErrors[j][0]);
    TGraphErrors* quadGraph = new TGraphErrors(means[0].size(), &means[j][0], &variances[j][0], 
					       &meanErrs[j][0], &varianceErrors[j][0]);
  
    linGraph->SetMarkerStyle(20);
    linGraph->Fit("pol1");
    linGraph->SetTitle(Form("SiPM 5%i", j+2));
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
  
  for( int i = 0; i < 4; ++ i){
    for (int j = 0; j<i; ++j){
      new TCanvas();
      TGraphErrors* corrGraph = new TGraphErrors(means[i].size(), &means[i][0], &means[j][0], 
						&meanErrs[i][0], &meanErrs[j][0]);
      corrGraph->GetXaxis()->SetTitle(Form("SiPM %i Mean", i+1));
      corrGraph->GetYaxis()->SetTitle(Form("SiPM %i Mean", j+1));
      corrGraph->SetTitle(Form("SiPM %i vs SiPM %i",j+1, i+1)); 
      corrGraph->Fit("pol1");
      corrGraph->SetMarkerStyle(20);
      corrGraph->Draw("ap");
    }
  } 
}
