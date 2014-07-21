bool valid(TH1F* hist, int col){
  mean = hist->GetFunction("gaus")->GetParameter(1);
  if(col > 5){
    bool rv = mean < 20000;
    return rv;
  }
  else{
    bool rv = mean<25000;
    return rv;
  }

}


void howManyPe(){
  cout << "Starting" << endl;
  const int numRuns = 6;
  const char* files[numRuns] = {
    "../../../crunchedFiles/run_00896_crunched.root" ,
    "../../../crunchedFiles/run_00903_crunched.root" ,
    "../../../crunchedFiles/run_00898_crunched.root",
    "../../../crunchedFiles/run_00900_crunched.root",
    "../../../crunchedFiles/run_00901_crunched.root" ,
    "../../../crunchedFiles/run_00902_crunched.root"
				};

				
  
  gStyle->SetOptFit(1);
  gStyle->SetOptStat(0);
  
  double pes[28];
  double peErrors[28];
  double gains[28];
  double gainErrors[28];
  
  TCanvas* c1 = new TCanvas();
  
  cout << "starting loop " << endl;

  for(int row = 2; row < 6; ++row){
    for(int col = 2; col < 9; ++col){
      UInt_t index = (row-2)*7 + (col-2);
      if((row == 2 && (col == 7 || col == 8 || col == 6 )) ||
	 (row == 3 && (col == 8 || col == 6))){
	pes[index] = 0;
	gains[index] = 0;
	peErrors[index] = 0;
	gainErrors[index] = 0;
	continue;
      }

      vector<double> means;
      vector<double> sigs;
      vector<double> meanErrs;
      vector<double> sigErrs;
      for (int i = 0; i < numRuns; ++i){
	TFile* f = new TFile(files[i]);
	TTree* t = (TTree*) f->Get("t");
	t->Draw(Form("sipm%i%i.energy>>histo%i%i", row,col,row,col),Form("sipm%i%i.valid",row,col));

	TH1F* h = (TH1F*) gROOT->FindObject(Form("histo%i%i",row,col));
	h->Fit("gaus","0q");
	if(valid(h,col)){
	  means.push_back(h->GetFunction("gaus")->GetParameter(1));
	  sigs.push_back(h->GetFunction("gaus")->GetParameter(2));
	  meanErrs.push_back(h->GetFunction("gaus")->GetParError(1));
	  sigErrs.push_back(h->GetFunction("gaus")->GetParError(2));
	}

      }
  
      vector<double> variances;
      vector<double> varianceErrors;
  
      for(int i = 0; i < means.size(); ++i){
	variances.push_back(sigs[i]*sigs[i]);
	varianceErrors.push_back(2*sigs[i]*sigErrs[i]);
      }

    
      if(means.size() > 1){

	TGraphErrors* linGraph = new TGraphErrors(means.size(), &means[0], &variances[0], 
						&meanErrs[0], &varianceErrors[0]);
	TGraphErrors* quadGraph = new TGraphErrors(means.size(), &means[0], &variances[0], 
						 &meanErrs[0], &varianceErrors[0]);
      
	linGraph->SetMarkerStyle(20);
	linGraph->Fit("pol1","q");
	linGraph->SetTitle(Form("SiPM %i%i", row, col));
	linGraph->GetFunction("pol1")->SetLineColor(kBlue);
	linGraph->GetXaxis()->SetTitle("Mean");
	linGraph->GetYaxis()->SetTitle("#sigma^{2}");
	quadGraph->SetMarkerStyle(20);
	quadGraph->Fit("pol2","q");
	quadGraph->GetFunction("pol2")->SetLineColor(kRed);
       
	linGraph->Draw("ap");
	quadGraph->Draw("psame");

	/*
	pes[index] = 1.0/linGraph->GetFunction("pol1")->GetParameter(1)*means[0];
	gains[index] = linGraph->GetFunction("pol1")->GetParameter(1);
	gainErrors[index] = linGraph->GetFunction("pol1")->GetParError(1);
	peErrors[index] = TMath::Sqrt((1/gains[index]*meanErrs[0])*(1/gains[index]*meanErrs[0])+
				      (means[0]/(gains[index]*gains[index])*gainErrors[index])*(means[0]/(gains[index]*gains[index])*gainErrors[index]));
	*/


	pes[index] = 1.0/quadGraph->GetFunction("pol2")->GetParameter(1)*means[0];
	gains[index] = quadGraph->GetFunction("pol2")->GetParameter(1);
	gainErrors[index] = quadGraph->GetFunction("pol2")->GetParError(1);
	peErrors[index] = TMath::Sqrt((1/gains[index]*meanErrs[0])*(1/gains[index]*meanErrs[0])+
	(means[0]/(gains[index]*gains[index])*gainErrors[index])*(means[0]/(gains[index]*gains[index])*gainErrors[index]));
	


	  delete linGraph;
	  delete quadGraph;
      }

      else if (means.size()==1){
	UInt_t index = (row-2)*4 + (col-2);
	UInt_t mean = means[0];
	UInt_t sig = sigs[0];
	UInt_t meanErr = meanErrs[0];
	UInt_t sigErr = sigErrs[0];
	
	pes[index] = mean*mean/sig/sig;
	gains[index] = mean/pes[index];
	gainErrors[index] = TMath::Sqrt((2*sig/mean*sigErr)*(2*sig/mean*sigErr)+(sig*sig/mean/mean*meanErr)*(sig*sig/mean/mean*meanErr));
	peErrors[index] = TMath::Sqrt((2*mean/sig/sig*sigErr)*(2*mean/sig/sig*sigErr)+(2*mean*mean/sig/sig/sig*sigErr)*(2*mean*mean/sig/sig/sig*sigErr));
      }
      
      else{
	UInt_t index = (row-2)*4 + (col-2);
	pes[index] = 0;
	gains[index] = 0;
	peErrors[index] = 0;
	gainErrors[index] = 0;
      }
       
    }
  }
	


  for(int i = 0; i <28; ++i){
    cout << "SiPM " << 2+i/7 << 2+i%7 << " --- " 
	 << "p1 : " << gains[i] << ", "
	 << pes[i] << " photoelectrons at open filter, " << gains[i]*pes[i] << "calibration constant." << endl;
  }

  
  TGraphErrors* peGainGraph = new TGraphErrors(28, gains, pes, gainErrors, peErrors);

  peGainGraph->SetMarkerStyle(20);  
  peGainGraph->GetXaxis()->SetTitle("gain");
  peGainGraph->GetYaxis()->SetTitle("PE at open filter");
  c1->SetGridx();
  c1->SetGridy();
  peGainGraph->Draw("ap");
  cout << "drew graph " << endl;

  TCanvas* c2 = new TCanvas();
  TH2F* peMap = new TH2F("pes","PE Map", 7,0,7,7,0,4);
  for(int i = 0; i < 28; ++i){
    peMap->Fill(.5+i%7, 3.5-i/7, pes[i]);
  }
  peMap->GetXaxis()->SetLabelOffset(99);
  peMap->GetYaxis()->SetLabelOffset(99);
  c2->SetTickx(0);
  c2->SetTicky(0);
  
  peMap->Draw("TEXT");

  TCanvas* c3 = new TCanvas();
  TH2F* gainMap = new TH2F("gains","Gain Map", 7,0,7,4,0,4);
  for(int i = 0; i < 28; ++i){
    gainMap->Fill(.5+i%7, 3.5-i/7, gains[i]);
  }
    
  gainMap->GetXaxis()->SetLabelOffset(99);
  gainMap->GetYaxis()->SetLabelOffset(99);
  gainMap->Draw("TEXT");

}
