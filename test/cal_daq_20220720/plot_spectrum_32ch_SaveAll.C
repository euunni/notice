#include <stdio.h>

//int plot_spectrum_32ch_update(const TString filename, const TString condition)
//int plot_spectrum_32ch_update(const TString filename)
int plot_spectrum_32ch_SaveAll(const TString filename, const int min, const int max)
{
  int channel;
  int ch_to_plot;
  FILE *fp;
  int file_size;
  int nevt;
  char data[64];
  short adc[32736];
  int evt;
  int data_length;
  int run_number;
  int tcb_trig_type;
  int tcb_trig_number;
  long long tcb_trig_time;
  int mid;
  int local_trig_number;
  int local_trigger_pattern;
  long long local_trig_time;
  long long diff_time;
  long long fine_time;
  long long coarse_time;
  int itmp;
  long long ltmp;
  int i;
  int j;
  int cont;

  std::vector<double> ch;
  std::vector<double> mean;
  std::vector<double> stddev;
   ch.clear();
   mean.clear();
   stddev.clear();

  // get channel to plot, channel = 1 ~ 32
//  printf("Channel to plot(1~32) : ");
//  scanf("%d", &channel);
//  if (channel < 1)
//    ch_to_plot = 0;
//  else if (channel > 32)
//    ch_to_plot = 31;
//  else
//    ch_to_plot = channel - 1;
    
  TCanvas *c1 = new TCanvas("c1", "CAL DAQ", 1800, 1000);
  c1->Divide(8,4,0.001,0.001);
  gPad->SetLeftMargin(0);
  gPad->SetRightMargin(0);
  gPad->SetBottomMargin(0);
  gPad->SetTopMargin(0);
  gStyle->SetStatY(0.9);
  gStyle->SetStatX(0.9);
  gStyle->SetStatW(0.4);
  gStyle->SetStatH(0.2);
  gStyle->SetOptStat(1111);
  gStyle->SetFillStyle(0);

  TH1F *plot[32];
//  int rmin = 3500 ;
//  int rmax = 4096 ;
  for( i = 0 ; i < 32 ; i ++)
  {
     plot[i] = new TH1F(Form("plot%d",i+1), Form("Spectrum ch%d", i+1), 1023,0,4096);
     //plot[i]->GetXaxis()->SetRangeUser(rmin,rmax);  
     plot[i]->GetXaxis()->SetRangeUser(min,max);  
  }
  TFile *tfile = new TFile(filename+"_SpectrumAllCh_.root","RECREATE");

  // get # of events in file
  fp = fopen(filename, "rb");
  fseek(fp, 0L, SEEK_END);
  file_size = ftell(fp);
  fclose(fp);
  nevt = file_size / 65536;
  //nevt = 1000;
  
  printf("-----------------------------------------------------------------------\n");
  fp = fopen(filename, "rb");

  for (evt = 0; evt < nevt; evt++) {
    // read header
    fread(data, 1, 64, fp);
    
    // data length
    data_length = data[0] & 0xFF;
    itmp = data[1] & 0xFF;
    itmp = itmp << 8;
    data_length = data_length + itmp;
    itmp = data[2] & 0xFF;
    itmp = itmp << 16;
    data_length = data_length + itmp;
    itmp = data[3] & 0xFF;
    itmp = itmp << 24;
    data_length = data_length + itmp;

    // run number
    run_number = data[4] & 0xFF;
    itmp = data[5] & 0xFF;
    itmp = itmp << 8;
    run_number = run_number + itmp;
    
      if(evt%100==0) cout<< evt << "/"<< nevt<< " is done ! " << endl;

    // read waveform
    fread(adc, 2, 32736, fp);
    
    // fill waveform for channel to plotgecit 
    
    for (i = 0; i < 1023; i++) {
      for( j = 0; j < 32 ; j ++) {
         plot[j]->Fill(adc[i * 32 + j]);
      }
      //if (adc[i * 32 + ch_to_plot] < 3000) printf("abnormal value = %d @ %d\n", adc[i * 32 + ch_to_plot], evt);
    }
    if (cont == 0)
      evt = nevt;
  }
//    int min1 =plot1->GetMean()-plot1->GetStdDev()*5;
//    int max1 = plot1->GetMean()+plot1->GetStdDev()*5;
//    plot1->GetXaxis()->SetRangeUser(min1,max1);

  int min_total=4096;
  for( i = 1; i < 33 ; i ++)  ch.push_back(i); //1~32 channel
  for( i = 0 ; i < 32 ; i ++)
  {
     mean.push_back(plot[i]->GetMean());
     stddev.push_back(plot[i]->GetStdDev());
     //if (min_total>plot[i]->FindFirstBinAbove(1)) min_total= plot[i]->FindFirstBinAbove(1);
  }
 
  for(i =0; i<32;i++){   
     //rmin = plot[i]->GetMean() - 5*plot[i]->GetStdDev();
     //rmax = plot[i]->GetMean() + 5*plot[i]->GetStdDev();
     //plot[i]->GetXaxis()->SetRangeUser(min_total,rmax);  

     c1->cd(i+1);
     plot[i]->Draw("hist");
  }

//    c1->Modified();
//    c1->Update();
    //c1->SaveAs(filename+"_AllchSpectrum.png");      
    //c1->SaveAs(filename+"_AllchSpectrum_SiPMmidConnectCable.png");      
    c1->SaveAs(filename+"_AllchSpectrum.png");      

  fclose(fp);

  TCanvas *c2 = new TCanvas("c2", "c2", 1000, 800);
  TGraphErrors* gr = new TGraphErrors(ch.size(), &(ch[0]), &(mean[0]), 0, &(stddev[0]));
  TString id(filename(9,10));
  gr->SetTitle("noise level as a function of channels @Mid "+id);
  gr->GetYaxis()->SetTitle("ADC");
  gr->GetYaxis()->SetTitleOffset(1.5);
  gr->GetXaxis()->SetTitle("Channels");
  gr->GetYaxis()->SetRangeUser(0,4000);
  gr->SetMarkerStyle(8);
  gr->Draw("AP");

  c2->SaveAs(filename+"_Allch_noiseLevel.png");      
  c2->Modified();
  c2->Update();
 
  c1->Write();
  c2->Write();
  tfile->Close();
  return 0;
}

