uwLaserCalib(){
  const int numRuns = 20;
  const char* files[numRuns] = {
				"../crunchedFiles/run_00259_crunched.root",
				"../crunchedFiles/run_00261_crunched.root",
				"../crunchedFiles/run_00262_crunched.root",
				"../crunchedFiles/run_00263_crunched.root",
				"../crunchedFiles/run_00264_crunched.root",
				"../crunchedFiles/run_00265_crunched.root",
				"../crunchedFiles/run_00266_crunched.root",
				"../crunchedFiles/run_00267_crunched.root",
				"../crunchedFiles/run_00268_crunched.root",
				"../crunchedFiles/run_00269_crunched.root",
				"../crunchedFiles/run_00270_crunched.root",
				"../crunchedFiles/run_00274_crunched.root",
				"../crunchedFiles/run_00275_crunched.root",
				"../crunchedFiles/run_00278_crunched.root",
				"../crunchedFiles/run_00279_crunched.root",
				"../crunchedFiles/run_00280_crunched.root",
				"../crunchedFiles/run_00281_crunched.root",
				"../crunchedFiles/run_00282_crunched.root",
				"../crunchedFiles/run_00284_crunched.root",
				"../crunchedFiles/run_00285_crunched.root"};
				
  
  gStyle->SetOptFit(1);

  vector<double> means[4];
  vector<double> sigs[4];
  vector<double> meanErrs[4];
  vector<double> sigErrs[4];

  for (int i = 0; i < numRuns; ++i){
    TFile* f = new TFile(files[i]);
    TTree* t = (TTree*) f->Get("t");
    for (int j = 0; j < 4; ++j){
      t->Draw(Form("sipm%i.energy>>h%i",j+1,j),Form("sipm%i.valid",j+1));
      TH1F* h = (TH1F*) gROOT->FindObject(Form("h%i",j));
      h->Fit("gaus","0");
      means[j].push_back(h->GetFunction("gaus")->GetParameter(1));
      sigs[j].push_back(h->GetFunction("gaus")->GetParameter(2));
      meanErrs[j].push_back(h->GetFunction("gaus")->GetParError(1));
      sigErrs[j].push_back(h->GetFunction("gaus")->GetParError(2));
    }
  }
  
  vector<double> variances[4];
  vector<double> varianceErrors[4];
  
  for(int i = 0; i < means.size(); ++i){
    for(int j = 0; j < 4; ++j){
      variances[j].push_back(sigs[j][i]*sigs[j][i]);
      varianceErrors[j].push_back(2*sigs[j][i]*sigErrs[j][i]);
    }
  }
  
  TCanvas* c1 = new TCanvas();
  
  c1->Divide(2,2);

  for(int j = 0; j < 4; ++j){
    c1->cd(j+1);
    TGraphErrors* linGraph = new TGraphErrors(means[j].size(), &means[j][0], &variances[j][0], 
					      &meanErrs[j][0], &varianceErrors[j][0]);
    TGraphErrors* quadGraph = new TGraphErrors(means.size(), &means[j][0], &variances[j][0], 
					       &meanErrs[j][0], &varianceErrors[j][0]);
  
    linGraph->SetMarkerStyle(20);
    linGraph->Fit("pol1");
    linGraph->SetTitle(Form("SiPM %i", j+1));
    linGraph->SetName(Form("SiPM%i", j+1));
    linGraph->GetFunction("pol1")->SetLineColor(kBlue);
    linGraph->GetXaxis()->SetTitle("Mean");
    linGraph->GetYaxis()->SetTitle("#sigma^{2}");
    quadGraph->SetMarkerStyle(20);
    quadGraph->Fit("pol2");
    quadGraph->GetFunction("pol2")->SetLineColor(kRed);
 
    linGraph->Draw("ap");
    quadGraph->Draw("psame");
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
