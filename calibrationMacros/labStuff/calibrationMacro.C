calibrationMacro(){
  const int numRuns = 4;
  const char* files[numRuns] = {"longRunCrunched.root", "filter2Crunched.root","filter4Crunched.root", "filter5Crunched.root"};
  
  gStyle->SetOptFit(1);

  vector<double> means;
  vector<double> sigs;
  vector<double> meanErrs;
  vector<double> sigErrs;
  vector<double> aMeans;
  vector<double> aSigs;
  vector<double> aMeanErrs;
  vector<double> aSigErrs;

  for (int i = 0; i < numRuns; ++i){
    TFile* f = new TFile(files[i]);
    TTree* t = (TTree*) f->Get("t");
    TCanvas* c1 = new TCanvas();
    t->Draw("-1*newSipm.energy>>h");
    h->Fit("gaus");
    means.push_back(h->GetFunction("gaus")->GetParameter(1));
    sigs.push_back(h->GetFunction("gaus")->GetParameter(2));
    meanErrs.push_back(h->GetFunction("gaus")->GetParError(1));
    sigErrs.push_back(h->GetFunction("gaus")->GetParError(2));
    h->SetTitle(Form("%s : Fit", files[i]));
    TCanvas* c2 = new TCanvas();
    t->Draw("-1*newSipm.aSum>>h2");
    h2->Fit("gaus");
    aMeans.push_back(h2->GetFunction("gaus")->GetParameter(1));
    aSigs.push_back(h2->GetFunction("gaus")->GetParameter(2));
    aMeanErrs.push_back(h2->GetFunction("gaus")->GetParError(1));
    aSigErrs.push_back(h2->GetFunction("gaus")->GetParError(2));
    h2.SetTitle(Form("%s : Analogue", files[i]));
  }
  
  vector<double> variances;
  vector<double> varianceErrors;
  vector<double> aVariances;
  vector<double> aVarianceErrors;
  
  for(int i = 0; i < means.size(); ++i){
    variances.push_back(sigs[i]*sigs[i]);
    varianceErrors.push_back(2*sigs[i]*sigErrs[i]);

    aVariances.push_back(aSigs[i]*aSigs[i]);
    aVarianceErrors.push_back(2*aSigs[i]*aSigErrs[i]);
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
  
  new TCanvas();
  TGraphErrors* aLinGraph = new TGraphErrors(aMeans.size(), &aMeans[0], &aVariances[0], 
					    &aMeanErrs[0], &aVarianceErrors[0]);
  TGraphErrors* aQuadGraph = new TGraphErrors(means.size(), &aMeans[0], &aVariances[0], 
					    &aMeanErrs[0], &aVarianceErrors[0]);
  
  aLinGraph->SetMarkerStyle(20);
  aLinGraph->Fit("pol1");
  aLinGraph->SetTitle("Analogue");
  aLinGraph->GetFunction("pol1")->SetLineColor(kBlue);
  aLinGraph->GetXaxis()->SetTitle("Mean");
  aLinGraph->GetYaxis()->SetTitle("#sigma^{2}");
  aQuadGraph->SetMarkerStyle(20);
  aQuadGraph->Fit("pol2");
  aQuadGraph->GetFunction("pol2")->SetLineColor(kRed);
 
  aLinGraph->Draw("ap");
  aQuadGraph->Draw("psame");
  

}
