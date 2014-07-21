
void howManyPE(){
  const int numRuns = 6;

  const char* files[numRuns] = {
				"../crunchedFiles/run_00393_crunched.root",
				"../crunchedFiles/run_00394_crunched.root",
				"../crunchedFiles/run_00395_crunched.root",
				"../crunchedFiles/run_00398_crunched.root",
				"../crunchedFiles/run_00399_crunched.root",
				"../crunchedFiles/run_00400_crunched.root"
				};

				
  
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
	means.push_back(h->GetFunction("gaus")->GetParameter(1));
	sigs.push_back(h->GetFunction("gaus")->GetParameter(2));
	meanErrs.push_back(h->GetFunction("gaus")->GetParError(1));
	sigErrs.push_back(h->GetFunction("gaus")->GetParError(2));

      }
  
      vector<double> variances;
      vector<double> varianceErrors;
  
      for(int i = 0; i < means.size(); ++i){
	variances.push_back(sigs[i]*sigs[i]);
	varianceErrors.push_back(2*sigs[i]*sigErrs[i]);
      }

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

      UInt_t index = (row-2)*4 + (col-2);
    
      pes[index] = 1.0/quadGraph->GetFunction("pol2")->GetParameter(1)*means[0];
      gains[index] = quadGraph->GetFunction("pol2")->GetParameter(1);
      gainErrors[index] = quadGraph->GetFunction("pol2")->GetParError(1);
      peErrors[index] = TMath::Sqrt((1/gains[index]*meanErrs[0])*(1/gains[index]*meanErrs[0])+
				  (means[0]/(gains[index]*gains[index])*gainErrors[index])*(means[0]/(gains[index]*gains[index])*gainErrors[index]));
      /*
	pes[drs] = 1.0/quadGraph->GetFunction("pol2")->GetParameter(1)*means[0];
	gains[drs] = quadGraph->GetFunction("pol2")->GetParameter(1);
	gainErrors[drs] = quadGraph->GetFunction("pol2")->GetParError(1);
	peErrors[drs] = TMath::Sqrt((1/gains[drs]*meanErrs[0])*(1/gains[drs]*meanErrs[0])+
	(means[0]/(gains[drs]*gains[drs])*gainErrors[drs])*(means[0]/(gains[drs]*gains[drs])*gainErrors[drs]));
      */
    
    
      delete linGraph;
      delete quadGraph;
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
