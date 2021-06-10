/*************************************************************************
 * This file is part of the REST software framework.                     *
 *                                                                       *
 * Copyright (C) 2016 GIFNA/TREX (University of Zaragoza)                *
 * For more information see http://gifna.unizar.es/trex                  *
 *                                                                       *
 * REST is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * REST is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have a copy of the GNU General Public License along with   *
 * REST in $REST_PATH/LICENSE.                                           *
 * If not, see http://www.gnu.org/licenses/.                             *
 * For the list of contributors see $REST_PATH/CREDITS.                  *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
///
/// \brief Process to fit pulses (TRestRawSignal) with the electronic response function
/// or with the convolution of the electronic response function with a gaussian pulse.
///
/// ### Fitting functions:
///
/// **AGET function:**
/// (electronic response function of AGET chip)
///
/// TMath::Exp(-3. * (x-[1])/[0] ) * (x-[1])/[0] * (x-[1])/[0] * (x-[1])/[0]
/// * sin((x-[1])/[0])/(1+TMath::Exp(-10000*(x-[1])))
///
/// [0] = "ShapingTime"
/// [1] = "StartPosition"
///
/// **Gauss function:**
/// exp(-0.5*x*x/[0])*[1]
///
/// [0] = "VarianceGauss"
/// [1]= "Amplitude"
///
///
/// ### Options:
///
/// * **agetFit**: Select fitting mode:
///                * False (Default) -> Convolution fit.
///                                     Pulse fitted with convolution of both functions.
///                                     Amplitude parameter fixed to 10 * PulseAmplitude.
///                * True -> AGET fit.
///                          Pulse fitted only with AGET funcion.
///
/// * **shapingFixed**: Fix ShapingTime parameter to desired value.
///                If not provided it is a free parameter of the fit.
///
/// * **startPositionFixed**: Fix StartPosition parameter to MaxPeakBin - startPositionFixed.
///                If not provided it is a free parameter of the fit.
///
/// * **varianceFixed**: Fix VarianceGauss parameter to desired value.
///                If not provided it is a free parameter of the fit.
///
/// * **amplitudeFixed**: Fix Amplitude parameter to amplitudeFixed * singleSignal->GetData(MaxPeakBin).
///                If not provided it is a free parameter of the fit.
///
/// * **shapingInitialValue**: Initial value for ShapingTime parameter.
///
/// * **startPositionInitialValue**: Initial value for StartPosition parameter.
///
/// * **varianceInitialValue**: Initial value for VarianceGauss parameter.
///
/// * **amplitudeInitialValue**: Initial value for Amplitude parameter.
///
/// Convolution mode default initial parameters are:
///  * ShapingTime = 32.
///  * StartPosition = MaxPeakBin - 25.
///  * VarianceGauss = 1.
///  * Amplitude = singleSignal->GetData(MaxPeakBin)* 10
///
/// AGET mode default initial parameters are:
///  * ShapingTime = 32.
///  * StartPosition = MaxPeakBin - 25.
///  * Amplitude = singleSignal->GetData(MaxPeakBin)
///
/// * **Good signal's selection**: Convolution mode only.
/// Parameters to select good signals can be provided and only good ones will be fitted.
/// Parameters needed are:
///                * BaseLineRange
///                * PointsOverThreshold
///                * PointThreshold
///                * SignalThreshold
/// 
/// * **addAllPulses**: Add all pulses in the event and fit the resulting one.
/// Example in rml file:
/// \code
/// <addProcess type="TRestRawSignalFitEventProcess" name="rawFitEvent" value="ON"
///        observable="all" verboseLevel="info"  agetFit= "false" shapingFixed = "32"
///        baseLineRange="(20,150)"  pointsOverThreshold="40" pointThreshold="6" signalThreshold="3" >
/// </addProcess>
/// \endcode
///
///
/// ### Observables
///
///
/// * **FitAmplitude_map**: Amplitude parameter of each pulse.
///
/// * **FitShapingTime_map**: Shaping parameter of each pulse.
///
/// * **FitStartPosition_map**: Starting point of each pulse.
///
/// * **FitVarianceGauss_map**: Only in Convolution mode. Variance of gaussian component.
///
/// * **FitRatioSigmaMaxPeak_map**: Square root of the squared difference betweeen
///  raw signal and fit divided by number of bins and by amplitude of the pulse.
///
/// * **FitMaxVarianceGauss**: Only in Convolution mode. Maximum variance parameter between
/// pulses in the event.
///
/// * **FitVarianceGaussWMean**: Only in Convolution mode. Weighted mean of VarianceGauss parameter.
///
/// * **FitVarianceGaussWStdDev**: Only in Convolution mode. Weighted standard deviation of
/// VarianceGauss parameter.
///
/// * **FitSigmaMean**: Mean over all pulses in the event of square root of the squared
/// difference betweeen raw signal and fit divided by number of bins.
///
/// * **FitSigmaStdDev**: Standard deviation over all pulses in the event of square root of the squared
/// difference betweeen raw signal and fit divided by number of bins.
///
/// * **FitChiSquareMean**: Mean over all pulses in the event of chi square of the fit.
///
/// * **FitRatioSigmaMaxPeakMean**: Mean over all pulses in the event of square root of the squared
/// difference betweeen raw signal and fit divided by number of bins and divided by amplitude of the pulse.
///
///_______________________________________________________________________________
///
/// RESTsoft - Software for Rare Event Searches with TPCs
///
/// History of developments:
///
/// 2021-March   Merged ConvolutionFittingProcess with FittingProcess.
///              New Weighted observables and option to fix ShapingTime.
///
/// \class      TRestRawSignalFitEventProcess
/// \author     David Diez
///
///______________________________________________________________________________
///
//////////////////////////////////////////////////////////////////////////

#include "TRestRawSignalFitEventProcess.h"
using namespace std;

ClassImp(TRestRawSignalFitEventProcess);

///////////////////////////////////////////////
/// \brief Default constructor
///
TRestRawSignalFitEventProcess::TRestRawSignalFitEventProcess() { Initialize(); }

///////////////////////////////////////////////
/// \brief Constructor loading data from a config file
///
/// If no configuration path is defined using TRestMetadata::SetConfigFilePath
/// the path to the config file must be specified using full path, absolute or
/// relative.
///
/// The default behaviour is that the config file must be specified with
/// full path, absolute or relative.
///
/// \param cfgFileName A const char* giving the path to an RML file.
///
TRestRawSignalFitEventProcess::TRestRawSignalFitEventProcess(char* cfgFileName) {
    Initialize();

    if (LoadConfigFromFile(cfgFileName)) LoadDefaultConfig();
}

///////////////////////////////////////////////
/// \brief Default destructor
///
TRestRawSignalFitEventProcess::~TRestRawSignalFitEventProcess() {}

///////////////////////////////////////////////
/// \brief Function to load the default config in absence of RML input
///
void TRestRawSignalFitEventProcess::LoadDefaultConfig() { SetTitle("Default config"); }

///////////////////////////////////////////////
/// \brief Function to initialize input/output event members and define the
/// section name
///
void TRestRawSignalFitEventProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fRawSignalEvent = NULL;
}

///////////////////////////////////////////////
/// \brief Function to load the configuration from an external configuration
/// file.
///
/// If no configuration path is defined in TRestMetadata::SetConfigFilePath
/// the path to the config file must be specified using full path, absolute or
/// relative.
///
/// \param cfgFileName A const char* giving the path to an RML file.
/// \param name The name of the specific metadata. It will be used to find the
/// correspondig TRestGeant4AnalysisProcess section inside the RML.
///
void TRestRawSignalFitEventProcess::LoadConfig(std::string cfgFilename, std::string name) {
    if (LoadConfigFromFile(cfgFilename, name)) LoadDefaultConfig();
}

///////////////////////////////////////////////
/// \brief Process initialization.
///
void TRestRawSignalFitEventProcess::InitProcess() {
    // fSignalFittingObservables = TRestEventProcess::ReadObservables();
}

///////////////////////////////////////////////
/// \brief The main processing event function
///
TRestEvent* TRestRawSignalFitEventProcess::ProcessEvent(TRestEvent* evInput) {
    // no need for verbose copy now
    fRawSignalEvent = (TRestRawSignalEvent*)evInput;

    debug << "TRestRawSignalFitEventProcess::ProcessEvent. Event ID : " << fRawSignalEvent->GetID() << endl;

    Double_t SigmaMean = 0;
    Double_t Sigma[fRawSignalEvent->GetNumberOfSignals()] = {};
    Double_t RatioSigmaMaxPeakMean = 0;
    Double_t RatioSigmaMaxPeak[fRawSignalEvent->GetNumberOfSignals()] = {};
    Double_t ChiSquareMean = 0;
    Double_t ChiSquare[fRawSignalEvent->GetNumberOfSignals()] = {};
    Double_t VGmean = 0;
    Double_t VGvariance = 0;
    Double_t VGweights = 0;

    map<int, Double_t> amplitudeFit;
    map<int, Double_t> shapingtimeFit;
    map<int, Double_t> StartPositionFit;
    map<int, Double_t> variancegaussFit;

    map<int, Double_t> ratiosigmaamplitudeFit;

    amplitudeFit.clear();
    shapingtimeFit.clear();
    StartPositionFit.clear();
    variancegaussFit.clear();

    int MinBinRange = 45;
    int MaxBinRange = 70;

    int goodSig = 0;
    
    if (fAddAllPulses == false) {

        /////----- Fit with convoluted function -----/////
    
        if (fAgetFit == false) {
            /// Fit all signals ///
            if (fPointThreshold == 0 && fSignalThreshold == 0 && fPointsOverThreshold == 0 &&
                fBaseLineRange.X() == 0 && fBaseLineRange.Y() == 0) {
                for (int s = 0; s < fRawSignalEvent->GetNumberOfSignals(); s++) {
                    TRestRawSignal* singleSignal = fRawSignalEvent->GetSignal(s);
                    singleSignal->CalculateBaseLine(20, 150);
                    int MaxPeakBin = singleSignal->GetMaxPeakBin();
    
                    // ShaperSin function (AGET theoretical curve) times logistic function
                    TF1* f = new TF1("Aget",
                                     "TMath::Exp(-3. * (x-[1])/[0] ) * (x-[1])/[0] * (x-[1])/[0] * (x-[1])/[0] "
                                     " * sin((x-[1])/[0])/(1+TMath::Exp(-10000*(x-[1])))",
                                     0, 511);
                    f->SetParNames("ShapingTime", "StartPosition");
    
                    // Gaussian pulse
                    TF1* g = new TF1("pulse", "exp(-0.5*x*x/[0])*[1]", 0, 511);
                    g->SetParNames("VarianceGauss", "Amplitude");
    
                    // Convolution of AGET and gaussian functions
                    TF1Convolution* conv = new TF1Convolution("Aget", "pulse", 0, 511, true);
                    conv->SetRange(0, 511);
                    conv->SetNofPointsFFT(10000);
    
                    TF1* fit_conv = new TF1("fit", *conv, 0, 511, conv->GetNpar());
                    fit_conv->SetParNames("ShapingTime", "StartPosition", "VarianceGauss", "Amplitude");
    
                    // Create histogram from signal
                    Int_t nBins = singleSignal->GetNumberOfPoints();
                    TH1D* h = new TH1D("histo", "Signal to histo", nBins, 0, nBins);
    
                    for (int i = 0; i < nBins; i++) {
                        // h->Fill(i,
                        // (singleSignal->GetRawData(i)-singleSignal->GetBaseLine())*100/singleSignal->GetData(MaxPeakBin));
                        // h->SetBinError(i,
                        // singleSignal->GetBaseLineSigma()*100/singleSignal->GetData(MaxPeakBin));
                        h->Fill(i, singleSignal->GetRawData(i) - singleSignal->GetBaseLine());
                        h->SetBinError(i, singleSignal->GetBaseLineSigma());
                    }
    
                    // Fit histogram with convolution
                    // fit_conv->SetParameters(32., MaxPeakBin - 25., 1., singleSignal->GetData(MaxPeakBin)* 10);
    
                    // Read initial parameters
                    if (fShapingInitialValue != 0) {
                        fit_conv->SetParameter(0, fShapingInitialValue);
                    } else {
                        fit_conv->SetParameter(0, 32.);
                    }
                    if (fStartPositionInitialValue != 0) {
                        fit_conv->SetParameter(1, MaxPeakBin - fStartPositionInitialValue);
                    } else {
                        fit_conv->SetParameter(1, MaxPeakBin - 25);
                    }
                    if (fVarianceInitialValue != 0) {
                        fit_conv->SetParameter(2, fVarianceInitialValue);
                    } else {
                        fit_conv->SetParameter(2, 1.);
                    }
                    if (fAmplitudeInitialValue != 0) {
                        fit_conv->SetParameter(3, singleSignal->GetData(MaxPeakBin) * fAmplitudeInitialValue);
                    } else {
                        fit_conv->SetParameter(3, singleSignal->GetData(MaxPeakBin) * 10);
                    }
    
                    // Read fixed parameters
                    if (fShapingFixed != 0) {
                        fit_conv->FixParameter(0, fShapingFixed);
                    }
                    if (fStartPositionFixed != 0) {
                        fit_conv->FixParameter(1, MaxPeakBin - fStartPositionFixed);
                    }
                    if (fVarianceFixed != 0) {
                        fit_conv->FixParameter(2, fVarianceFixed);
                    }
                    if (fAmplitudeFixed != 0) {
                        fit_conv->FixParameter(3, singleSignal->GetData(MaxPeakBin) * fAmplitudeFixed);
                    }
                    // fit_conv->FixParameter(3, singleSignal->GetData(MaxPeakBin) * 100);
                    // fit_conv->FixParameter(3, singleSignal->GetIntegralInRange(MaxPeakBin - MinBinRange,
                    // MaxPeakBin + MaxBinRange));
    
                    h->Fit(fit_conv, "RMNQWW", "", MaxPeakBin - MinBinRange, MaxPeakBin + MaxBinRange);
                    // Options: L->Likelihood minimization, R->fit in range, N->No draw, Q->Quiet
    
                    Double_t sigma = 0;
                    for (int j = MaxPeakBin - MinBinRange; j < MaxPeakBin + MaxBinRange; j++) {
                        sigma +=
                            (h->GetBinContent(j) - fit_conv->Eval(j)) * (h->GetBinContent(j) - fit_conv->Eval(j));
                    }
                    Sigma[s] = TMath::Sqrt(sigma / (MinBinRange + MaxBinRange));
                    RatioSigmaMaxPeak[s] = Sigma[s] / h->GetBinContent(MaxPeakBin + 1);
                    RatioSigmaMaxPeakMean += RatioSigmaMaxPeak[s];
                    SigmaMean += Sigma[s];
                    ChiSquare[s] = fit_conv->GetChisquare();
                    ChiSquareMean += ChiSquare[s];
    
                    amplitudeFit[singleSignal->GetID()] = fit_conv->GetParameter("Amplitude");
                    shapingtimeFit[singleSignal->GetID()] = fit_conv->GetParameter("ShapingTime");
                    StartPositionFit[singleSignal->GetID()] = fit_conv->GetParameter("StartPosition");
                    variancegaussFit[singleSignal->GetID()] = fit_conv->GetParameter("VarianceGauss");
    
                    ratiosigmaamplitudeFit[singleSignal->GetID()] = RatioSigmaMaxPeak[s];
    
                    VGmean += fit_conv->GetParameter("VarianceGauss") / fit_conv->GetParError(2);
                    VGvariance += fit_conv->GetParameter("VarianceGauss") * fit_conv->GetParameter(2) /
                                  fit_conv->GetParError(2);
                    VGweights += 1 / fit_conv->GetParError(2);
    
                    h->Delete();
                }
            }
    
            /// Fit good signal's selection ///
            else {
                fRawSignalEvent->SetBaseLineRange(fBaseLineRange);
                for (int s = 0; s < fRawSignalEvent->GetNumberOfSignals(); s++) {
                    TRestRawSignal* singleSignal = fRawSignalEvent->GetSignal(s);
                    singleSignal->CalculateBaseLine(fBaseLineRange.X(), fBaseLineRange.Y());
                    singleSignal->InitializePointsOverThreshold(TVector2(fPointThreshold, fSignalThreshold),
                                                                fPointsOverThreshold);
    
                    if (singleSignal->GetPointsOverThreshold().size() >= 2) {
                        goodSig++;
                        int MaxPeakBin = singleSignal->GetMaxPeakBin();
    
                        // ShaperSin function (AGET theoretical curve) times logistic function
                        TF1* f =
                            new TF1("Aget",
                                    "TMath::Exp(-3. * (x-[1])/[0] ) * (x-[1])/[0] * (x-[1])/[0] * (x-[1])/[0] "
                                    " * sin((x-[1])/[0])/(1+TMath::Exp(-10000*(x-[1])))",
                                    0, 511);
                        f->SetParNames("ShapingTime", "StartPosition");
    
                        // Gaussian pulse
                        TF1* g = new TF1("pulse", "exp(-0.5*x*x/[0])*[1]", 0, 511);
                        g->SetParNames("VarianceGauss", "Amplitude");
    
                        // Convolution of AGET and gaussian functions
                        TF1Convolution* conv = new TF1Convolution("Aget", "pulse", 0, 511, true);
                        conv->SetRange(0, 511);
                        conv->SetNofPointsFFT(10000);
    
                        TF1* fit_conv = new TF1("fit", *conv, 0, 511, conv->GetNpar());
                        fit_conv->SetParNames("ShapingTime", "StartPosition", "VarianceGauss", "Amplitude");
    
                        // Create histogram from signal
                        Int_t nBins = singleSignal->GetNumberOfPoints();
                        TH1D* h = new TH1D("histo", "Signal to histo", nBins, 0, nBins);
    
                        for (int i = 0; i < nBins; i++) {
                            // h->Fill(i,
                            // (singleSignal->GetRawData(i)-singleSignal->GetBaseLine())*100/singleSignal->GetData(MaxPeakBin));
                            // h->SetBinError(i,
                            // singleSignal->GetBaseLineSigma()*100/singleSignal->GetData(MaxPeakBin));
                            h->Fill(i, singleSignal->GetRawData(i) - singleSignal->GetBaseLine());
                            h->SetBinError(i, singleSignal->GetBaseLineSigma());
                        }
    
                        // Fit histogram with convolution
                        // fit_conv->SetParameters(32., MaxPeakBin - 25., 1., singleSignal->GetData(MaxPeakBin)*
                        // 10);
    
                        // Read initial parameters
                        if (fShapingInitialValue != 0) {
                            fit_conv->SetParameter(0, fShapingInitialValue);
                        } else {
                            fit_conv->SetParameter(0, 32.);
                        }
                        if (fStartPositionInitialValue != 0) {
                            fit_conv->SetParameter(1, MaxPeakBin - fStartPositionInitialValue);
                        } else {
                            fit_conv->SetParameter(1, MaxPeakBin - 25);
                        }
                        if (fVarianceInitialValue != 0) {
                            fit_conv->SetParameter(2, fVarianceInitialValue);
                        } else {
                            fit_conv->SetParameter(2, 1.);
                        }
                        if (fAmplitudeInitialValue != 0) {
                            fit_conv->SetParameter(3, singleSignal->GetData(MaxPeakBin) * fAmplitudeInitialValue);
                        } else {
                            fit_conv->SetParameter(3, singleSignal->GetData(MaxPeakBin) * 10);
                        }
    
                        // Read fixed parameters
                        if (fShapingFixed != 0) {
                            fit_conv->FixParameter(0, fShapingFixed);
                        }
                        if (fStartPositionFixed != 0) {
                            fit_conv->FixParameter(1, MaxPeakBin - fStartPositionFixed);
                        }
                        if (fVarianceFixed != 0) {
                            fit_conv->FixParameter(2, fVarianceFixed);
                        }
                        if (fAmplitudeFixed != 0) {
                            fit_conv->FixParameter(3, singleSignal->GetData(MaxPeakBin) * fAmplitudeFixed);
                        }
                        // fit_conv->FixParameter(3, singleSignal->GetData(MaxPeakBin) * 100);
                        // fit_conv->FixParameter(3, singleSignal->GetIntegralInRange(MaxPeakBin - MinBinRange,
                        // MaxPeakBin + MaxBinRange));
    
                        h->Fit(fit_conv, "RMNQWW", "", MaxPeakBin - MinBinRange, MaxPeakBin + MaxBinRange);
                        // Options: L->Likelihood minimization, R->fit in range, N->No draw, Q->Quiet
    
                        Double_t sigma = 0;
                        for (int j = MaxPeakBin - MinBinRange; j < MaxPeakBin + MaxBinRange; j++) {
                            sigma += (h->GetBinContent(j) - fit_conv->Eval(j)) *
                                     (h->GetBinContent(j) - fit_conv->Eval(j));
                        }
                        Sigma[s] = TMath::Sqrt(sigma / (MinBinRange + MaxBinRange));
                        RatioSigmaMaxPeak[s] = Sigma[s] / h->GetBinContent(MaxPeakBin + 1);
                        RatioSigmaMaxPeakMean += RatioSigmaMaxPeak[s];
                        SigmaMean += Sigma[s];
                        ChiSquare[s] = fit_conv->GetChisquare();
                        ChiSquareMean += ChiSquare[s];
    
                        amplitudeFit[singleSignal->GetID()] = fit_conv->GetParameter("Amplitude");
                        shapingtimeFit[singleSignal->GetID()] = fit_conv->GetParameter("ShapingTime");
                        StartPositionFit[singleSignal->GetID()] = fit_conv->GetParameter("StartPosition");
                        variancegaussFit[singleSignal->GetID()] = fit_conv->GetParameter("VarianceGauss");
    
                        ratiosigmaamplitudeFit[singleSignal->GetID()] = RatioSigmaMaxPeak[s];
    
                        VGmean += fit_conv->GetParameter("VarianceGauss") / fit_conv->GetParError(2);
                        VGvariance += fit_conv->GetParameter("VarianceGauss") * fit_conv->GetParameter(2) /
                                      fit_conv->GetParError(2);
                        VGweights += 1 / fit_conv->GetParError(2);
    
                        h->Delete();
    
                    } else {
                        amplitudeFit[singleSignal->GetID()] = -1;
                        shapingtimeFit[singleSignal->GetID()] = -1;
                        StartPositionFit[singleSignal->GetID()] = -1;
                        variancegaussFit[singleSignal->GetID()] = -1;
    
                        ratiosigmaamplitudeFit[singleSignal->GetID()] = -1;
                    }
                }
            }
    
            //////////// Fitted parameters Map Observables /////////////
            SetObservableValue("FitAmplitude_map", amplitudeFit);
            SetObservableValue("FitShapingTime_map", shapingtimeFit);
            SetObservableValue("FitStartPosition_map", StartPositionFit);
            SetObservableValue("FitVarianceGauss_map", variancegaussFit);
    
            SetObservableValue("FitRatioSigmaMaxPeak_map", ratiosigmaamplitudeFit);
    
            Double_t maxVarianceGauss = 0;
            for (int k = 0; k < fRawSignalEvent->GetNumberOfSignals(); k++) {
                debug << variancegaussFit[fRawSignalEvent->GetSignal(k)->GetID()] << endl;
                if (variancegaussFit[fRawSignalEvent->GetSignal(k)->GetID()] > maxVarianceGauss) {
                    maxVarianceGauss = variancegaussFit[fRawSignalEvent->GetSignal(k)->GetID()];
                }
            }
            SetObservableValue("FitMaxVarianceGauss", maxVarianceGauss);
    
            Double_t SigmaMeanStdDev = 0;
            Double_t sigmaMeanStdDev = 0;
    
            //////////// Fitted event Observables /////////////
            if (fPointThreshold == 0 && fSignalThreshold == 0 && fPointsOverThreshold == 0 &&
                fBaseLineRange.X() == 0 && fBaseLineRange.Y() == 0) {
                /// Sigma Mean Observable
                SigmaMean = SigmaMean / (fRawSignalEvent->GetNumberOfSignals());
                /// Sigma Mean Standard Deviation Observable
                sigmaMeanStdDev = 0;
                for (int k = 0; k < fRawSignalEvent->GetNumberOfSignals(); k++) {
                    sigmaMeanStdDev += (Sigma[k] - SigmaMean) * (Sigma[k] - SigmaMean);
                }
                SigmaMeanStdDev = TMath::Sqrt(sigmaMeanStdDev / fRawSignalEvent->GetNumberOfSignals());
                /// Chi Square Mean Observable
                ChiSquareMean = ChiSquareMean / fRawSignalEvent->GetNumberOfSignals();
                /// Ratio Sigma MaxPeak Mean Observable
                RatioSigmaMaxPeakMean = RatioSigmaMaxPeakMean / fRawSignalEvent->GetNumberOfSignals();
            }
    
            else {
                /// Sigma Mean Observable
                SigmaMean = SigmaMean / goodSig;
                /// Sigma Mean Standard Deviation  Observable
                sigmaMeanStdDev = 0;
                for (int k = 0; k < fRawSignalEvent->GetNumberOfSignals(); k++) {
                    if (Sigma[k] != 0) {
                        sigmaMeanStdDev += (Sigma[k] - SigmaMean) * (Sigma[k] - SigmaMean);
                    }
                }
                SigmaMeanStdDev = TMath::Sqrt(sigmaMeanStdDev / goodSig);
                /// Chi Square Mean Observable
                ChiSquareMean = ChiSquareMean / goodSig;
                /// Ratio Sigma MaxPeak Mean Observable
                RatioSigmaMaxPeakMean = RatioSigmaMaxPeakMean / goodSig;
            }
    
            SetObservableValue("FitSigmaMean", SigmaMean);
            SetObservableValue("FitSigmaStdDev", SigmaMeanStdDev);
            SetObservableValue("FitChiSquareMean", ChiSquareMean);
            SetObservableValue("FitRatioSigmaMaxPeakMean", RatioSigmaMaxPeakMean);
    
            SetObservableValue("FitVarianceGaussWMean", VGmean / VGweights);
            SetObservableValue("FitVarianceGaussWStdDev",
                               TMath::Sqrt(VGvariance / VGweights - (VGmean / VGweights) * (VGmean / VGweights)));
    
            debug << "SigmaMean: " << SigmaMean << endl;
            debug << "SigmaMeanStdDev: " << SigmaMeanStdDev << endl;
            debug << "ChiSquareMean: " << ChiSquareMean << endl;
            debug << "RatioSigmaMaxPeakMean: " << RatioSigmaMaxPeakMean << endl;
            for (int k = 0; k < fRawSignalEvent->GetNumberOfSignals(); k++) {
                debug << "Standard deviation of signal number " << k << ": " << Sigma[k] << endl;
                debug << "Chi square of fit signal number " << k << ": " << ChiSquare[k] << endl;
                debug << "Sandard deviation divided by amplitude of signal number " << k << ": "
                      << RatioSigmaMaxPeak[k] << endl;
            }
    
            // If cut condition matches the event will be not registered.
            if (ApplyCut()) return NULL;
    
            return fRawSignalEvent;
        }
    
        /////----- Fit with AGET function -----/////
    
        if (fAgetFit == true) {
            for (int s = 0; s < fRawSignalEvent->GetNumberOfSignals(); s++) {
                TRestRawSignal* singleSignal = fRawSignalEvent->GetSignal(s);
                singleSignal->CalculateBaseLine(20, 150);
                int MaxPeakBin = singleSignal->GetMaxPeakBin();
    
                // ShaperSin function (AGET theoretical curve) times logistic function
                TF1* f = new TF1("Aget",
                                 "[2]*TMath::Exp(-3. * (x-[1])/[0] ) * (x-[1])/[0] * (x-[1])/[0] * (x-[1])/[0] "
                                 " * sin((x-[1])/[0])/(1+TMath::Exp(-10000*(x-[1])))",
                                 0, 511);
                f->SetParNames("ShapingTime", "StartPosition", "Amplitude");
    
                // Create histogram from signal
                Int_t nBins = singleSignal->GetNumberOfPoints();
                TH1D* h = new TH1D("histo", "Signal to histo", nBins, 0, nBins);
    
                for (int i = 0; i < nBins; i++) {
                    h->Fill(i, singleSignal->GetRawData(i) - singleSignal->GetBaseLine());
                    h->SetBinError(i, singleSignal->GetBaseLineSigma());
                }
    
                // Fit histogram with ShaperSin
                // f->SetParameters(32., 200, singleSignal->GetData(MaxPeakBin));
    
                // Read initial parameters
                if (fShapingInitialValue != 0) {
                    f->SetParameter(0, fShapingInitialValue);
                } else {
                    f->SetParameter(0, 32.);
                }
                if (fStartPositionInitialValue != 0) {
                    f->SetParameter(1, MaxPeakBin - fStartPositionInitialValue);
                } else {
                    f->SetParameter(1, MaxPeakBin - 25);
                }
                if (fAmplitudeInitialValue != 0) {
                    f->SetParameter(2, singleSignal->GetData(MaxPeakBin) * fAmplitudeInitialValue);
                } else {
                    f->SetParameter(2, singleSignal->GetData(MaxPeakBin));
                }
    
                // Read fixed parameters
                if (fShapingFixed != 0) {
                    f->FixParameter(0, fShapingFixed);
                }
                if (fStartPositionFixed != 0) {
                    f->FixParameter(1, MaxPeakBin - fStartPositionFixed);
                }
                if (fAmplitudeFixed != 0) {
                    f->FixParameter(2, singleSignal->GetData(MaxPeakBin) * fAmplitudeFixed);
                }
    
                h->Fit(f, "RMNQWW", "", MaxPeakBin - MinBinRange, MaxPeakBin + MaxBinRange);
                // Options: R->fit in range, N->No draw, Q->Quiet
    
                Double_t sigma = 0;
                for (int j = MaxPeakBin - 145; j < MaxPeakBin + 165; j++) {
                    sigma += (h->GetBinContent(j) - f->Eval(j)) * (h->GetBinContent(j) - f->Eval(j));
                }
                Sigma[s] = TMath::Sqrt(sigma / (145 + 165));
                RatioSigmaMaxPeak[s] = Sigma[s] / h->GetBinContent(MaxPeakBin + 1);
                RatioSigmaMaxPeakMean += RatioSigmaMaxPeak[s];
                SigmaMean += Sigma[s];
                ChiSquare[s] = f->GetChisquare();
                ChiSquareMean += ChiSquare[s];
    
                amplitudeFit[singleSignal->GetID()] = f->GetParameter(2);
                shapingtimeFit[singleSignal->GetID()] = f->GetParameter(0);
                StartPositionFit[singleSignal->GetID()] = f->GetParameter(1);
    
                ratiosigmaamplitudeFit[singleSignal->GetID()] = RatioSigmaMaxPeak[s];
    
                h->Delete();
            }
    
            //////////// Fitted parameters Map Observables /////////////
            SetObservableValue("FitAmplitude_map", amplitudeFit);
            SetObservableValue("FitShapingTime_map", shapingtimeFit);
            SetObservableValue("FitStartPosition_map", StartPositionFit);
    
            SetObservableValue("FitRatioSigmaMaxPeak_map", ratiosigmaamplitudeFit);
    
            //////////// Sigma Mean Observable /////////////
            SigmaMean = SigmaMean / (fRawSignalEvent->GetNumberOfSignals());
            SetObservableValue("FitSigmaMean", SigmaMean);
    
            //////////// Sigma Mean Standard Deviation  Observable /////////////
            Double_t sigmaMeanStdDev = 0;
            for (int k = 0; k < fRawSignalEvent->GetNumberOfSignals(); k++) {
                sigmaMeanStdDev += (Sigma[k] - SigmaMean) * (Sigma[k] - SigmaMean);
            }
            Double_t SigmaMeanStdDev = TMath::Sqrt(sigmaMeanStdDev / fRawSignalEvent->GetNumberOfSignals());
            SetObservableValue("FitSigmaStdDev", SigmaMeanStdDev);
    
            //////////// Chi Square Mean Observable /////////////
            ChiSquareMean = ChiSquareMean / fRawSignalEvent->GetNumberOfSignals();
            SetObservableValue("FitChiSquareMean", ChiSquareMean);
    
            //////////// Ratio Sigma MaxPeak Mean Observable /////////////
            RatioSigmaMaxPeakMean = RatioSigmaMaxPeakMean / fRawSignalEvent->GetNumberOfSignals();
            SetObservableValue("FitRatioSigmaMaxPeakMean", RatioSigmaMaxPeakMean);
    
            debug << "SigmaMean: " << SigmaMean << endl;
            debug << "SigmaMeanStdDev: " << SigmaMeanStdDev << endl;
            debug << "ChiSquareMean: " << ChiSquareMean << endl;
            debug << "RatioSigmaMaxPeakMean: " << RatioSigmaMaxPeakMean << endl;
            for (int k = 0; k < fRawSignalEvent->GetNumberOfSignals(); k++) {
                debug << "Standard deviation of signal number " << k << ": " << Sigma[k] << endl;
                debug << "Chi square of fit signal number " << k << ": " << ChiSquare[k] << endl;
                debug << "Sandard deviation divided by amplitude of signal number " << k << ": "
                      << RatioSigmaMaxPeak[k] << endl;
            }
    
            // If cut condition matches the event will be not registered.
            if (ApplyCut()) return NULL;
    
            return fRawSignalEvent;
        }
    }
    
    //// All pulses ////
    if (fAddAllPulses == true) {
        TRestRawSignal* allSig = new TRestRawSignal();
        for(int i=0; i<512; i++){
            double a = 0;
            for( int j=0; j<fRawSignalEvent->GetNumberOfSignals(); j++ ){
                TRestRawSignal* singleSignal = fRawSignalEvent->GetSignal(j);
                a+=singleSignal->GetData(i);
            }
            allSig->AddPoint(a);
        }
        allSig->CalculateBaseLine(fBaseLineRange.X(), fBaseLineRange.Y());
        //allSig->SetID(0);
        
        int MaxPeakBin = allSig->GetMaxPeakBin();
    
        // ShaperSin function (AGET theoretical curve) times logistic function
        TF1* f = new TF1("Aget",
                         "TMath::Exp(-3. * (x-[1])/[0] ) * (x-[1])/[0] * (x-[1])/[0] * (x-[1])/[0] "
                         " * sin((x-[1])/[0])/(1+TMath::Exp(-10000*(x-[1])))",
                         0, 511);
        f->SetParNames("ShapingTime", "StartPosition");

        // Gaussian pulse
        TF1* g = new TF1("pulse", "exp(-0.5*x*x/[0])*[1]", 0, 511);
        g->SetParNames("VarianceGauss", "Amplitude");

        // Convolution of AGET and gaussian functions
        TF1Convolution* conv = new TF1Convolution("Aget", "pulse", 0, 511, true);
        conv->SetRange(0, 511);
        conv->SetNofPointsFFT(10000);

        TF1* fit_conv = new TF1("fit", *conv, 0, 511, conv->GetNpar());
        fit_conv->SetParNames("ShapingTime", "StartPosition", "VarianceGauss", "Amplitude");

        // Create histogram from signal
        Int_t nBins = allSig->GetNumberOfPoints();
        TH1D* h = new TH1D("histo", "Signal to histo", nBins, 0, nBins);

        for (int i = 0; i < nBins; i++) {
            // h->Fill(i,
            // (singleSignal->GetRawData(i)-singleSignal->GetBaseLine())*100/singleSignal->GetData(MaxPeakBin));
            // h->SetBinError(i,
            // singleSignal->GetBaseLineSigma()*100/singleSignal->GetData(MaxPeakBin));
            h->Fill(i, allSig->GetRawData(i) - allSig->GetBaseLine());
            h->SetBinError(i, allSig->GetBaseLineSigma());
        }

        // Fit histogram with convolution
        // fit_conv->SetParameters(32., MaxPeakBin - 25., 1., singleSignal->GetData(MaxPeakBin)* 10);

        // Read initial parameters
        if (fShapingInitialValue != 0) {
            fit_conv->SetParameter(0, fShapingInitialValue);
        } else {
            fit_conv->SetParameter(0, 32.);
        }
        if (fStartPositionInitialValue != 0) {
            fit_conv->SetParameter(1, MaxPeakBin - fStartPositionInitialValue);
        } else {
            fit_conv->SetParameter(1, MaxPeakBin - 25);
        }
        if (fVarianceInitialValue != 0) {
            fit_conv->SetParameter(2, fVarianceInitialValue);
        } else {
            fit_conv->SetParameter(2, 1.);
        }
        if (fAmplitudeInitialValue != 0) {
            fit_conv->SetParameter(3, allSig->GetData(MaxPeakBin) * fAmplitudeInitialValue);
        } else {
            fit_conv->SetParameter(3, allSig->GetData(MaxPeakBin) * 10);
        }

        // Read fixed parameters
        if (fShapingFixed != 0) {
            fit_conv->FixParameter(0, fShapingFixed);
        }
        if (fStartPositionFixed != 0) {
            fit_conv->FixParameter(1, MaxPeakBin - fStartPositionFixed);
        }
        if (fVarianceFixed != 0) {
            fit_conv->FixParameter(2, fVarianceFixed);
        }
        if (fAmplitudeFixed != 0) {
            fit_conv->FixParameter(3, allSig->GetData(MaxPeakBin) * fAmplitudeFixed);
        }
        // fit_conv->FixParameter(3, singleSignal->GetData(MaxPeakBin) * 100);
        // fit_conv->FixParameter(3, singleSignal->GetIntegralInRange(MaxPeakBin - MinBinRange,
        // MaxPeakBin + MaxBinRange));

        h->Fit(fit_conv, "RMNQWW", "", MaxPeakBin - MinBinRange, MaxPeakBin + MaxBinRange);
        // Options: L->Likelihood minimization, R->fit in range, N->No draw, Q->Quiet

        Double_t sigma = 0;
        for (int j = MaxPeakBin - MinBinRange; j < MaxPeakBin + MaxBinRange; j++) {
            sigma +=
                (h->GetBinContent(j) - fit_conv->Eval(j)) * (h->GetBinContent(j) - fit_conv->Eval(j));
        }
        
        //Sigma[s] = TMath::Sqrt(sigma / (MinBinRange + MaxBinRange));
        //RatioSigmaMaxPeak[s] = Sigma[s] / h->GetBinContent(MaxPeakBin + 1);
        //RatioSigmaMaxPeakMean += RatioSigmaMaxPeak[s];
        //SigmaMean += Sigma[s];
        //ChiSquare[s] = fit_conv->GetChisquare();
        //ChiSquareMean += ChiSquare[s];

        amplitudeFit[allSig->GetID()] = fit_conv->GetParameter("Amplitude");
        shapingtimeFit[allSig->GetID()] = fit_conv->GetParameter("ShapingTime");
        StartPositionFit[allSig->GetID()] = fit_conv->GetParameter("StartPosition");
        variancegaussFit[allSig->GetID()] = fit_conv->GetParameter("VarianceGauss");

        //ratiosigmaamplitudeFit[allSig->GetID()] = RatioSigmaMaxPeak[s];

        VGmean += fit_conv->GetParameter("VarianceGauss") / fit_conv->GetParError(2);
        VGvariance += fit_conv->GetParameter("VarianceGauss") * fit_conv->GetParameter(2) /
                      fit_conv->GetParError(2);
        VGweights += 1 / fit_conv->GetParError(2);

        
        //////////// Fitted parameters Map Observables /////////////
        SetObservableValue("FitAmplitude_map", amplitudeFit);
        SetObservableValue("FitShapingTime_map", shapingtimeFit);
        SetObservableValue("FitStartPosition_map", StartPositionFit);
        SetObservableValue("FitVarianceGauss_map", variancegaussFit);

        SetObservableValue("FitRatioSigmaMaxPeak_map", ratiosigmaamplitudeFit);

        debug << variancegaussFit[allSig->GetID()] << endl;
        SetObservableValue("FitMaxVarianceGauss", variancegaussFit[allSig->GetID()]);

        Double_t SigmaMeanStdDev = 0;
        Double_t sigmaMeanStdDev = 0;

        //////////// Fitted event Observables /////////////
        
        
        /// Chi Square Mean Observable
        ChiSquareMean = ChiSquareMean / fRawSignalEvent->GetNumberOfSignals();
        /// Ratio Sigma MaxPeak Mean Observable
        RatioSigmaMaxPeakMean = RatioSigmaMaxPeakMean / fRawSignalEvent->GetNumberOfSignals();
        
        SetObservableValue("FitSigmaMean", TMath::Sqrt(sigma / (MinBinRange + MaxBinRange)));
        
        SetObservableValue("FitChiSquareMean", fit_conv->GetChisquare());
        SetObservableValue("FitRatioSigmaMaxPeakMean", TMath::Sqrt(sigma / (MinBinRange + MaxBinRange)) / h->GetBinContent(MaxPeakBin + 1));

        SetObservableValue("FitVarianceGaussWMean", fit_conv->GetParameter("VarianceGauss"));
        SetObservableValue("FitVarianceGaussWStdDev", fit_conv->GetParError(2));

        debug << "SigmaMean: " << SigmaMean << endl;
        debug << "SigmaMeanStdDev: " << SigmaMeanStdDev << endl;
        debug << "ChiSquareMean: " << ChiSquareMean << endl;
        debug << "RatioSigmaMaxPeakMean: " << RatioSigmaMaxPeakMean << endl;
        
        h->Delete();
        
        

        // If cut condition matches the event will be not registered.
        if (ApplyCut()) return NULL;

        return fRawSignalEvent;

    
    }
    return NULL;
}

///////////////////////////////////////////////
/// \brief Function to include required actions after all events have been
/// processed. This method will write the channels histogram.
///
void TRestRawSignalFitEventProcess::EndProcess() {
    // Function to be executed once at the end of the process
    // (after all events have been processed)

    // Start by calling the EndProcess function of the abstract class.
    // Comment this if you don't want it.
    // TRestEventProcess::EndProcess();
}
