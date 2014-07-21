bool valid(TH1F* hist, int which){
  mean = hist->GetFunction("gaus")->GetParameter(1);
  if(to_string(which)[1] > 5){
    return (mean < 20000&&mean>200);
  }
  return (mean < 25000&&mean>300);
}


void fullCalib(){
  const int numRuns = 6;
  const char* files[numRuns] = {
    "../../../crunchedFiles/run_00886_crunched.root",
    "../../../crunchedFiles/run_00885_crunched.root",
    "../../../crunchedFiles/run_00880_crunched.root",
    "../../../crunchedFiles/run_00881_crunched.root",
    "../../../crunchedFiles/run_00882_crunched.root",
    "../../../crunchedFiles/run_00884_crunched.root"
				};
				
  
  gStyle->SetOptFit(1);

  cout << "Which? " << endl;
  int which;
  cin >> which;


  vector<double> means;
  vector<double> sigs;
  vector<double> meanErrs;
  vector<double> sigErrs;

  for (int i = 0; i < numRuns; ++i){
    TFile* f = new TFile(files[i]);
    TTree* t = (TTree*) f->Get("t");
    cout << "got tree" << endl;
    t->Draw(Form("sipm%i.energy>>histo%i", which,i),Form("sipm%i.valid",which));
    cout << "drew" << endl;

    cout << "finding obj" << endl;
    TH1F* h = (TH1F*) gROOT->FindObject(Form("histo%i",i));
    h->Fit("gaus","0");
    if(valid(h, which)){
      means.push_back(h->GetFunction("gaus")->GetParameter(1));
      sigs.push_back(h->GetFunction("gaus")->GetParameter(2));
      meanErrs.push_back(h->GetFunction("gaus")->GetParError(1));
      sigErrs.push_back(h->GetFunction("gaus")->GetParError(2));
    }
    cout << "end itr" << endl;
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
    linGraph->Fit("pol1");
    linGraph->SetTitle(Form("SiPM %i", which));
    linGraph->GetFunction("pol1")->SetLineColor(kBlue);
    linGraph->GetXaxis()->SetTitle("Mean");
    linGraph->GetYaxis()->SetTitle("#sigma^{2}");
    quadGraph->SetMarkerStyle(20);
    
    TF1* quad = new TF1("quad",
			    "[0]+[1]*x+[2]*x*x",
			    0,100000);

    
    quad->SetParameter(0,0);
    quad->SetParameter(1,5);
    quad->SetParameter(2,.0001);
    quad->SetParLimits(2,-.0004,.0004);
    quadGraph->Fit("quad");
    quad->SetLineColor(kRed);
    linGraph->Draw("ap");
    quadGraph->Draw("psame");
  
}
