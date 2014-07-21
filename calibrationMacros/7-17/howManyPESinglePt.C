
void howManyPESinglePt(){
  const int numRuns = 6;

  const char* file = "../../crunchedFiles/run_00462_crunched.root";
				
  
  gStyle->SetOptFit(1);
  gStyle->SetOptStat(0);
  
  double pes[16];
  double peErrors[16];
  double gains[16];
  double gainErrors[16];
  
  TCanvas* c1 = new TCanvas();
  
  cout << "starting loop " << endl;

  for(int row = 2; row < 6; ++row){
    for(int col = 2; col < 6; ++col){
    
      double mean;
      double sig;
      double meanErr;
      double sigErr;
      
      TFile* f = new TFile(file);
      TTree* t = (TTree*) f->Get("t");
      t->Draw(Form("sipm%i%i.energy>>histo%i%i", row,col,row,col),Form("sipm%i%i.valid",row,col));

      TH1F* h = (TH1F*) gROOT->FindObject(Form("histo%i%i",row,col));
      h->Fit("gaus","0q");
      mean = (h->GetFunction("gaus")->GetParameter(1));
      sig = (h->GetFunction("gaus")->GetParameter(2));
      meanErr = (h->GetFunction("gaus")->GetParError(1));
      sigErr = (h->GetFunction("gaus")->GetParError(2));
      
  
      double variance = sig*sig;
      double varianceError = 2*sig*sigErr;
  
  
      UInt_t index = (row-2)*4 + (col-2);
    
      pes[index] = mean*mean/sig/sig;
      gains[index] = mean/pes[index];
      gainErrors[index] = TMath::Sqrt((2*sig/mean*sigErr)*(2*sig/mean*sigErr)+(sig*sig/mean/mean*meanErr)*(sig*sig/mean/mean*meanErr));
      peErrors[index] = TMath::Sqrt((2*mean/sig/sig*sigErr)*(2*mean/sig/sig*sigErr)+(2*mean*mean/sig/sig/sig*sigErr)*(2*mean*mean/sig/sig/sig*sigErr));
      /*
	pes[drs] = 1.0/quadGraph->GetFunction("pol2")->GetParameter(1)*means[0];
	gains[drs] = quadGraph->GetFunction("pol2")->GetParameter(1);
	gainErrors[drs] = quadGraph->GetFunction("pol2")->GetParError(1);
	peErrors[drs] = TMath::Sqrt((1/gains[drs]*meanErrs[0])*(1/gains[drs]*meanErrs[0])+
	(means[0]/(gains[drs]*gains[drs])*gainErrors[drs])*(means[0]/(gains[drs]*gains[drs])*gainErrors[drs]));
      */
    

    }
  }



  for(int i = 0; i <16; ++i){
    cout << "SiPM " << 2+i/4 << 2+i%4 << " --- " 
	 << "p1 : " << gains[i] << ", "
	 << pes[i] << " photoelectrons at open filter." << endl;
  }

  
  TGraphErrors* peGainGraph = new TGraphErrors(16, gains, pes, gainErrors, peErrors);

  peGainGraph->SetMarkerStyle(20);  
  peGainGraph->GetXaxis()->SetTitle("gain");
  peGainGraph->GetYaxis()->SetTitle("PE at open filter");
  c1->SetGridx();
  c1->SetGridy();
  peGainGraph->Draw("ap");
  cout << "drew graph " << endl;

  TCanvas* c2 = new TCanvas();
  TH2F* peMap = new TH2F("pes","PE Map", 4,0,4,4,0,4);
  for(int i = 0; i < 16; ++i){
    peMap->Fill(.5+i%4, 3.5-i/4, pes[i]);
  }
  peMap->GetXaxis()->SetLabelOffset(99);
  peMap->GetYaxis()->SetLabelOffset(99);
  c2->SetTickx(0);
  c2->SetTicky(0);
  
  peMap->Draw("TEXT");

  TCanvas* c3 = new TCanvas();
  TH2F* gainMap = new TH2F("gains","Gain Map", 4,0,4,4,0,4);
  for(int i = 0; i < 16; ++i){
    gainMap->Fill(.5+i%4, 3.5-i/4, gains[i]);
  }
    
  gainMap->GetXaxis()->SetLabelOffset(99);
  gainMap->GetYaxis()->SetLabelOffset(99);
  gainMap->Draw("TEXT");

}
