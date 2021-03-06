#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "TClass.h"
#include "TApplication.h"
#include "TGClient.h"
#include "TROOT.h"
#include "TH1.h"
#include "TStyle.h"
#include "TClass.h"
#include "TFile.h"

#include "IUAmpTools/AmpToolsInterface.h"
#include "IUAmpTools/FitResults.h"

#include "AmpPlotter/PlotterMainWindow.h"
#include "AmpPlotter/PlotFactory.h"

#include "EtaPiPlotGenerator.h"
#include "AMPTOOLS_DATAIO/ROOTDataReader.h"
#include "AMPTOOLS_AMPS/TwoPSHelicity.h"
#include "AMPTOOLS_AMPS/Zlm.h"
#include "AMPTOOLS_AMPS/BreitWigner.h"

typedef EtaPiPlotGenerator PlotGen;

void atiSetup(){
  
  AmpToolsInterface::registerAmplitude( TwoPSHelicity() );
  AmpToolsInterface::registerAmplitude( Zlm() );
  AmpToolsInterface::registerAmplitude( BreitWigner() );
  AmpToolsInterface::registerDataReader( ROOTDataReader() );
}

using namespace std;

int main( int argc, char* argv[] ){


    // ************************
    // usage
    // ************************

  cout << endl << " *** Viewing Results Using AmpPlotter and writing root histograms *** " << endl << endl;
  auto help_message = []()
  {
      cout << "\t-o <file>\t output file path" << endl;
      cout << "\t-s <amp string> choose a subset of amplitudes to plot. _ separated -- i.e. S0+_D0+" << endl;
      cout << "\t-g <file>\t show GUI" << endl;
      exit(1);
  };

  if (argc < 3){
    cout << "======\nUsage:" << endl << "======" << endl;
    cout << "twopi_plotter <results file name> -o <output file name> -s <amp string>" << endl << endl;
    help_message();
    return 0;
  }

  bool showGui = false;
  string outName = "etapi_plot.root";
  string resultsName(argv[1]);
  map<string,int> selectedAmps;
  bool keepAllAmps=false;
  for (int i = 2; i < argc; i++){

    string arg(argv[i]);

    if (arg == "-g"){
      showGui = true;
    }
    if (arg == "-o"){
      outName = argv[++i];
    }
    if (arg == "-h"){
        help_message();
    }

    if (arg == "-s"){
        string ampString=argv[++i];
        if (ampString==""){ 
            cout << "\n------------\nPlotting all amplitudes\n------------" << endl;
            keepAllAmps=true;
        }
        else {
            string tmp; 
            stringstream ss(ampString);
            while(getline(ss, tmp, '_')){
                selectedAmps[tmp] = -1;
                cout << "\n------------\nPlotting only with these amplitude contributions:" << endl;
                cout << tmp << endl;
            }
            cout << "------------\n" << endl;
        }
    }
  }


    // ************************
    // parse the command line parameters
    // ************************

  cout << "Fit results file name    = " << resultsName << endl;
  cout << "Output file name    = " << outName << endl << endl;

    // ************************
    // load the results and display the configuration info
    // ************************

  FitResults results( resultsName );
  if( !results.valid() ){
    
    cout << "Invalid fit results in file:  " << resultsName << endl;
    exit( 1 );
  }

    // ************************
    // set up the plot generator
    // ************************

  atiSetup();
  PlotGen plotGen( results );

    // ************************
    // set up an output ROOT file to store histograms
    // ************************

  TFile* plotfile = new TFile( outName.c_str(), "recreate");
  TH1::AddDirectory(kFALSE);

  string reactionName = results.reactionList()[0];
  plotGen.enableReaction( reactionName );
  vector<string> sums = plotGen.uniqueSums();
  vector<string> amps = plotGen.uniqueAmplitudes();
  cout << "\npossible sums:\n----------" << endl;
  for (auto i:sums){
    cout << i << endl;
  }
  cout << "\npossible amps:\n----------" << endl;
  for (unsigned int i=0; i<amps.size(); ++i){
    cout << amps[i] << endl;
    if (selectedAmps.find(amps[i])!=selectedAmps.end()){
        selectedAmps[amps[i]]=i;
    }
  }
  if(!keepAllAmps){
    for (unsigned int i=0; i<amps.size(); ++i){
      plotGen.disableAmp(i);
    }
    for (std::map<string,int>::iterator it=selectedAmps.begin(); it!=selectedAmps.end(); ++it){
      plotGen.enableAmp(it->second);
    }
  }

  // loop over sum configurations (one for each of the individual contributions, and the combined sum of all)
  for (unsigned int isum = 0; isum <= sums.size(); isum++){

    // turn on all sums by default
    for (unsigned int i = 0; i < sums.size(); i++){
      plotGen.enableSum(i);
    }

    // for individual contributions turn off all sums but the one of interest
    if (isum < sums.size()){
      for (unsigned int i = 0; i < sums.size(); i++){
        if (i != isum) plotGen.disableSum(i);
      }
    }

    // loop over data, accMC, and genMC
    for (unsigned int iplot = 0; iplot < PlotGenerator::kNumTypes; iplot++){
      if (isum < sums.size() && iplot == PlotGenerator::kData) continue; // only plot data once

      // loop over different variables
      for (unsigned int ivar  = 0; ivar  < EtaPiPlotGenerator::kNumHists; ivar++){

        // set unique histogram name for each plot (could put in directories...)
        string histname =  "";
        if (ivar == EtaPiPlotGenerator::kEtaPiMass)  histname += "Metapi";
	else if (ivar == EtaPiPlotGenerator::kEtaCosTheta)  histname += "cosTheta";
        else if (ivar == EtaPiPlotGenerator::kPhi)  histname += "Phi";
        else if (ivar == EtaPiPlotGenerator::kphi)  histname += "phi";
        else if (ivar == EtaPiPlotGenerator::kPsi)  histname += "psi";
        else if (ivar == EtaPiPlotGenerator::kt)  histname += "t";
        else continue;

        if (iplot == PlotGenerator::kData) histname += "dat";
        if (iplot == PlotGenerator::kAccMC) histname += "acc";
        if (iplot == PlotGenerator::kGenMC) histname += "gen";

        if (isum < sums.size()){
          //ostringstream sdig;  sdig << (isum + 1);
          //histname += sdig.str();

	  // get name of sum for naming histogram
          string sumName = sums[isum];
          histname += "_";
          histname += sumName;
        }

        Histogram* hist = plotGen.projection(ivar, reactionName, iplot);
        TH1* thist = hist->toRoot();
        thist->SetName(histname.c_str());
        plotfile->cd();
        thist->Write();

      }
    }
  }

  plotfile->Close();

    // ************************
    // retrieve amplitudes for output
    // ************************
  /*
  // parameters to check
  vector< string > pars;
  pars.push_back("Pi+Pi-::Positive::S0+_re");
  pars.push_back("Pi+Pi-::Positive::S0+_im");

  // file for writing parameters (later switch to putting in ROOT file)
  ofstream outfile;
  outfile.open( "twopi_fitPars.txt" );

  for(unsigned int i = 0; i<pars.size(); i++) {
    double parValue = results.parValue( pars[i] );
    double parError = results.parError( pars[i] );
    outfile << parValue << "\t" << parError << "\t";
  }

  // Note: For twopi_amp_plotter: The following computations are nonsense for amplitudes

  // covariance matrix
  vector< vector< double > > covMatrix;
  covMatrix = results.errorMatrix();

  double SigmaN = results.parValue(pars[3]) + results.parValue(pars[6]);
  double SigmaN_err = covMatrix[5][5] + covMatrix[8][8] + 2*covMatrix[5][8];

  double SigmaD = 0.5*(1 - results.parValue(pars[0])) + results.parValue(pars[2]);
  double SigmaD_err = 0.5*0.5*covMatrix[2][2] + covMatrix[4][4] - 2*0.5*covMatrix[2][4];

  double Sigma = SigmaN/SigmaD;
  double Sigma_err = fabs(Sigma) * sqrt(SigmaN_err/SigmaN/SigmaN + SigmaD_err/SigmaD/SigmaD);

  double P = 2*results.parValue(pars[6]) - results.parValue(pars[4]);
  double P_err = sqrt(2*2*covMatrix[8][8] + covMatrix[6][6] - 2*2*covMatrix[6][8]);

  Sigma = Sigma_err = P = P_err = 0;
  outfile << Sigma << "\t" << Sigma_err << "\t";
  outfile << P << "\t" << P_err << "\t";

  outfile << endl;
  */

    // ************************
    // start the GUI
    // ************************

  if(showGui) {

	  cout << ">> Plot generator ready, starting GUI..." << endl;
	  
	  int dummy_argc = 0;
	  char* dummy_argv[] = {};  
	  TApplication app( "app", &dummy_argc, dummy_argv );
          cout << "Started TApp" << endl;
	  
	  gStyle->SetFillColor(10);
	  gStyle->SetCanvasColor(10);
	  gStyle->SetPadColor(10);
	  gStyle->SetFillStyle(1001);
	  gStyle->SetPalette(1);
	  gStyle->SetFrameFillColor(10);
	  gStyle->SetFrameFillStyle(1001);
	  
	  PlotFactory factory( plotGen );	
          cout << "Started factory" << endl;
	  PlotterMainWindow mainFrame( gClient->GetRoot(), factory );
          cout << "Starting window" << endl;
	  
	  app.Run();
          cout << "Done!" << endl;
  }
    
  return 0;

}

