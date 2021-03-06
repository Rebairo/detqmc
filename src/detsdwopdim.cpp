/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  See the enclosed file LICENSE for a copy or if
 * that was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2017 Max H. Gerlach
 * 
 * */

/*
 * detsdwopdim.cpp
 *
 *  Created on: Aug 04, 2014
 *      Author: gerlach
 */

#include <cmath>
#include <numeric>
#include <functional>
#include <array>
#include <tuple>
#include <cassert>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "boost/assign/std/vector.hpp"    // 'operator+=()' for vectors
#include "boost/archive/binary_oarchive.hpp"
#include "boost/archive/binary_iarchive.hpp"
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/back_inserter.hpp"
#include "boost/iostreams/device/array.hpp"
#include "boost/filesystem.hpp"
#pragma GCC diagnostic pop
#include "observable.h"
#include "detsdwopdim.h"
#include "exceptions.h"
#include "timing.h"
#include "checkarray.h"
#include "tools.h"
#include "toolsdebug.h"
#include "pytools.h"

namespace fs = boost::filesystem;

template<CheckerboardMethod CBM, int OPDIM>
void createReplica(std::unique_ptr<DetSDW<CBM, OPDIM>>& replica_out,
                   RngWrapper& rng, ModelParamsDetSDW pars,
                   DetModelLoggingParams loggingPars /*def arg*/,
                   const std::string& logfiledir_ /*def arg*/) {
    pars = updateTemperatureParameters(pars);

    pars.check();

    std::string logfiledir = ((logfiledir_ != "") ? logfiledir_ : "./");

    loggingPars.logSV_filename = (fs::path(logfiledir) /
                                  fs::path("sv.log")).string();
    loggingPars.logSV_max_filename = (fs::path(logfiledir) /
                                      fs::path("svmax.log")).string();
    loggingPars.logSV_min_filename = (fs::path(logfiledir) /
                                      fs::path("svmin.log")).string();
    loggingPars.logDetRatio_filename = (fs::path(logfiledir) /
                                        fs::path("detratio.log")).string();
    loggingPars.logGreen_filename = (fs::path(logfiledir) /
                                     fs::path("green.log")).string();
    loggingPars.check();

    assert((pars.checkerboard and (CBM == CB_ASSAAD_BERG)) or
           (not pars.checkerboard and (CBM == CB_NONE))
        );
    assert(pars.opdim == OPDIM);

    // Update chemical potential setting -- if mux and muy are given, they supersede mu
    if (not (pars.specified.count("mux") and pars.specified.count("muy"))) {
        pars.mux = pars.mu;
        pars.muy = pars.mu;
    }

    replica_out = std::unique_ptr<DetSDW<CBM, OPDIM>>(
        new DetSDW<CBM, OPDIM>(rng, pars, loggingPars, logfiledir));
}
//explicit instantiations:
#ifndef DETSDW_NO_O1
template void createReplica(std::unique_ptr<DetSDW<CB_NONE, 1>>& replica_out,
                            RngWrapper& rng, ModelParamsDetSDW pars,
                            DetModelLoggingParams loggingPars /*def arg*/,
                            const std::string& logfiledir);
template void createReplica(std::unique_ptr<DetSDW<CB_ASSAAD_BERG, 1>>& replica_out,
                            RngWrapper& rng, ModelParamsDetSDW pars,
                            DetModelLoggingParams loggingPars /*def arg*/,
                            const std::string& logfiledir);
#endif //DETSDW_NO_O1
#ifndef DETSDW_NO_O2
template void createReplica(std::unique_ptr<DetSDW<CB_NONE, 2>>& replica_out,
                            RngWrapper& rng, ModelParamsDetSDW pars,
                            DetModelLoggingParams loggingPars /*def arg*/,
                            const std::string& logfiledir);
template void createReplica(std::unique_ptr<DetSDW<CB_ASSAAD_BERG, 2>>& replica_out,
                            RngWrapper& rng, ModelParamsDetSDW pars,
                            DetModelLoggingParams loggingPars /*def arg*/,
                            const std::string& logfiledir);
#endif //DETSDW_NO_O2
#ifndef DETSDW_NO_O3
template void createReplica(std::unique_ptr<DetSDW<CB_NONE, 3>>& replica_out,
                            RngWrapper& rng, ModelParamsDetSDW pars,
                            DetModelLoggingParams loggingPars /*def arg*/,
                            const std::string& logfiledir);
template void createReplica(std::unique_ptr<DetSDW<CB_ASSAAD_BERG, 3>>& replica_out,
                            RngWrapper& rng, ModelParamsDetSDW pars,
                            DetModelLoggingParams loggingPars /*def arg*/,
                            const std::string& logfiledir);
#endif //DETSDW_NO_O3


// define these constants, values are set in the header file
#if __GNUC__ == 4 && __GNUC_MINOR < 7
//work around g++ bug
#define const
#endif// __GNUC__ == 4 && __GNUC_MINOR < 7
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::InitialPhiDelta;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::InitialAngleDelta;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::InitialScaleDelta;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::MinScaleDelta;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::MaxScaleDelta;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::MinAngleDelta;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::MaxAngleDelta;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const uint32_t DetSDW<Checkerboard, OPDIM>::AdjustmentData::AccRatioAdjustmentSamples;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::phiDeltaGrowFactor;
template<CheckerboardMethod Checkerboard, int OPDIM>
constexpr const num DetSDW<Checkerboard, OPDIM>::AdjustmentData::phiDeltaShrinkFactor;
#if __GNUC__ == 4 && __GNUC_MINOR < 7
//work around g++ bug
#undef const
#endif// __GNUC__ == 4 && __GNUC_MINOR < 7





//initial values for field components chosen from this range:
const num PhiLow = -1;
const num PhiHigh = 1;


template<CheckerboardMethod CB, int OPDIM>
DetSDW<CB, OPDIM>::DetSDW(RngWrapper& rng_, const ModelParams& pars_,
                          const DetModelLoggingParams& loggingPars /*def arg*/,
                          const std::string& logfiledir_ /*def arg*/) :
    Base(pars_, (pars_.turnoffFermions ? 1  // save memory in no fermion case
                 : (MatrixSizeFactor * pars_.L*pars_.L)),
         loggingPars),
    smalleye(arma::eye<MatData>(MatrixSizeFactor, MatrixSizeFactor)),
    rng(rng_), normal_distribution(rng),
    pars(pars_),
    us(),                       // UpdateStatistics
    hopHor(), hopVer(), sinhHopHor(), sinhHopVer(), coshHopHor(), coshHopVer(),
    sinhHopHorHalf(), sinhHopVerHalf(), coshHopHorHalf(), coshHopVerHalf(),
    mu(),
    spaceNeigh(pars.L), timeNeigh(pars.m),
    propK(), propKx(propK[XBAND]), propKy(propK[YBAND]),
    propK_half(), propKx_half(propK_half[XBAND]), propKy_half(propK_half[YBAND]),
    propK_half_inv(), propKx_half_inv(propK_half_inv[XBAND]), propKy_half_inv(propK_half_inv[YBAND]),
    g(green[0]), g_inv_sv(green_inv_sv[0]),
    phi(pars.N, OPDIM, pars.m+1), cdwl(pars.N, pars.m+1),
    coshTermPhi(pars.N, pars.m+1), sinhTermPhi(pars.N, pars.m+1),
    coshTermCDWl(pars.N, pars.m+1), sinhTermCDWl(pars.N, pars.m+1),
    ad(pars),                   // AdjustmentData
    performedSweeps(0),
    meanPhi(), normMeanPhi(0), phiRhoS_Gs(0), phiRhoS_Gc(0),
    associatedEnergy(0),
    // kgreenXUP(), kgreenYDOWN(), kgreenXDOWN(), kgreenYUP(),
    greenXUPXUP_summed(), greenYDOWNYDOWN_summed(), greenXDOWNXDOWN_summed(), greenYUPYUP_summed(),
    greenXUPYDOWN_summed(), greenYDOWNXUP_summed(),
    greenK0(), greenLocal(),
    kOcc(), kOccX(kOcc[XBAND]), kOccY(kOcc[YBAND]),
    // occ(), occX(occ[XBAND]), occY(occ[YBAND]),
    pairPlusMax(0.0), pairMinusMax(0.0),
    pairPlus(), pairMinus(),
    fermionEkinetic(0), fermionEcouple(0),
    // occCorr(), chargeCorr(), occCorrFT(), chargeCorrFT(), occDiffSq(),
    timeslices_included_in_measurement(),
    dud(pars.N, pars.delaySteps), gmd(pars.N, m, pars_.turnoffFermions),
    greenConsistencyLogger(logfiledir_, loggingPars.logGreenConsistency), detRatioLogging(), greenLogging()
{
    //use contents of ModelParams pars
    assert((pars.checkerboard and CB != CB_NONE) or (not pars.checkerboard and CB == CB_NONE));
    assert(pars.N == pars.L*pars.L);
    assert(pars.d == 2);

    //zero some dynamic data
    phi.zeros();
    // std::cout << phi;
    cdwl.zeros();
    coshTermPhi.zeros();
    sinhTermPhi.zeros();
    coshTermCDWl.ones();
    sinhTermCDWl.zeros();

    if (not pars.phiFixed) {
        setupRandomField();
    } else {
        setupConstantField();
    }

    //weak magnetic field in z-direction
    if (pars.weakZflux) {
        zmag[XUP]   = +1.0 / (pars.N);
        zmag[YDOWN] = +1.0 / (pars.N);
        zmag[YUP]   = -1.0 / (pars.N);
        zmag[XDOWN] = -1.0 / (pars.N);        
    } else {
        zmag[XUP]   = 0.0;
        zmag[YDOWN] = 0.0;
        zmag[YUP]   = 0.0;
        zmag[XDOWN] = 0.0;        
    }

    //hopping constants. These are the t_ij in sum_<i,j> -t_ij c^+_i c_j
    //So for actual calculations an additional minus-sign needs to be included.
    //In the case of anti-periodic boundaries between i and j, another extra minus-sign
    //must be added.
    hopHor[XBAND] = pars.txhor;
    hopVer[XBAND] = pars.txver;
    hopHor[YBAND] = pars.tyhor;
    hopVer[YBAND] = pars.tyver;
    //precalculate hyperbolic functions, used in checkerboard decomposition [without magnetic field]
    using std::sinh; using std::cosh;
    num dtauHere = pars.dtau;                // to fix capture issues
    for_each_band( [this, dtauHere](Band band) {
            sinhHopHor[band] = sinh(-dtauHere * hopHor[band]);
            coshHopHor[band] = cosh(-dtauHere * hopHor[band]);
            sinhHopVer[band] = sinh(-dtauHere * hopVer[band]);
            coshHopVer[band] = cosh(-dtauHere * hopVer[band]);
            sinhHopHorHalf[band] = sinh(-0.5*dtauHere * hopHor[band]);
            coshHopHorHalf[band] = cosh(-0.5*dtauHere * hopHor[band]);
            sinhHopVerHalf[band] = sinh(-0.5*dtauHere * hopVer[band]);
            coshHopVerHalf[band] = cosh(-0.5*dtauHere * hopVer[band]);
        } );
    if (pars.weakZflux) {
        // needed for checkerboard computations when we have a magnetic field
        precalc_4site_hopping_exponentials(); 
    }
    
    //chemical potential
    //These are the \mu in sum_<i> -\mu c^+_i c_i
    //So for actual calculations an additional minus-sign needs to be included.
    mu[XBAND] = pars.mux;
    mu[YBAND] = pars.muy;

    if (not pars.turnoffFermions) {
        setupPropK();
    }

    setupUdVStorage_and_calculateGreen();

    using std::cref;
    using namespace boost::assign;
    obsScalar += ScalarObservable(cref(normMeanPhi), "normMeanPhi", "nmp"),
                 ScalarObservable(cref(associatedEnergy), "associatedEnergy", "");

    if (OPDIM == 2) {
        obsScalar += ScalarObservable(cref(phiRhoS_Gs), "phiRhoS_Gs", "");
        obsScalar += ScalarObservable(cref(phiRhoS_Gc), "phiRhoS_Gc", "");
    }

    if (not (pars.turnoffFermions or pars.turnoffFermionMeasurements)) {
    
        obsScalar +=
            ScalarObservable(cref(pairPlusMax), "pairPlusMax", "ppMax"),
            ScalarObservable(cref(pairMinusMax), "pairMinusMax", "pmMax")// ,
            // ScalarObservable(cref(fermionEkinetic), "fermionEkinetic", "fEkin"),
            // ScalarObservable(cref(fermionEcouple), "fermionEcouple", "fEcouple")
            ;

        kOccX.zeros(pars.N);
        kOccY.zeros(pars.N);
        obsVector += VectorObservable(cref(kOccX), pars.N, "kOccX", "nkx"),
            VectorObservable(cref(kOccY), pars.N, "kOccY", "nky");
        // output some different sectors of the Green's function in the
        // momentum space representation
        // obsVector += VectorObservable(cref(kgreenXUP), pars.N, "kgreenXUP", ""),
        //     VectorObservable(cref(kgreenYDOWN), pars.N, "kgreenYDOWN", ""),
        //     VectorObservable(cref(kgreenXUP), pars.N, "kgreenXUP", ""),
        //     VectorObservable(cref(kgreenYDOWN), pars.N, "kgreenYDOWN", "");
        // kgreenXUP.zeros(pars.N);
        // kgreenYDOWN.zeros(pars.N);
        // kgreenXDOWN.zeros(pars.N);
        // kgreenYUP.zeros(pars.N);

        obsScalar += ScalarObservable(cref(greenK0), "greenK0", ""),
            ScalarObservable(cref(greenLocal), "greenLocal", "");

        // occX.zeros(pars.N);
        // occY.zeros(pars.N);
        // obsVector += VectorObservable(cref(occX), pars.N, "occX", "nx"),
        //     VectorObservable(cref(occY), pars.N, "occY", "ny");

        //attention:
        // these do not have valid entries for site 0
        pairPlus.zeros(pars.N);
        pairMinus.zeros(pars.N);
        obsVector += VectorObservable(cref(pairPlus), pars.N, "pairPlus", "pp"),
            VectorObservable(cref(pairMinus), pars.N, "pairMinus", "pm");

        // const Band BandValues[2] = {XBAND, YBAND};
        // for (Band b1 : BandValues) {
        //     for (Band b2 : BandValues) {
        //         MatNum& occC = occCorr(b1, b2);
        //         occC.zeros(pars.N,pars.N);
        //         VecNum& occCFT = occCorrFT(b1, b2);
        //         occCFT.zeros(pars.N);
        //         obsVector += VectorObservable(cref(occCFT), pars.N, "occCorrFT" + bandstr(b1) + bandstr(b2), "");
        //     }
        // }

        // chargeCorr.zeros(pars.N,pars.N);
        // chargeCorrFT.zeros(pars.N);
        // obsVector += VectorObservable(cref(chargeCorrFT), pars.N, "chargeCorrFT", "");

        occDiffSq = 0.0;
        obsScalar += ScalarObservable(cref(occDiffSq), "occDiffSq", "");
    }
    
    consistencyCheck();

    // for consistency checks:
    if (loggingParams.checkAndLogDetRatio) {
        detRatioLogging = std::unique_ptr<DoubleVectorWriterSuccessive>(
            new DoubleVectorWriterSuccessive(
                loggingParams.logDetRatio_filename,
                false // append to file = false: always start a new file for this
                )
            );
        detRatioLogging->addHeaderText("Attention: this file is recreated and the log restarted for each run of the program. It is not continued if the simulation is resumed from a saved state.");
        detRatioLogging->addHeaderText("Here we log the difference of two possible evaluations of the Green's function determinant ratio");
        detRatioLogging->writeHeader();
    }
    if (loggingParams.checkAndLogGreen) {
        greenLogging = std::unique_ptr<DoubleVectorWriterSuccessive>(
            new DoubleVectorWriterSuccessive(
                loggingParams.logGreen_filename,
                false // append to file = false: always start a new file for this
                )
            );
        greenLogging->addHeaderText("Attention: this file is recreated and the log restarted for each run of the program. It is not continued if the simulation is resumed from a saved state.");
        greenLogging->addHeaderText("Here we log the maximum absolute difference of two possible evaluations of the updated Green's function");
        greenLogging->writeHeader();
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::setupUdVStorage_and_calculateGreen() {
    if (not pars.turnoffFermions) {
        //setupUdVStorage_and_calculateGreen_skeleton(sdwComputeBmat(this));
        setupUdVStorage_and_calculateGreen_skeleton(sdwLeftMultiplyBmat(this));
    } else {
        g.zeros();
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::setupUdVStorage_and_calculateGreen_forTimeslice(
    uint32_t timeslice) {
    if (not pars.turnoffFermions) {
        //setupUdVStorage_and_calculateGreen_skeleton(sdwComputeBmat(this));
        setupUdVStorage_and_calculateGreen_forTimeslice_skeleton(
            timeslice, sdwLeftMultiplyBmat(this));
    } else {
        g.zeros();
    }
}

template<CheckerboardMethod CB, int OPDIM>
DetSDW<CB, OPDIM>::~DetSDW() {
}

template<CheckerboardMethod CB, int OPDIM>
uint32_t DetSDW<CB, OPDIM>::getSystemN() const {
    return pars.N;
}

template<CheckerboardMethod CB, int OPDIM>
MetadataMap DetSDW<CB, OPDIM>::prepareModelMetadataMap() const {
    MetadataMap meta = pars.prepareMetadataMap();
#define META_INSERT(VAR) {meta[#VAR] = numToString(VAR);}
    if (pars.globalShift) {
    	num globalShiftAccRatio = 0.;
        if (us.attemptedGlobalShifts > 0) {
            globalShiftAccRatio =
            num(us.acceptedGlobalShifts) / num(us.attemptedGlobalShifts);
        }
    	META_INSERT(globalShiftAccRatio);
    }
    if (pars.wolffClusterUpdate) {
        num wolffClusterUpdateAccRatio = 0.0;
        if (us.attemptedWolffClusterUpdates) {
            wolffClusterUpdateAccRatio =
                num(us.acceptedWolffClusterUpdates) /
                num(us.attemptedWolffClusterUpdates);
        }
    	META_INSERT(wolffClusterUpdateAccRatio);
        num averageAcceptedWolffClusterSize = 0.0;
        if (us.acceptedWolffClusterUpdates) {
            averageAcceptedWolffClusterSize =
                us.addedWolffClusterSize / num(us.acceptedWolffClusterUpdates);
        }
    	META_INSERT(averageAcceptedWolffClusterSize);
    }
    if (pars.wolffClusterShiftUpdate) {
        num wolffClusterShiftUpdateAccRatio = 0.;
        if (us.attemptedWolffClusterShiftUpdates) {
             wolffClusterShiftUpdateAccRatio =
                 num(us.acceptedWolffClusterShiftUpdates) /
                 num(us.attemptedWolffClusterShiftUpdates);
        }
    	META_INSERT(wolffClusterShiftUpdateAccRatio);
        num averageAcceptedWolffClusterSize = 0.;
        if (us.acceptedWolffClusterShiftUpdates) {
            averageAcceptedWolffClusterSize =
                us.addedWolffClusterSize / num(us.acceptedWolffClusterShiftUpdates);
        }
    	META_INSERT(averageAcceptedWolffClusterSize);
    }
#undef META_INSERT
    return meta;
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::initMeasurements() {
    timing.start("sdw-measure");

    timeslices_included_in_measurement.clear();

    //meanPhi
    meanPhi.zeros();
    normMeanPhi = 0;

    // bosonic spin stiffness
    if (OPDIM == 2) {
        phiRhoS_Gs = 0.0;
        phiRhoS_Gc = 0.0;
    }

    associatedEnergy = 0;

    if (not (pars.turnoffFermions or pars.turnoffFermionMeasurements)) {

        if (pars.dumpGreensFunction) {
            // some sectors of the momentum space Green's function,
            // helpers:
            greenXUPXUP_summed.zeros(pars.N, pars.N);
            greenYDOWNYDOWN_summed.zeros(pars.N, pars.N);
            if (OPDIM == 3) {
                greenXDOWNXDOWN_summed.zeros(pars.N, pars.N);
                greenYUPYUP_summed.zeros(pars.N, pars.N);
            }
            greenXUPYDOWN_summed.zeros(pars.N, pars.N);
            greenYDOWNXUP_summed.zeros(pars.N, pars.N);
        }
        
        // scalar functions of the Green's function
        greenK0 = 0.;
        greenLocal = 0.;

        // //fermion occupation number -- real space
        // occX.zeros(pars.N);
        // occY.zeros(pars.N);

        //fermion occupation number -- k-space
        kOccX.zeros(pars.N);
        kOccY.zeros(pars.N);

        //equal-time pairing-correlations
        pairPlus.zeros(pars.N);
        pairMinus.zeros(pars.N);

        // // Fermionic energy contribution
        // fermionEkinetic = 0;
        // fermionEcouple = 0;

        // // band occupation / charge correlations
        // const Band BandValues[2] = {XBAND, YBAND};
        // for (Band b1 : BandValues) {
        //     for (Band b2 : BandValues) {
        //         MatNum& occC = occCorr(b1, b2);
        //         occC.zeros(pars.N,pars.N);
        //     }
        // }
        occDiffSq = 0.0;

    }

    timing.stop("sdw-measure");
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::measure(uint32_t timeslice) {

    // This function *adds* the measurements for the current timeslice
    // to the observable variable, which must have been zero'ed in
    // initMeasurements.  Then finishMeasurements() will divide the
    // observable variable to get the average over all timeslices.

    timing.start("sdw-measure");

    // to ease notation in here
    const auto L = pars.L;
    const auto N = pars.N;

    timeslices_included_in_measurement.insert(timeslice);

    // bosonic spin stiffness
    if (OPDIM == 2) {
        for (uint32_t site = 0; site < N; ++site) {
            Phi phi_site   = getPhi(site, timeslice);
            Phi phi_xneigh = getPhi(spaceNeigh(XPLUS, site), timeslice);
            Phi phi_yneigh = getPhi(spaceNeigh(YPLUS, site), timeslice);

            phiRhoS_Gc += arma::dot(phi_site, phi_xneigh) +
                          arma::dot(phi_site, phi_yneigh);

            phiRhoS_Gs += phi_xneigh[0] * phi_site[1] - 
                          phi_xneigh[1] * phi_site[0];
        }
    }

    //normphi, meanPhi, sdw-susceptibility, associatedEnergy
    for (uint32_t site = 0; site < pars.N; ++site) {
        Phi phi_site = getPhi(site, timeslice);
        meanPhi += phi_site;
        associatedEnergy += arma::dot(phi_site, phi_site);
    }

    

    if (not (pars.turnoffFermions or pars.turnoffFermionMeasurements)) {

        MatData gshifted = shiftGreenSymmetric();

        // some sectors of the momentum space Green's function
        // helpers:
        auto gblock = [&gshifted, N](uint32_t row, uint32_t col) {
            return gshifted.submat(row * N, col * N,
                                   (row + 1) * N - 1, (col + 1) * N - 1);
        };
        if (pars.dumpGreensFunction) {
            greenXUPXUP_summed     += gblock(0, 0);
            greenYDOWNYDOWN_summed += gblock(1, 1);
            if (OPDIM == 3) {
                greenXDOWNXDOWN_summed += gblock(2, 2);
                greenYUPYUP_summed     += gblock(3, 3);
            }
            greenXUPYDOWN_summed += gblock(0, 1);
            greenYDOWNXUP_summed += gblock(1, 0);
        }
        
        // scalar functions of the Green's function
        if (OPDIM == 3) {
            greenK0 += std::real(arma::accu( gshifted ));
        }
        else {
            // only the 2x2 top-left and 2x2 bottom-right blocks of G are non-zero
            //
            // Sum [ Green ] = Sum [ Green_XUP_YDOWN ] + Sum [ Green_XDOWN_YUP ]
            //               = Sum [ Green_XUP_YDOWN ] + Sum [ Green_XUP_YDOWN^(*) ]
            //               = 2 * Sum [ Real ( Green_XUP_YDOWN ) ]
            greenK0 += 2 * std::real( arma::accu( gshifted ) );
        }
        
        
        if (OPDIM == 3) {
            // cpx local_with_imag = arma::trace(gshifted) / (4. * N);
            // assert( std::abs(std::imag(local_with_imag)) < 1E-14 );
            greenLocal += std::real(arma::trace(gshifted)) / (4. * N);
        }
        else {
            // Trace [ Green ] = Trace [ Green_XUP_YDOWN ] + Trace [ Green_XDOWN_YUP ]
            //                 = Trace [ Green_XUP_YDOWN ] + Trace [ Green_XUP_YDOWN^(*) ]
            //                 = 2 * Trace [ Real ( Green_XUP_YDOWN ) ]
            greenLocal += 2. * std::real(arma::trace( gshifted )) / (4. * N);
        }

        //helper function to access the Green's function elements for the
        //current time slice, definition depending on OPDIM
        // *1 is for the row index,
        // *2 is for the column index
        auto gl1 = [this, N, &gshifted](uint32_t site1, BandSpin bs1,
                                        uint32_t site2, BandSpin bs2) -> DataType {
            static_assert(XUP == 0, "XUP wrong"); static_assert(YDOWN == 1, "YDOWN wrong");
            static_assert(XDOWN == 2, "XDOWN wrong"); static_assert(YUP == 3, "YUP wrong");
            if (OPDIM == 3) {
                return gshifted(site1 + N*bs1, site2 + N*bs2);
            }
            else {
                if ((bs1 == XUP or bs1 == YDOWN) and (bs2 == XUP or bs2 == YDOWN)) {
                    return gshifted(site1 + N*bs1, site2 + N*bs2);
                }
                else if ((bs1 == XDOWN or bs1 == YUP) and (bs2 == XDOWN or bs2 == YUP)) {
                    return std::conj(gshifted(site1 + N*(bs1-2), site2 + N*(bs2-2)));
                }
                else {
                    return DataType(0);
                }
            }
        };
        auto gl = [this, N, &gl1](uint32_t site1, Band band1, Spin spin1,
                                  uint32_t site2, Band band2, Spin spin2) -> DataType {
            typedef DetSDW<CB,OPDIM> D;
            BandSpin bs1 = D::getBandSpin(band1, spin1);
            BandSpin bs2 = D::getBandSpin(band2, spin2);
            return gl1(site1, bs1, site2, bs2);
        };

        // //fermion occupation number -- real space
        // for (uint32_t i = 0; i < N; ++i) {
        //     occX[i] += std::real(gl1(i, XUP, i, XUP) + gl1(i, XDOWN, i, XDOWN));
        //     occY[i] += std::real(gl1(i, YUP, i, YUP) + gl1(i, YDOWN, i, YDOWN));
        // }

        //fermion occupation number -- k-space
        static const num pi = M_PI;
        //offset k-components for antiperiodic bc
        num offset_x = 0.0;
        num offset_y = 0.0;
        if (pars.bc == BC_Type::APBC_X or pars.bc == BC_Type::APBC_XY) {
            offset_x = 0.5;
        }
        if (pars.bc == BC_Type::APBC_Y or pars.bc == BC_Type::APBC_XY) {
            offset_y = 0.5;
        }
        for (uint32_t ksite = 0; ksite < N; ++ksite) {
            uint32_t ksitey = ksite / L;
            uint32_t ksitex = ksite % L;
            num ky = -pi + (num(ksitey) + offset_y) * 2*pi / num(L);
            num kx = -pi + (num(ksitex) + offset_x) * 2*pi / num(L);

            for (uint32_t i = 0; i < N; ++i) {
                num iy = num(i / L);
                num ix = num(i % L);
                for (uint32_t j = 0; j  < N; ++j) {
                    num jy = num(j / L);
                    num jx = num(j % L);

                    num argument = kx * (ix - jx) + ky * (iy - jy);
                    cpx phase = std::exp(cpx(0, argument));

                    DataType green_x_up   = gl1(i, XUP, j, XUP);
                    DataType green_x_down = gl1(i, XDOWN, j, XDOWN);
                    DataType green_y_up   = gl1(i, YUP, j, YUP);
                    DataType green_y_down = gl1(i, YDOWN, j, YDOWN);

                    cpx x_cpx = phase * (green_x_up + green_x_down);
                    cpx y_cpx = phase * (green_y_up + green_y_down);

                    kOccX[ksite] += std::real(x_cpx);
                    kOccY[ksite] += std::real(y_cpx);
                }
            }
        }

        //equal-time pairing-correlations
        //-------------------------------

        for (uint32_t i = 0; i < N; ++i) {
            //            checkarray<std::tuple<uint32_t,uint32_t>, 2> sitePairs = {
            //                    std::make_tuple(i, 0), std::make_tuple(0, i)
            //            };
            //compiler-compatibilty fix
            std::tuple<uint32_t,uint32_t> sitePairs[2] = {
                std::tuple<uint32_t,uint32_t>(i, 0),
                std::tuple<uint32_t,uint32_t>(0, i)
            };

            DataType pairPlusCpx(0);
            DataType pairMinusCpx(0);

            for (auto sites : sitePairs) {
                uint32_t siteA = std::get<0>(sites);
                uint32_t siteB = std::get<1>(sites);

                // the following two unwieldy sums have been evaluated with the Mathematica
                // notebook pairing-corr.nb (and they match the terms calculated by hand on paper)
                pairPlusCpx += DataType(-4.0) * (
                    gl(siteA, XBAND, SPINDOWN, siteB, XBAND, SPINUP)*gl(siteA, XBAND, SPINUP, siteB, XBAND, SPINDOWN) -
                    gl(siteA, XBAND, SPINDOWN, siteB, XBAND, SPINDOWN)*gl(siteA, XBAND, SPINUP, siteB, XBAND, SPINUP) +
                    gl(siteA, XBAND, SPINDOWN, siteB, YBAND, SPINUP)*gl(siteA, XBAND, SPINUP, siteB, YBAND, SPINDOWN) -
                    gl(siteA, XBAND, SPINDOWN, siteB, YBAND, SPINDOWN)*gl(siteA, XBAND, SPINUP, siteB, YBAND, SPINUP) +
                    gl(siteA, YBAND, SPINDOWN, siteB, XBAND, SPINUP)*gl(siteA, YBAND, SPINUP, siteB, XBAND, SPINDOWN) -
                    gl(siteA, YBAND, SPINDOWN, siteB, XBAND, SPINDOWN)*gl(siteA, YBAND, SPINUP, siteB, XBAND, SPINUP) +
                    gl(siteA, YBAND, SPINDOWN, siteB, YBAND, SPINUP)*gl(siteA, YBAND, SPINUP, siteB, YBAND, SPINDOWN) -
                    gl(siteA, YBAND, SPINDOWN, siteB, YBAND, SPINDOWN)*gl(siteA, YBAND, SPINUP, siteB, YBAND, SPINUP)
                    );

                pairMinusCpx += DataType(-4.0) * (
                    gl(siteA, XBAND, SPINDOWN, siteB, XBAND, SPINUP)*gl(siteA, XBAND, SPINUP, siteB, XBAND, SPINDOWN) -
                    gl(siteA, XBAND, SPINDOWN, siteB, XBAND, SPINDOWN)*gl(siteA, XBAND, SPINUP, siteB, XBAND, SPINUP) -
                    gl(siteA, XBAND, SPINDOWN, siteB, YBAND, SPINUP)*gl(siteA, XBAND, SPINUP, siteB, YBAND, SPINDOWN) +
                    gl(siteA, XBAND, SPINDOWN, siteB, YBAND, SPINDOWN)*gl(siteA, XBAND, SPINUP, siteB, YBAND, SPINUP) -
                    gl(siteA, YBAND, SPINDOWN, siteB, XBAND, SPINUP)*gl(siteA, YBAND, SPINUP, siteB, XBAND, SPINDOWN) +
                    gl(siteA, YBAND, SPINDOWN, siteB, XBAND, SPINDOWN)*gl(siteA, YBAND, SPINUP, siteB, XBAND, SPINUP) +
                    gl(siteA, YBAND, SPINDOWN, siteB, YBAND, SPINUP)*gl(siteA, YBAND, SPINUP, siteB, YBAND, SPINDOWN) -
                    gl(siteA, YBAND, SPINDOWN, siteB, YBAND, SPINDOWN)*gl(siteA, YBAND, SPINUP, siteB, YBAND, SPINUP)
                    );
            }

            pairPlus[i] += std::real(pairPlusCpx);
            //pairPlusimag[i] += std::imag(pairPlusCpx);
            pairMinus[i] += std::real(pairMinusCpx);
            //pairMinusimag[i] += std::imag(pairMinusCpx);
        }

        // Fermionic energy contribution
        // -----------------------------
        auto glij = [this, gl](uint32_t site1, uint32_t site2, Band band, Spin spin) -> DataType {
            return gl(site1, band, spin,
                      site2, band, spin);
        };
        const auto txhor = pars.txhor;
        const auto txver = pars.txver;
        const auto tyhor = pars.tyhor;
        const auto tyver = pars.tyver;
        // code using this commented out
        (void)glij;
        (void)txhor; (void)txver; (void)tyhor; (void)tyver;
        
        // for (uint32_t i = 0; i < N; ++i) {
        //     //TODO: write in a nicer fashion using hopping-array as used in the checkerboard branch
        //     Spin spins[] = {SPINUP, SPINDOWN};
        //     for (auto spin: spins) {
        //         DataType e = DataType(txhor) * glij(i, spaceNeigh(XPLUS, i), XBAND, spin)
        //             + DataType(txhor) * glij(i, spaceNeigh(XMINUS,i), XBAND, spin)
        //             + DataType(txver) * glij(i, spaceNeigh(YPLUS, i), XBAND, spin)
        //             + DataType(txver) * glij(i, spaceNeigh(YMINUS,i), XBAND, spin)
        //             + DataType(tyhor) * glij(i, spaceNeigh(XPLUS, i), YBAND, spin)
        //             + DataType(tyhor) * glij(i, spaceNeigh(XMINUS,i), YBAND, spin)
        //             + DataType(tyver) * glij(i, spaceNeigh(YPLUS, i), YBAND, spin)
        //             + DataType(tyver) * glij(i, spaceNeigh(YMINUS,i), YBAND, spin);
        //         fermionEkinetic += std::real(e);
        //         //fermionEkinetic_imag += std::imag(e);
        //     }
        // }
        // for (uint32_t i = 0; i < N; ++i) {
        //     auto glbs = [this, i, gl](Band band1, Spin spin1,
        //                               Band band2, Spin spin2) -> DataType {
        //         return gl(i, band1, spin1, i, band2, spin2);
        //     };

        //     //factors for different combinations of spins
        //     //overall factor of -1 included
        //     // up_up, up_dn, dn_up, dn_dn;
        //     DataType up_up(0);
        //     DataType up_dn = DataType(-phi(i,0,timeslice)); // real part
        //     DataType dn_up = DataType(-phi(i,0,timeslice));
        //     DataType dn_dn(0);
        //     if (OPDIM >= 2) {
        //         setImag(up_dn, +phi(i,1,timeslice));
        //         setImag(dn_up, -phi(i,1,timeslice));
        //     }
        //     if (OPDIM == 3) {
        //         up_up = DataType(-phi(i,2,timeslice));
        //         dn_dn = DataType(+phi(i,2,timeslice));
        //     }

        //     DataType e = up_up * (glbs(XBAND, SPINUP, YBAND, SPINUP) +
        //                           glbs(YBAND, SPINUP, XBAND, SPINUP))
        //         + up_dn * (glbs(XBAND, SPINUP, YBAND, SPINDOWN) +
        //                    glbs(YBAND, SPINUP, XBAND, SPINDOWN))
        //         + dn_up * (glbs(XBAND, SPINDOWN, YBAND, SPINUP) +
        //                    glbs(YBAND, SPINDOWN, XBAND, SPINUP))
        //         + dn_dn * (glbs(XBAND, SPINDOWN, YBAND, SPINDOWN) +
        //                    glbs(YBAND, SPINDOWN, XBAND, SPINDOWN));

        //     fermionEcouple += std::real(e);
        //     //fermionEcouple_imag += std::imag(e);
        // }

        // band occupation / charge correlations
        // -------------------------------------
        // code generated in Mathematica: sdw-cdw-corr-obs.nb
        // for (uint32_t i = 0; i < N; ++i) {
        //     for (uint32_t j = 0; j < N; ++j) {
        //         if (i != j) {
        //             //unequal sites, slightly generic
        //             const Band BandValues[2] = {XBAND, YBAND};
        //             for (Band b1 : BandValues) {
        //                 for (Band b2 : BandValues) {
        //                     DataType contrib = 4.0 -
        //                         gl(i, b1, SPINDOWN, j, b2, SPINDOWN)*
        //                         gl(j, b2, SPINDOWN, i, b1, SPINDOWN) -
        //                         gl(i, b1, SPINUP, j, b2, SPINDOWN)*
        //                         gl(j, b2, SPINDOWN, i, b1, SPINUP) - 2.0*
        //                         gl(j, b2, SPINDOWN, j, b2, SPINDOWN) -
        //                         gl(i, b1, SPINDOWN, j, b2, SPINUP)*
        //                         gl(j, b2, SPINUP, i, b1, SPINDOWN) -
        //                         gl(i, b1, SPINUP, j, b2, SPINUP)*
        //                         gl(j, b2, SPINUP, i, b1, SPINUP) - 2.0*
        //                         gl(j, b2, SPINUP, j, b2, SPINUP) +
        //                         gl(i, b1, SPINDOWN, i, b1, SPINDOWN)*
        //                         (-2.0 +
        //                          gl(j, b2, SPINDOWN, j, b2, SPINDOWN) +
        //                          gl(j, b2, SPINUP, j, b2, SPINUP)) +
        //                         gl(i, b1, SPINUP, i, b1, SPINUP)*
        //                         (-2.0 +
        //                          gl(j, b2, SPINDOWN, j, b2, SPINDOWN) +
        //                          gl(j, b2, SPINUP, j, b2, SPINUP));
        //                     occCorr(b1, b2)(i, j) += std::real(contrib);
        //                 }
        //             }
        //         } else {
        //             //equal site i, use band-specific code
        //             DataType contribxx = 4.0 - 2.0*
        //                 gl(i, XBAND, SPINDOWN, i, XBAND, SPINUP)*
        //                 gl(i, XBAND, SPINUP, i, XBAND, SPINDOWN) - 3.0*
        //                 gl(i, XBAND, SPINUP, i, XBAND, SPINUP) +
        //                 gl(i, XBAND, SPINDOWN, i, XBAND, SPINDOWN)*
        //                 (-3.0 + 2.0*
        //                  gl(i, XBAND, SPINUP, i, XBAND, SPINUP));
        //             occCorr(XBAND,XBAND)(i, i) += std::real(contribxx);

        //             DataType contribxy = 4.0 -
        //                 gl(i, XBAND, SPINDOWN, i, YBAND, SPINDOWN)*
        //                 gl(i, YBAND, SPINDOWN, i, XBAND, SPINDOWN) -
        //                 gl(i, XBAND, SPINUP, i, YBAND, SPINDOWN)*
        //                 gl(i, YBAND, SPINDOWN, i, XBAND, SPINUP) - 2.0*
        //                 gl(i, YBAND, SPINDOWN, i, YBAND, SPINDOWN) -
        //                 gl(i, XBAND, SPINDOWN, i, YBAND, SPINUP)*
        //                 gl(i, YBAND, SPINUP, i, XBAND, SPINDOWN) -
        //                 gl(i, XBAND, SPINUP, i, YBAND, SPINUP)*
        //                 gl(i, YBAND, SPINUP, i, XBAND, SPINUP) - 2.0*
        //                 gl(i, YBAND, SPINUP, i, YBAND, SPINUP) +
        //                 gl(i, XBAND, SPINDOWN, i, XBAND, SPINDOWN)*
        //                 (-2.0 +
        //                  gl(i, YBAND, SPINDOWN, i, YBAND, SPINDOWN) +
        //                  gl(i, YBAND, SPINUP, i, YBAND, SPINUP)) +
        //                 gl(i, XBAND, SPINUP, i, XBAND, SPINUP)*
        //                 (-2.0 +
        //                  gl(i, YBAND, SPINDOWN, i, YBAND, SPINDOWN) +
        //                  gl(i, YBAND, SPINUP, i, YBAND, SPINUP));
        //             occCorr(XBAND,YBAND)(i,i) += std::real(contribxy);
        //             occCorr(YBAND,XBAND)(i,i) += std::real(contribxy); // it's symmetric in xy

        //             DataType contribyy = 4.0 - 2.0*
        //                 gl(i, YBAND, SPINDOWN, i, YBAND, SPINUP)*
        //                 gl(i, YBAND, SPINUP, i, YBAND, SPINDOWN) - 3.0*
        //                 gl(i, YBAND, SPINUP, i, YBAND, SPINUP) +
        //                 gl(i, YBAND, SPINDOWN, i, YBAND, SPINDOWN)*
        //                 (-3.0 + 2.0*
        //                  gl(i, YBAND, SPINUP, i, YBAND, SPINUP));
        //             occCorr(YBAND,YBAND)(i,i) += std::real(contribyy);
        //         }
        //     }
        // }

        DataType occDiffSqContrib = 0.0;
        for (uint32_t i = 0; i < N; ++i) {
            occDiffSqContrib += -2.0*
                gl(i, XBAND, SPINDOWN, i, XBAND, SPINUP)*
                gl(i, XBAND, SPINUP, i, XBAND, SPINDOWN) +
                gl(i, XBAND, SPINUP, i, XBAND, SPINUP) + 2.0*
                gl(i, XBAND, SPINDOWN, i, YBAND, SPINDOWN)*
                gl(i, YBAND, SPINDOWN, i, XBAND, SPINDOWN) + 2.0*
                gl(i, XBAND, SPINUP, i, YBAND, SPINDOWN)*
                gl(i, YBAND, SPINDOWN, i, XBAND, SPINUP) +
                gl(i, YBAND, SPINDOWN, i, YBAND, SPINDOWN) - 2.0*
                gl(i, XBAND, SPINUP, i, XBAND, SPINUP)*
                gl(i, YBAND, SPINDOWN, i, YBAND, SPINDOWN) + 2.0*
                gl(i, XBAND, SPINDOWN, i, YBAND, SPINUP)*
                gl(i, YBAND, SPINUP, i, XBAND, SPINDOWN) + 2.0*
                gl(i, XBAND, SPINUP, i, YBAND, SPINUP)*
                gl(i, YBAND, SPINUP, i, XBAND, SPINUP) - 2.0*
                gl(i, YBAND, SPINDOWN, i, YBAND, SPINUP)*
                gl(i, YBAND, SPINUP, i, YBAND, SPINDOWN) +
                gl(i, XBAND, SPINDOWN, i, XBAND, SPINDOWN)*
                (1.0 + 2.0*
                 gl(i, XBAND, SPINUP, i, XBAND, SPINUP) - 2.0*
                 gl(i, YBAND, SPINDOWN, i, YBAND, SPINDOWN) - 2.0*
                 gl(i, YBAND, SPINUP, i, YBAND, SPINUP)) +
                gl(i, YBAND, SPINUP, i, YBAND, SPINUP) - 2.0*
                gl(i, XBAND, SPINUP, i, XBAND, SPINUP)*
                gl(i, YBAND, SPINUP, i, YBAND, SPINUP) + 2.0*
                gl(i, YBAND, SPINDOWN, i, YBAND, SPINDOWN)*
                gl(i, YBAND, SPINUP, i, YBAND, SPINUP);
        }
        occDiffSq += (std::real(occDiffSqContrib)) / num(N);
    }

    timing.stop("sdw-measure");
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::finishMeasurements() {
    //to ease notation:
    const auto L = pars.L;
    const auto N = pars.N;
    const auto m = pars.m;
    const auto dtau = pars.dtau;

    assert(timeslices_included_in_measurement.size() == m);

    //normphi, meanPhi, sdw-susceptibility
    meanPhi /= num(N * m);
    normMeanPhi = arma::norm(meanPhi, 2);

    // bosonic spin stiffness
    if (OPDIM == 2) {
        phiRhoS_Gc *= (0.5 * dtau);
        phiRhoS_Gs *= dtau;
    }

    associatedEnergy /= (2.0 * N * m);

    if (not (pars.turnoffFermions or pars.turnoffFermionMeasurements)) {    

        if (pars.dumpGreensFunction) {

            // some sectors of the momentum space Green's function
            greenXUPXUP_summed /= num(m);
            greenYDOWNYDOWN_summed /= num(m);
            // computeStructureFactor(kgreenXUP, greenXUPXUP_summed);
            // computeStructureFactor(kgreenYDOWN, greenYDOWNYDOWN_summed);
            if (OPDIM == 3) {
                greenXDOWNXDOWN_summed /= num(m);
                greenYUPYUP_summed   /= num(m);
                // computeStructureFactor(kgreenXDOWN, greenXDOWNXDOWN_summed);
                // computeStructureFactor(kgreenYUP, greenYUPYUP_summed);
            } else {
                // the following equalities are up to complex conjugation,
                // but we only consider real parts anyway
                // kgreenXDOWN = kgreenXUP;
                // kgreenYUP = kgreenYDOWN;
            }
            greenXUPYDOWN_summed /= num(m);
            greenYDOWNXUP_summed /= num(m);

            // HACK - save current real-space Green's function
            debugSaveMatrixCpx(greenXUPXUP_summed,     "green_eqtime_realspace_XUPXUP_" + numToString(performedSweeps+1));
            debugSaveMatrixCpx(greenXUPYDOWN_summed,   "green_eqtime_realspace_XUPYDOWN_" + numToString(performedSweeps+1));        
            debugSaveMatrixCpx(greenYDOWNXUP_summed,   "green_eqtime_realspace_YDOWNXUP_" + numToString(performedSweeps+1));
            debugSaveMatrixCpx(greenYDOWNYDOWN_summed, "green_eqtime_realspace_YDOWNYDOWN_" + numToString(performedSweeps+1));        
        }
            
        // scalar functions of the Green's function
        greenK0 /= num(m);
        greenLocal /= num(m);

        // //fermion occupation number -- real space
        // occX /= num(m * N);
        // occY /= num(m * N);

        //fermion occupation number -- k-space
        for (uint32_t ksite = 0; ksite < N; ++ksite) {
            // add 2.0 and not 1.0 because spin is included
            kOccX[ksite] = 2.0 - kOccX[ksite] / num(m * N);
            kOccY[ksite] = 2.0 - kOccY[ksite] / num(m * N);
        }

        //equal-time pairing-correlations
        //-------------------------------
        pairPlus /= m;
        pairMinus /= m;
        // sites around the maximum range L/2, L/2
        static const uint32_t numSitesFar = 9;
        uint32_t sitesfar[numSitesFar] = {
            coordsToSite(L/2 - 1, L/2 - 1), coordsToSite(L/2, L/2 - 1), coordsToSite(L/2 + 1, L/2 - 1),
            coordsToSite(L/2 - 1, L/2),     coordsToSite(L/2, L/2),     coordsToSite(L/2 + 1, L/2),
            coordsToSite(L/2 - 1, L/2 + 1), coordsToSite(L/2, L/2 + 1), coordsToSite(L/2 + 1, L/2 + 1)
        };
        pairPlusMax = 0;
        pairMinusMax = 0;
        for (uint32_t i : sitesfar) {
            pairPlusMax += pairPlus[i];
            pairMinusMax += pairMinus[i];
        }
        pairPlusMax /= numSitesFar;
        pairMinusMax /= numSitesFar;

        // // Fermionic energy contribution
        // // -----------------------------
        // fermionEkinetic /= num(m*N);
        // fermionEcouple /= num(m*N);


        // // band occupation / charge correlations
        // // -------------------------------------
        // const Band BandValues[2] = {XBAND, YBAND};
        // for (Band b1 : BandValues) {
        //     for (Band b2 : BandValues) {
        //         occCorr(b1,b2) /= num(m);
        //     }
        // }
        // chargeCorr = occCorr(XBAND,XBAND) + occCorr(XBAND,YBAND) +
        //     occCorr(YBAND,XBAND) + occCorr(YBAND,YBAND);

        // // Fourier transforms
        // for (Band b1 : BandValues) {
        //     for (Band b2 : BandValues) {
        //         computeStructureFactor(occCorrFT(b1,b2), occCorr(b1,b2));
        //     }
        // }
        // computeStructureFactor(chargeCorrFT, chargeCorr);

        occDiffSq /= num(m);

    }
}


// This assumes that in_r is for translationally invariant data, i.e.
// obeys periodic boundary conditions.  Note: Even if we have
// anti-periodic boundary conditions for the fermion operators,
// e.g. c_x = -c_{x+L} [1d for ease of notation], the real space density
// is periodic: n_x = c^+_x c_x = c^_{x+L} c_{x+L} and likewise for the spin
// --> no offsetting of k-space vectors!
template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::computeStructureFactor(VecNum& out_k, const MatNum& in_r) {
    static const num pi = M_PI;
    const auto L = pars.L;
    const auto N = pars.N;
    // //offset k-components for antiperiodic bc
    // num offset_x = 0.0;
    // num offset_y = 0.0;
    // if (pars.bc == BC_Type::APBC_X or pars.bc == BC_Type::APBC_XY) {
    //     offset_x = 0.5;
    // }
    // if (pars.bc == BC_Type::APBC_Y or pars.bc == BC_Type::APBC_XY) {
    //     offset_y = 0.5;
    // }
    out_k.zeros(N);
    for (uint32_t ksite = 0; ksite < N; ++ksite) {
        uint32_t ksitey = ksite / L;
        uint32_t ksitex = ksite % L;
        // num ky = -pi + (num(ksitey) + offset_y) * 2*pi / num(L);
        // num kx = -pi + (num(ksitex) + offset_x) * 2*pi / num(L);
        num ky = -pi + num(ksitey) * 2*pi / num(L);
        num kx = -pi + num(ksitex) * 2*pi / num(L);
        for (uint32_t i = 0; i < N; ++i) {
            num iy = num(i / L);
            num ix = num(i % L);
            for (uint32_t j = 0; j  < N; ++j) {
                num jy = num(j / L);
                num jx = num(j % L);

                num argument = kx * (ix - jx) + ky * (iy - jy);
                cpx phase = std::exp(cpx(0, argument));

                cpx contrib = in_r(i,j) * phase;
                out_k(ksite) += contrib.real();
            }
        }
    }
    out_k /= num(N);
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::computeStructureFactor(VecNum& out_k, const MatCpx& in_r) {
    static const num pi = M_PI;
    const auto L = pars.L;
    const auto N = pars.N;
    out_k.zeros(N);
    for (uint32_t ksite = 0; ksite < N; ++ksite) {
        uint32_t ksitey = ksite / L;
        uint32_t ksitex = ksite % L;
        // num ky = -pi + (num(ksitey) + offset_y) * 2*pi / num(L);
        // num kx = -pi + (num(ksitex) + offset_x) * 2*pi / num(L);
        num ky = -pi + num(ksitey) * 2*pi / num(L);
        num kx = -pi + num(ksitex) * 2*pi / num(L);
        cpx k_contrib = cpx(0);
        for (uint32_t i = 0; i < N; ++i) {
            num iy = num(i / L);
            num ix = num(i % L);
            for (uint32_t j = 0; j  < N; ++j) {
                num jy = num(j / L);
                num jx = num(j % L);

                num argument = kx * (ix - jx) + ky * (iy - jy);
                cpx phase = std::exp(cpx(0, argument));

                k_contrib += in_r(i,j) * phase;
            }
        }
        out_k(ksite) = k_contrib.real();
    }
    out_k /= num(N);
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::setupRandomField() {
    for (uint32_t k = 1; k <= pars.m; ++k) {
        for (uint32_t site = 0; site < pars.N; ++site) {
            for (uint32_t dim = 0; dim < OPDIM; ++dim) {
                phi(site, dim, k) = rng.randRange(PhiLow, PhiHigh);
            }
            num r = rng.rand01();
            if      (r <= 0.25) cdwl(site, k) = +2;
            else if (r <= 0.5)	cdwl(site, k) = -2;
            else if (r <= 0.75)	cdwl(site, k) = +1;
            else                cdwl(site, k) = -1;
            updateCoshSinhTerms(site, k);
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::setupConstantField() {
    for (uint32_t k = 1; k <= pars.m; ++k) {
        for (uint32_t site = 0; site < pars.N; ++site) {
            phi(site, 0, k) = 1.0;
            for (uint32_t dim = 1; dim < OPDIM; ++dim) {
                phi(site, dim, k) = 0;
            }
            cdwl(site, k) = +1;
            updateCoshSinhTerms(site, k);
        }
    }
}



template<CheckerboardMethod CB, int OPDIM>
std::tuple<num,num> DetSDW<CB, OPDIM>::getCoshSinhTermPhi(Phi phi) {
    num phiNorm = arma::norm(phi, 2);
    return std::make_tuple(std::cosh(pars.lambda * pars.dtau * phiNorm),
                           std::sinh(pars.lambda * pars.dtau * phiNorm) / phiNorm);
}

template<CheckerboardMethod CB, int OPDIM>
std::tuple<num,num> DetSDW<CB, OPDIM>::getCoshSinhTermCDWl(
    int32_t cdwl) {
    num arg = std::sqrt(pars.dtau) * pars.cdwU * cdwl_eta(cdwl);
    return std::make_tuple(std::cosh(arg), std::sinh(arg));
}



template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateCoshSinhTerms(uint32_t site, uint32_t k) {
    updateCoshSinhTermsPhi(site, k);
    if (pars.cdwU) updateCoshSinhTermsCDWl(site, k);
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateCoshSinhTermsPhi(uint32_t site, uint32_t k) {
    std::tie(coshTermPhi(site,k), sinhTermPhi(site,k)) =
        getCoshSinhTermPhi(getPhi(site,k));
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateCoshSinhTermsCDWl(uint32_t site, uint32_t k) {
    std::tie(coshTermCDWl(site,k), sinhTermCDWl(site,k)) =
        getCoshSinhTermCDWl(cdwl(site, k));
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateCoshSinhTerms() {
    for (uint32_t k = 1; k <= pars.m; ++k) {
        for (uint32_t site = 0; site < pars.N; ++site) {
            updateCoshSinhTerms(site,k);
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateCoshSinhTermsPhi() {
    for (uint32_t k = 1; k <= pars.m; ++k) {
        for (uint32_t site = 0; site < pars.N; ++site) {
            updateCoshSinhTermsPhi(site,k);
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateCoshSinhTermsCDWl() {
    for (uint32_t k = 1; k <= pars.m; ++k) {
        for (uint32_t site = 0; site < pars.N; ++site) {
            updateCoshSinhTermsCDWl(site,k);
        }
    }
}


// compute e^(-dtau*K^{\alpha}..) matrices by diagonalization
// (\alpha = x, y):
//
//    K^{\alpha}[i, j] = - t^{\alpha}_ij * e^{i A^{\alpha}_ij}
//
//  default: t^{x}_{i, i \pm \hat{x}} = -1     [x hor]
//           t^{x}_{i, i \pm \hat{y}} = -0.5   [x ver]
//           t^{y}_{i, i \pm \hat{x}} = 0.5    [x hor]
//           t^{y}_{i, i \pm \hat{y}} = 1      [x ver]
//
//  for OPDIM==2 we also support a z-parallel magnetic field with
//  vector potential A != 0.  For OPDIM==3 this needs to be
//  generalized: we want one field for XUP and YDOWN, but another one
//  (the inverse) for YUP and XDOWN to remain free of sign-problem.
//
//  chemical potential term: - \mu \delta_{ij} not included here
template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::setupPropK() {
    const num pi = M_PI;
    const uint32_t dim = 2;
    const uint32_t z = 2*dim;

    checkarray<checkarray<num,z>, 2> t;
    t[XBAND][XPLUS] = t[XBAND][XMINUS] = hopHor[XBAND];
    t[XBAND][YPLUS] = t[XBAND][YMINUS] = hopVer[XBAND];
    t[YBAND][XPLUS] = t[YBAND][XMINUS] = hopHor[YBAND];
    t[YBAND][YPLUS] = t[YBAND][YMINUS] = hopVer[YBAND];

    checkarray<num, 2> mu;
    mu[XBAND] = pars.mux;
    mu[YBAND] = pars.muy;
    
//  for (auto band : {XBAND, YBAND}) {
    Band bands[2] = {XBAND, YBAND};
    for (Band band : bands) {
        MatCpx k = -mu[band] * arma::eye<MatCpx>(pars.N,pars.N);

        num zmag_here = 0.0;
        if      (band == XBAND) zmag_here = zmag[XUP];
        else if (band == YBAND) zmag_here = zmag[YDOWN];
        
        for (uint32_t site = 0; site < pars.N; ++site) {
            for (uint32_t dir = 0; dir < z; ++dir) {
                uint32_t neigh = spaceNeigh(dir, site);
                num hop = t[band][dir];

                uint32_t siteY = site / pars.L;
                uint32_t siteX = site % pars.L;
                if (pars.bc == BC_Type::APBC_X or pars.bc == BC_Type::APBC_XY) {
                    if ((siteX == 0 and dir == XMINUS) or (siteX == pars.L-1 and dir == XPLUS)) {
                        //crossing x-boundary
                        hop *= -1;
                    }
                }
                if (pars.bc == BC_Type::APBC_Y or pars.bc == BC_Type::APBC_XY) {
                    if ((siteY == 0 and dir == YMINUS) or (siteY == pars.L-1 and dir == YPLUS)) {
                        //crossing y-boundary
                        hop *= -1;
                    }
                }

                // add magnetic field:
                cpx phase(1.0, 0.0);
                // horizontal bonds
                if (dir == XPLUS) {
                    num imag_argument = -2.0 * pi * zmag_here * siteY;
                    phase = std::exp(cpx(0.0, imag_argument));
                }
                if (dir == XMINUS) {
                    num imag_argument = +2.0 * pi * zmag_here * siteY;
                    phase = std::exp(cpx(0.0, imag_argument));
                }
                // vertical bonds -- only those crossing the lattice boundary
                if (dir == YPLUS and siteY == pars.L-1) {
                    num imag_argument = +2.0 * pi * zmag_here * pars.L * siteX;
                    phase = std::exp(cpx(0.0, imag_argument));
                }
                if (dir == YMINUS and siteY == 0) {
                    num imag_argument = -2.0 * pi * zmag_here * pars.L * siteX;
                    phase = std::exp(cpx(0.0, imag_argument));
                }

                k(site, neigh) -= hop * phase;
            }
        }
        //debugSaveMatrix(k, "k" + bandstr(band));

        propK[band] = computePropagator(pars.dtau, k);

        propK_half[band] = computePropagator(pars.dtau / 2.0, k);
        propK_half_inv[band] = computePropagator(-pars.dtau / 2.0, k);
    }
}


template<CheckerboardMethod CB, int OPDIM>
template<class Vec>
VecNum DetSDW<CB, OPDIM>::compute_d_for_cdwl(const Vec& cdwl) {
    VecNum kd(pars.N);
    for (uint32_t i = 0; i < pars.N; ++ i) {
        kd[i] = compute_d_for_cdwl_site(cdwl[i]);
    }
    return kd;
}


template<CheckerboardMethod CB, int OPDIM> inline
num DetSDW<CB, OPDIM>::compute_d_for_cdwl_site(int32_t cdwl) {
    //TODO: check assembly -- operations removed?
    return std::sqrt(pars.dtau) * pars.cdwU * cdwl_eta(cdwl);
}



template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::computeBmatSDW(uint32_t k2, uint32_t k1) {
    // this routine is never called in performance-sensitive code, its speed does not matter 
    
    //if (CB == CB_NONE) {
    {
        const auto N = pars.N;
        timing.start("computeBmatSDW_direct");
        using arma::eye; using arma::zeros; using arma::diagmat;
        if (k2 == k1) {
            return arma::eye<MatData>(MatrixSizeFactor*N, MatrixSizeFactor*N);
        }
        assert(k2 > k1);
        assert(k2 <= m);

        //compute the matrix e^(-dtau*V_k) * e^(-dtau*K)
        auto singleTimesliceProp = [this, N](uint32_t k) -> MatData {
            timing.start("singleTimesliceProp_direct");
            MatData result(MatrixSizeFactor*N, MatrixSizeFactor*N);

            //submatrix view helper for a 2Nx2n | 4Nx4N matrix
            auto block = [&result, N](uint32_t row, uint32_t col) {
                return result.submat(row * N, col * N,
                                     (row + 1) * N - 1, (col + 1) * N - 1);
            };
            const auto& kphi0 = phi.slice(k).col(0);
            const auto& kphi1 = (OPDIM >  1 ? phi.slice(k).col(1) : kphi0);
            const auto& kphi2 = (OPDIM == 3 ? phi.slice(k).col(2) : kphi0);
            //      Debugsavematrix(kphi0, "kphi0");
            //      debugSaveMatrix(kphi1, "kphi1");
            //      debugSaveMatrix(kphi2, "kphi2");
            const auto& kcoshTermPhi = coshTermPhi.col(k);
            const auto& ksinhTermPhi = sinhTermPhi.col(k);
            if (not pars.cdwU) { // to be safe
                coshTermCDWl.col(k).ones();
                sinhTermCDWl.col(k).zeros();
            }
            const auto& kcoshTermCDWl = coshTermCDWl.col(k);
            const auto& ksinhTermCDWl = sinhTermCDWl.col(k);

            auto approxZero = [](num var) -> bool {
                return std::abs(var) <= 1E-10;
            };
            
            // A) case without magnetic field
            //    -> propKx, propKy are purely real
            if (approxZero(zmag[XUP]) and approxZero(zmag[YDOWN]) and
                approxZero(zmag[XDOWN]) and approxZero(zmag[YUP])) {
            
                //upper left 2*2 blocks
                typedef DetSDW<CB,OPDIM> D;
                // fully real parts
                D::setRealImag(block(0, 0),
                               diagmat(kcoshTermPhi % kcoshTermCDWl + ksinhTermCDWl) * arma::real(propKx),
                               zeros(N, N));
                D::setRealImag(block(1, 1),
                               diagmat(kcoshTermPhi % kcoshTermCDWl - ksinhTermCDWl) * arma::real(propKy),
                               zeros(N,N));
                // O(1) model: only real part
                if (OPDIM == 1) {
                    D::setRealImag(block(0, 1),
                                   diagmat(-kphi0 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKy),
                                   zeros(N,N));
                    D::setRealImag(block(1, 0),
                                   diagmat(-kphi0 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKx),
                                   zeros(N,N));
                }
                else {
                    D::setRealImag(block(0, 1),
                                   diagmat(-kphi0 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKy),
                                   diagmat(+kphi1 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKy));
                    D::setRealImag(block(1, 0),
                                   diagmat(-kphi0 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKx),
                                   diagmat(-kphi1 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKx));
                }

                if (OPDIM == 3) {
                    //lower right 2*2 blocks
                    block(2, 2) = block(0, 0);
                    D::setRealImag(block(2, 3),
                                   diagmat(-kphi0 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKy),
                                   diagmat(-kphi1 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKy));
                    D::setRealImag(block(3, 2),
                                   diagmat(-kphi0 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKx),
                                   diagmat(+kphi1 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKx));
                    block(3, 3) = block(1, 1);

                    //anti-diagonal blocks
                    D::setRealImag(block(0, 3),
                                   diagmat(-kphi2 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKy),
                                   zeros(N,N));
                    D::setRealImag(block(1, 2),
                                   diagmat(+kphi2 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKx),
                                   zeros(N,N));
                    D::setRealImag(block(2, 1),
                                   diagmat(+kphi2 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKy),
                                   zeros(N,N));
                    D::setRealImag(block(3, 0),
                                   diagmat(-kphi2 % ksinhTermPhi % kcoshTermCDWl) * arma::real(propKx),
                                   zeros(N,N));

                    //zero blocks
                    block(0, 2).zeros();
                    block(1, 3).zeros();
                    block(2, 0).zeros();
                    block(3, 1).zeros();
                }
            }
            // B) case with magnetic field
            //    -> need to consider complex propKx, propKy 
            else {

                //upper left 2*2 blocks
                typedef DetSDW<CB,OPDIM> D;
                block(0, 0) = diagmat(kcoshTermPhi % kcoshTermCDWl + ksinhTermCDWl) * propKx;
                block(1, 1) = diagmat(kcoshTermPhi % kcoshTermCDWl - ksinhTermCDWl) * propKy;
                
                if (OPDIM == 1) {
                    block(0,1) = diagmat(VecCpx(-kphi0 % ksinhTermPhi % kcoshTermCDWl,
                                                zeros(N))) * propKy;
                    block(1,0) = diagmat(VecCpx(-kphi0 % ksinhTermPhi % kcoshTermCDWl,
                                                zeros(N))) * propKx;
                } else {
                    block(0,1) = diagmat(VecCpx(-kphi0 % ksinhTermPhi % kcoshTermCDWl,
                                                +kphi1 % ksinhTermPhi % kcoshTermCDWl)) * propKy;
                    block(1,0) = diagmat(VecCpx(-kphi0 % ksinhTermPhi % kcoshTermCDWl,
                                                -kphi1 % ksinhTermPhi % kcoshTermCDWl)) * propKx;
                }
                if (OPDIM == 3) {
                    //lower right 2*2 blocks

                    //  TODO:  These need to be adapted for the case with magnetic field
                    //         -> should amount to using the Hermitian conjugate of propK{x,y}
                    
                    block(2, 2) = block(0, 0);

                    block(2, 3) = diagmat(VecCpx(diagmat(-kphi0 % ksinhTermPhi % kcoshTermCDWl),
                                                 diagmat(-kphi1 % ksinhTermPhi % kcoshTermCDWl))) * propKy;
                    block(3, 2) = diagmat(VecCpx(diagmat(-kphi0 % ksinhTermPhi % kcoshTermCDWl),
                                                 diagmat(+kphi1 % ksinhTermPhi % kcoshTermCDWl))) * propKx;
                    block(3, 3) = block(1, 1);

                    //anti-diagonal blocks
                    block(0, 3) = diagmat(VecCpx(-kphi2 % ksinhTermPhi % kcoshTermCDWl,
                                                 zeros(N))) * propKy;
                    block(1, 2) = diagmat(VecCpx(+kphi2 % ksinhTermPhi % kcoshTermCDWl,
                                                 zeros(N))) * propKx;
                    block(2, 1) = diagmat(VecCpx(+kphi2 % ksinhTermPhi % kcoshTermCDWl,
                                                 zeros(N))) * propKy;
                    block(3, 0) = diagmat(VecCpx(-kphi2 % ksinhTermPhi % kcoshTermCDWl,
                                                 zeros(N))) * propKx;

                    //zero blocks
                    block(0, 2).zeros();
                    block(1, 3).zeros();
                    block(2, 0).zeros();
                    block(3, 1).zeros();
                }
                
                
            }

            //      debugSaveMatrix(arma::real(result), "emdtauVemdtauK_real");
            //      debugSaveMatrix(arma::imag(result), "emdtauVemdtauK_imag");
            timing.stop("singleTimesliceProp_direct");
            return result;
        };

        MatData result = singleTimesliceProp(k2);

        for (uint32_t k = k2 - 1; k > k1; --k) {
            result *= singleTimesliceProp(k);               // equivalent to: result = result * singleTimesliceProp(k);
        }

        timing.stop("computeBmatSDW_direct");

        return result;
    }
//    else {
//        // use the checkerboard routines to compute B by left-multiplying to unity
//        using arma::eye; using arma::zeros;
//        if (k2 == k1) {
//            return MatCpx(eye(4*N,4*N), zeros(4*N,4*N));
//        }
//        assert(k2 > k1);
//        assert(k2 <= m);
//        MatCpx unity(eye(4*N, 4*N), zeros(4*N, 4*N));
//        return checkerboardLeftMultiplyBmat(unity, k2, k1);
//    }
}

template<CheckerboardMethod CB, int OPDIM> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::computePotentialExponential(
    int sign, checkarray<VecNum, OPDIM> phi, VecInt cdwl) {
    const auto N = pars.N;

    //note: Armadillo functions set_real and set_imag are also defined
    //for purely real matrices (where set_imag just does nothing)

    VecData a;
    if (OPDIM == 3) {
        a.set_size(N);
        a.set_real(phi[2]);
        a.set_imag(arma::zeros<VecNum>(N));
    }

    VecData b;
    b.set_size(N);
    b.set_real(phi[0]);
    if (OPDIM == 2 or OPDIM == 3) {
        b.set_imag(-phi[1]);
    } // else b is real

    VecData bc;
    bc.set_size(N);
    bc.set_real(phi[0]);
    if (OPDIM == 2 or OPDIM == 3) {
        bc.set_imag( phi[1]);
    } // else bc is real (and equal to b)

    VecData d;
    d.set_size(N);
    d.set_real(compute_d_for_cdwl(cdwl));
    if (OPDIM == 2 or OPDIM == 3) {
        d.set_imag(arma::zeros<VecNum>(N));
    } // else d is of (real) type VecNum anyway

#define block(mat,row,col) mat.submat( (row) * N, (col) * N, ((row) + 1) * N - 1, ((col) + 1) * N - 1)
    MatData V(MatrixSizeFactor*N, MatrixSizeFactor*N);
    V.zeros(MatrixSizeFactor*N, MatrixSizeFactor*N);

    //OPDIM == 1 or OPDIM == 2: just two non-zero blocks of a 2Nx2N matrix
    block(V,0,1).diag() = b;
    block(V,1,0).diag() = bc;

    //OPDIM == 3: 4Nx4N matrix: some additional and some repeated blocks
    if (OPDIM == 3) {
        block(V,2,3).diag() = bc;
        block(V,3,2).diag() =  b;
        block(V,0,3).diag() =  a;
        block(V,1,2).diag() = -a;
        block(V,2,1).diag() = -a;
        block(V,3,0).diag() =  a;
    }

    // prefactor: coupling constant
    V *= pars.lambda;

    MatData D(MatrixSizeFactor*N, MatrixSizeFactor*N);
    D.zeros(MatrixSizeFactor*N, MatrixSizeFactor*N);
    //OPDIM == 1 or OPDIM == 2: just two non-zero blocks of a 2Nx2N matrix
    block(D,0,0).diag()	=  d;
    block(D,1,1).diag()	= -d;
    //OPDIM == 3: 4Nx4N matrix: repeated blocks
    if (OPDIM == 3) {
        block(D,2,2).diag() =  d;
        block(D,3,3).diag() = -d;
    }
#undef block

    VecNum eigval;
    MatData eigvec;

    arma::eig_sym(eigval, eigvec, num(sign)*0.5*pars.dtau*V);
    MatData exp_vphi_half(MatrixSizeFactor*N, MatrixSizeFactor*N);
    exp_vphi_half = eigvec * arma::diagmat(arma::exp(eigval)) * arma::trans(eigvec);

    arma::eig_sym(eigval, eigvec, -num(sign)*D);
    MatData exp_D(MatrixSizeFactor*N, MatrixSizeFactor*N);
    exp_D = eigvec * arma::diagmat(arma::exp(eigval)) * arma::trans(eigvec);

    MatData result = exp_vphi_half * exp_D * exp_vphi_half;

    return result;
}



//subgroup == 0: plaquettes A = [i j k l] = bonds (<ij>,<ik>,<kl>,<jl>)
//   i = (2m, 2n), with m,n integer, 2m < L, 2n < L
//   j = i + XPLUS
//   k = i + YPLUS
//   l = k + XPLUS
//subgroup == 1: plaquettes B = [i j k l] = bonds (<ij>,<ik>,<kl>,<jl>)
//   i = (2m+1, 2n+1), with m,n integer, 2m+1 < L, 2n+1 < L
//   j = i + XPLUS
//   k = i + YPLUS
//   l = k + XPLUS
template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::precalc_4site_hopping_exponentials() {
    const num pi = M_PI;
    const auto L = pars.L;

    num prefactors[4] = {
        -pars.dtau,        -0.5*pars.dtau,        +pars.dtau,       +0.5*pars.dtau
    };
    typedef std::reference_wrapper<checkarray<ExpHop4SiteStorage, 2> > StorageRef;
    StorageRef storage_collections[4] = {
        // expHop4Site_minus, expHop4Site_minusHalf, expHop4Site_plus, expHop4Site_plusHalf   // fails on Intel compiler
        std::ref(expHop4Site_minus), std::ref(expHop4Site_minusHalf), std::ref(expHop4Site_plus), std::ref(expHop4Site_plusHalf)
    };

    for (uint32_t prefactor_index = 0; prefactor_index < 4; ++prefactor_index) {
        num prefactor = prefactors[prefactor_index];
        checkarray<ExpHop4SiteStorage, 2>& storage = storage_collections[prefactor_index];
        
        Band bands[2] = {XBAND, YBAND};
        for (Band band : bands) {
            // TODO: this needs to be extended to properly deal with the O(3) case
            num zmag_here = 0.0;
            if      (band == XBAND) zmag_here = zmag[XUP];
            else if (band == YBAND) zmag_here = zmag[YDOWN];

            uint32_t subgroups[2] = {0, 1};
            for (uint32_t subgroup : subgroups) {
                storage[band][subgroup] = std::map<uint32_t, Mat4Site>();
                for (uint32_t i1 = subgroup; i1 < L; i1 += 2) {
                    for (uint32_t i2 = subgroup; i2 < L; i2 += 2) {
                        uint32_t i = this->coordsToSite(i1, i2);
                        uint32_t j = spaceNeigh(XPLUS, i);
                        uint32_t k = spaceNeigh(YPLUS, i);
                        // uint32_t l = spaceNeigh(XPLUS, k);

                        uint32_t j1 = j % L;
                        uint32_t k2 = k / L;

                        num hh = hopHor[band];
                        num hv = hopVer[band];
                    
                        if ((pars.bc == BC_Type::APBC_X or pars.bc == BC_Type::APBC_XY) and (i1 == L-1)) {
                            //this plaquette has horizontal boundary crossing bonds and APBC
                            hh *= -1;
                        }
                        if ((pars.bc == BC_Type::APBC_Y or pars.bc == BC_Type::APBC_XY) and i2 == L-1) {
                            //this plaquette has vertical boundary crossing bonds and APBC
                            hv *= -1;
                        }

                        // phase factors due to magnetic field
                        cpx ph_ij = std::exp(cpx(0.0, -2.0 * pi * zmag_here * i2)); // horizontal 1
                        cpx ph_kl = std::exp(cpx(0.0, -2.0 * pi * zmag_here * k2)); // horizontal 2
                        // vertical bonds pick up phase only if lattice boundary is crossed
                        cpx ph_ik = cpx(1.0, 0.0);
                        cpx ph_jl = cpx(1.0, 0.0);
                        if (i2 == L-1) {
                            ph_ik = std::exp(cpx(0.0, +2.0 * pi * zmag_here * L * i1)); // vertical 1
                            ph_jl = std::exp(cpx(0.0, +2.0 * pi * zmag_here * L * j1)); // vertical 2
                        }             

                        Mat4Site hop_mat; // 4x4, corresponding to sites i,j,k,l
                        hop_mat << 0 << ph_ij * hh << ph_ik * hv << 0         << arma::endr
                                << 0 << 0          << 0          << ph_jl * hv << arma::endr
                                << 0 << 0          << 0          << ph_kl * hh << arma::endr
                                << 0 << 0          << 0          << 0          << arma::endr;
                        hop_mat += arma::trans(hop_mat); // add hermitian conjugate for the other directions

                        // overall factor of -1
                        hop_mat *= -1;

                        // compute matrix exponential (with prefactor in exponent)
                        VecNum::fixed<4> eigval;       // hermitian matrix has real eigenvalues
                        Mat4Site eigvec;
                        arma::eig_sym(eigval, eigvec, hop_mat); // for hermitian matrix
                        
                        Mat4Site exp_hop_mat = eigvec *
                            arma::diagmat(arma::exp( prefactor * eigval )) *
                            arma::trans(eigvec);

                        // insert into precal storage
                        storage[band][subgroup][i] = exp_hop_mat;                    
                    }
                }
            }
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
template<class Matrix>
void DetSDW<CB, OPDIM>::cb_assaad_applyBondFactorsLeft_precalcedMatrices(Matrix& result, uint32_t subgroup,
                                                                         const ExpHop4SiteStorage& expHop4SiteMatrices) {
    const auto N = pars.N;
    const auto L = pars.L;
    assert(subgroup == 0 or subgroup == 1);
    arma::Row<DataType> new_row_i(N);
    arma::Row<DataType> new_row_j(N);
    arma::Row<DataType> new_row_k(N);
    for (uint32_t i1 = subgroup; i1 < L; i1 += 2) {
        for (uint32_t i2 = subgroup; i2 < L; i2 += 2) {
            uint32_t i = this->coordsToSite(i1, i2);
            uint32_t j = spaceNeigh(XPLUS, i);
            uint32_t k = spaceNeigh(YPLUS, i);
            uint32_t l = spaceNeigh(XPLUS, k);
            //change rows i,j,k,l of result
            const arma::Row<DataType>& ri = result.row(i);
            const arma::Row<DataType>& rj = result.row(j);
            const arma::Row<DataType>& rk = result.row(k);
            const arma::Row<DataType>& rl = result.row(l);

            const Mat4Site& mat = expHop4SiteMatrices[subgroup].find(i)->second;

            // indexes (0,1,2,3) correspond to (i,j,k,l)
            new_row_i     = mat(0,0)*ri + mat(0,1)*rj + mat(0,2)*rk + mat(0,3)*rl;
            new_row_j     = mat(1,0)*ri + mat(1,1)*rj + mat(1,2)*rk + mat(1,3)*rl;
            new_row_k     = mat(2,0)*ri + mat(2,1)*rj + mat(2,2)*rk + mat(2,3)*rl;
            result.row(l) = mat(3,0)*ri + mat(3,1)*rj + mat(3,2)*rk + mat(3,3)*rl;
            result.row(i) = new_row_i;
            result.row(j) = new_row_j;
            result.row(k) = new_row_k;
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
template<class Matrix>
void DetSDW<CB, OPDIM>::cb_assaad_applyBondFactorsRight_precalcedMatrices(Matrix& result, uint32_t subgroup,
                                                                          const ExpHop4SiteStorage& expHop4SiteMatrices) {
    const auto N = pars.N;
    const auto L = pars.L;
    assert(subgroup == 0 or subgroup == 1);
    arma::Col<DataType> new_col_i(N);
    arma::Col<DataType> new_col_j(N);
    arma::Col<DataType> new_col_k(N);
    for (uint32_t i1 = subgroup; i1 < L; i1 += 2) {
        for (uint32_t i2 = subgroup; i2 < L; i2 += 2) {
            uint32_t i = this->coordsToSite(i1, i2);
            uint32_t j = spaceNeigh(XPLUS, i);
            uint32_t k = spaceNeigh(YPLUS, i);
            uint32_t l = spaceNeigh(XPLUS, k);
            //change cols i,j,k,l of result
            const arma::Col<DataType>& ci = result.col(i);
            const arma::Col<DataType>& cj = result.col(j);
            const arma::Col<DataType>& ck = result.col(k);
            const arma::Col<DataType>& cl = result.col(l);

            const Mat4Site& mat = expHop4SiteMatrices[subgroup].find(i)->second;

            // indexes (0,1,2,3) correspond to (i,j,k,l)
            new_col_i     = ci*mat(0,0) + cj*mat(1,0) + ck*mat(2,0) + cl*mat(3,0);
            new_col_j     = ci*mat(0,1) + cj*mat(1,1) + ck*mat(2,1) + cl*mat(3,1);
            new_col_k     = ci*mat(0,2) + cj*mat(1,2) + ck*mat(2,2) + cl*mat(3,2);
            result.col(l) = ci*mat(0,3) + cj*mat(1,3) + ck*mat(2,3) + cl*mat(3,3);            
            result.col(i) = new_col_i;
            result.col(j) = new_col_j;
            result.col(k) = new_col_k;
        }
    }
}





template<CheckerboardMethod CB, int OPDIM>
template<class Matrix> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::cbLMultHoppingExp_impl(std::integral_constant<CheckerboardMethod, CB_NONE>,
                                          const Matrix&, Band, int, bool) {
    throw_GeneralError("CB_NONE makes no sense for the checkerboard multiplication routines");
    //TODO change things so this codepath is not needed
    return MatData();
}





//subgroup == 0: plaquettes A = [i j k l] = bonds (<ij>,<ik>,<kl>,<jl>)
//   i = (2m, 2n), with m,n integer, 2m < L, 2n < L
//   j = i + XPLUS
//   k = i + YPLUS
//   l = k + XPLUS
//subgroup == 1: plaquettes B = [i j k l] = bonds (<ij>,<ik>,<kl>,<jl>)
//   i = (2m+1, 2n+1), with m,n integer, 2m+1 < L, 2n+1 < L
//   j = i + XPLUS
//   k = i + YPLUS
//   l = k + XPLUS
template<CheckerboardMethod CB, int OPDIM>
template<class Matrix>
void DetSDW<CB, OPDIM>::cb_assaad_applyBondFactorsLeft(Matrix& result, uint32_t subgroup,
                                                       num ch_hor, num sh_hor, num ch_ver, num sh_ver) {
    const auto N = pars.N;
    const auto L = pars.L;
    assert(subgroup == 0 or subgroup == 1);
    arma::Row<DataType> new_row_i(N);
    arma::Row<DataType> new_row_j(N);
    arma::Row<DataType> new_row_k(N);
    for (uint32_t i1 = subgroup; i1 < L; i1 += 2) {
        for (uint32_t i2 = subgroup; i2 < L; i2 += 2) {
            uint32_t i = this->coordsToSite(i1, i2);
            uint32_t j = spaceNeigh(XPLUS, i);
            uint32_t k = spaceNeigh(YPLUS, i);
            uint32_t l = spaceNeigh(XPLUS, k);
            //change rows i,j,k,l of result
            const arma::Row<DataType>& ri = result.row(i);
            const arma::Row<DataType>& rj = result.row(j);
            const arma::Row<DataType>& rk = result.row(k);
            const arma::Row<DataType>& rl = result.row(l);
            num b_sh_hor = sh_hor;
            num b_sh_ver = sh_ver;
            if ((pars.bc == BC_Type::APBC_X or pars.bc == BC_Type::APBC_XY) and i1 == L-1) {
                //this plaquette has horizontal boundary crossing bonds and APBC
                b_sh_hor *= -1;
            }
            if ((pars.bc == BC_Type::APBC_Y or pars.bc == BC_Type::APBC_XY) and i2 == L-1) {
                //this plaquette has vertical boundary crossing bonds and APBC
                b_sh_ver *= -1;
            }
            new_row_i     = ch_hor*ch_ver*ri + ch_ver*b_sh_hor*rj + ch_hor*b_sh_ver*rk + b_sh_hor*b_sh_ver*rl;
            new_row_j     = ch_ver*b_sh_hor*ri + ch_hor*ch_ver*rj + b_sh_hor*b_sh_ver*rk + ch_hor*b_sh_ver*rl;
            new_row_k     = ch_hor*b_sh_ver*ri + b_sh_hor*b_sh_ver*rj + ch_hor*ch_ver*rk + ch_ver*b_sh_hor*rl;
            result.row(l) = b_sh_hor*b_sh_ver*ri + ch_hor*b_sh_ver*rj + ch_ver*b_sh_hor*rk + ch_hor*ch_ver*rl;
            result.row(i) = new_row_i;
            result.row(j) = new_row_j;
            result.row(k) = new_row_k;
        }
    }
}







// with sign = +/- 1, band = XBAND|YBAND: set R := E^(sign * dtau * K_band) * A
// using the symmetric checkerboard break up
template<CheckerboardMethod CB, int OPDIM>
template<class Matrix> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::cbLMultHoppingExp_impl(std::integral_constant<CheckerboardMethod, CB_ASSAAD_BERG>,
                                          const Matrix& A, Band band, int sign, bool) {
    MatData result = A;      //can't avoid this copy

    assert(sign == 1 or sign == -1);

    if (not pars.weakZflux) {
        // perform the multiplication e^(+-dtau (K_1)/2) e^(+-dtau K_0) e^(+-dtau (K_1)/2) X
        cb_assaad_applyBondFactorsLeft(result, 1, coshHopHorHalf[band], sign * sinhHopHorHalf[band],
                                       coshHopVerHalf[band], sign * sinhHopVerHalf[band]);
        cb_assaad_applyBondFactorsLeft(result, 0, coshHopHor[band], sign * sinhHopHor[band],
                                       coshHopVer[band], sign * sinhHopVer[band]);
        cb_assaad_applyBondFactorsLeft(result, 1, coshHopHorHalf[band], sign * sinhHopHorHalf[band],
                                       coshHopVerHalf[band], sign * sinhHopVerHalf[band]);
    } else {
        //order of matrix multiplications symmetric
        //perform the multiplication e^(+-dtau (K_1)/2) e^(+-dtau K_0) e^(+-dtau (K_1)/2) X
        if (sign == +1) {
            cb_assaad_applyBondFactorsLeft_precalcedMatrices(result, 1, expHop4Site_plusHalf[band]);
            cb_assaad_applyBondFactorsLeft_precalcedMatrices(result, 0, expHop4Site_plus[band]);
            cb_assaad_applyBondFactorsLeft_precalcedMatrices(result, 1, expHop4Site_plusHalf[band]);
        }
        else if (sign == -1) {
            cb_assaad_applyBondFactorsLeft_precalcedMatrices(result, 1, expHop4Site_minusHalf[band]);
            cb_assaad_applyBondFactorsLeft_precalcedMatrices(result, 0, expHop4Site_minus[band]);
            cb_assaad_applyBondFactorsLeft_precalcedMatrices(result, 1, expHop4Site_minusHalf[band]);
        }
    }

    return result;
}

// with A: NxN, sign = +/- 1, band = XBAND|YBAND: return a matrix equal to A * E^(sign * dtau * K_band)
template<CheckerboardMethod CB, int OPDIM>
template <class Matrix> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::cbLMultHoppingExp(const Matrix& A, Band band, int sign, bool invertedCbOrder) {
    return cbLMultHoppingExp_impl(std::integral_constant<CheckerboardMethod, CB>(),
                                  A, band, sign, invertedCbOrder);
}




template<CheckerboardMethod CB, int OPDIM>
template<class Matrix> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::cbRMultHoppingExp_impl(std::integral_constant<CheckerboardMethod, CB_NONE>,
                                          const Matrix&, Band, int, bool) {
    throw_GeneralError("CB_NONE makes no sense for the checkerboard multiplication routines");
    return MatData();
}


//subgroup == 0: plaquettes A = [i j k l] = bonds (<ij>,<ik>,<kl>,<jl>)
//   i = (2m, 2n), with m,n integer, 2m < L, 2n < L
//   j = i + XPLUS
//   k = i + YPLUS
//   l = k + XPLUS
//subgroup == 1: plaquettes B = [i j k l] = bonds (<ij>,<ik>,<kl>,<jl>)
//   i = (2m+1, 2n+1), with m,n integer, 2m+1 < L, 2n+1 < L
//   j = i + XPLUS
//   k = i + YPLUS
//   l = k + XPLUS
template<CheckerboardMethod CB, int OPDIM>
template<class Matrix>
void DetSDW<CB, OPDIM>::cb_assaad_applyBondFactorsRight(Matrix& result, uint32_t subgroup,
                                                        num ch_hor, num sh_hor, num ch_ver, num sh_ver) {
    const auto N = pars.N;
    const auto L = pars.L;
    assert(subgroup == 0 or subgroup == 1);
    arma::Col<DataType> new_col_i(N);
    arma::Col<DataType> new_col_j(N);
    arma::Col<DataType> new_col_k(N);
    for (uint32_t i1 = subgroup; i1 < L; i1 += 2) {
        for (uint32_t i2 = subgroup; i2 < L; i2 += 2) {
            uint32_t i = this->coordsToSite(i1, i2);
            uint32_t j = spaceNeigh(XPLUS, i);
            uint32_t k = spaceNeigh(YPLUS, i);
            uint32_t l = spaceNeigh(XPLUS, k);
            //change cols i,j,k,l of result
            const arma::Col<DataType>& ci = result.col(i);
            const arma::Col<DataType>& cj = result.col(j);
            const arma::Col<DataType>& ck = result.col(k);
            const arma::Col<DataType>& cl = result.col(l);
            num b_sh_hor = sh_hor;
            num b_sh_ver = sh_ver;
            if ((pars.bc == BC_Type::APBC_X or pars.bc == BC_Type::APBC_XY) and i1 == L-1) {
                //this plaquette has horizontal boundary crossing bonds and APBC
                b_sh_hor *= -1;
            }
            if ((pars.bc == BC_Type::APBC_Y or pars.bc == BC_Type::APBC_XY) and i2 == L-1) {
                //this plaquette has vertical boundary crossing bonds and APBC
                b_sh_ver *= -1;
            }
            new_col_i     = ch_hor*ch_ver*ci + ch_ver*b_sh_hor*cj + ch_hor*b_sh_ver*ck + b_sh_hor*b_sh_ver*cl;
            new_col_j     = ch_ver*b_sh_hor*ci + ch_hor*ch_ver*cj + b_sh_hor*b_sh_ver*ck + ch_hor*b_sh_ver*cl;
            new_col_k     = ch_hor*b_sh_ver*ci + b_sh_hor*b_sh_ver*cj + ch_hor*ch_ver*ck + ch_ver*b_sh_hor*cl;
            result.col(l) = b_sh_hor*b_sh_ver*ci + ch_hor*b_sh_ver*cj + ch_ver*b_sh_hor*ck + ch_hor*ch_ver*cl;
            result.col(i) = new_col_i;
            result.col(j) = new_col_j;
            result.col(k) = new_col_k;
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
template<class Matrix> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::cbRMultHoppingExp_impl(std::integral_constant<CheckerboardMethod, CB_ASSAAD_BERG>,
                                          const Matrix& A, Band band, int sign, bool) {
    typename DetSDW<CB, OPDIM>::MatData result = A;      //can't avoid this copy

    assert(sign == 1 or sign == -1);

    if (not pars.weakZflux) {
        //order of matrix multiplications symmetric
        //perform the multiplication X e^(+-dtau (K_1)/2) e^(+-dtau K_0) e^(+-dtau (K_1)/2)
        cb_assaad_applyBondFactorsRight(result, 1, coshHopHorHalf[band], sign * sinhHopHorHalf[band],
                                        coshHopVerHalf[band], sign * sinhHopVerHalf[band]);
        cb_assaad_applyBondFactorsRight(result, 0, coshHopHor[band], sign * sinhHopHor[band],
                                        coshHopVer[band], sign * sinhHopVer[band]);
        cb_assaad_applyBondFactorsRight(result, 1, coshHopHorHalf[band], sign * sinhHopHorHalf[band],
                                        coshHopVerHalf[band], sign * sinhHopVerHalf[band]);
    } else {
        //order of matrix multiplications symmetric
        //perform the multiplication X e^(+-dtau (K_1)/2) e^(+-dtau K_0) e^(+-dtau (K_1)/2)
        if (sign == +1) {
            cb_assaad_applyBondFactorsRight_precalcedMatrices(result, 1, expHop4Site_plusHalf[band]);
            cb_assaad_applyBondFactorsRight_precalcedMatrices(result, 0, expHop4Site_plus[band]);
            cb_assaad_applyBondFactorsRight_precalcedMatrices(result, 1, expHop4Site_plusHalf[band]);
        }
        else if (sign == -1) {
            cb_assaad_applyBondFactorsRight_precalcedMatrices(result, 1, expHop4Site_minusHalf[band]);
            cb_assaad_applyBondFactorsRight_precalcedMatrices(result, 0, expHop4Site_minus[band]);
            cb_assaad_applyBondFactorsRight_precalcedMatrices(result, 1, expHop4Site_minusHalf[band]);
        }
    }

    return result;
}

// with sign = +/- 1, band = XBAND|YBAND: return A * E^(sign * dtau * K_band)
template<CheckerboardMethod CB, int OPDIM>
template <class Matrix> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::cbRMultHoppingExp(const Matrix& A, Band band, int sign, bool invertedCbOrder) {
    return cbRMultHoppingExp_impl(std::integral_constant<CheckerboardMethod, CB>(),
                                  A, band, sign, invertedCbOrder);
}





template<CheckerboardMethod CB, int OPDIM> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::leftMultiplyBk(const typename DetSDW<CB, OPDIM>::MatData& orig, uint32_t k) {
    const auto N = pars.N;
    //helper: submatrix block for a matrix
#define block(mat,row,col) mat.submat( (row) * N, (col) * N, ((row) + 1) * N - 1, ((col) + 1) * N - 1)

    const auto& ksinhTermPhi = sinhTermPhi.col(k);
    const auto& kcoshTermPhi = coshTermPhi.col(k);
    const auto& ksinhTermCDWl = sinhTermCDWl.col(k);
    const auto& kcoshTermCDWl = coshTermCDWl.col(k);
    const VecNum cd  = (pars.cdwU ? (kcoshTermPhi % kcoshTermCDWl + ksinhTermCDWl).eval() : kcoshTermPhi);
    const VecNum cmd = (pars.cdwU ? (kcoshTermPhi % kcoshTermCDWl - ksinhTermCDWl).eval() : kcoshTermPhi);

    VecNum ax, max;
    if (OPDIM == 3) {
        const auto& kphi2 = phi.slice(k).col(2);
        ax  = (pars.cdwU ? (kphi2 % ksinhTermPhi % kcoshTermCDWl).eval() : (kphi2 % ksinhTermPhi).eval());
        max = -ax;
    }
    VecData b, bc;
    const auto& kphi0 = phi.slice(k).col(0);
    b.set_size( N);
    bc.set_size(N);
    // b.set_real( kphi0);
    // bc.set_real(kphi0);
    setVectorReal(b,  kphi0);
    setVectorReal(bc, kphi0);
    if (OPDIM >  1) {
        const auto& kphi1 = (OPDIM >  1 ? phi.slice(k).col(1) : kphi0);
        // b.set_imag( -kphi1);
        // bc.set_imag( kphi1);
        setVectorImag(b, -kphi1);
        setVectorImag(bc, kphi1);
    }
    VecData mbx  = (pars.cdwU ? (-b  % ksinhTermPhi % kcoshTermCDWl).eval() : (-b  % ksinhTermPhi).eval());
    VecData mbcx = (pars.cdwU ? (-bc % ksinhTermPhi % kcoshTermCDWl).eval() : (-bc % ksinhTermPhi).eval());

    //overall factor for entire matrix for chemical potential
//    num ovFac = std::exp(pars.dtau*pars.mu);
    checkarray<num, 2> mu;
    mu[XBAND] = pars.mux;
    mu[YBAND] = pars.muy;
    num ovFacXBAND = std::exp(pars.dtau*mu[XBAND]);
    num ovFacYBAND = std::exp(pars.dtau*mu[YBAND]);
    
    // CHECK_VEC_NAN(mbx);
    // CHECK_VEC_NAN(mbcx);

    MatData result(MatrixSizeFactor*N, MatrixSizeFactor*N);

    for (uint32_t col = 0; col < MatrixSizeFactor; ++col) {
        using arma::diagmat;
        block(result, 0, col) = diagmat(ovFacXBAND * cd)   * cbLMultHoppingExp(block(orig, 0, col), XBAND, -1, false)
                              + diagmat(ovFacYBAND * mbx)  * cbLMultHoppingExp(block(orig, 1, col), YBAND, -1, false);

        block(result, 1, col) = diagmat(ovFacXBAND * mbcx) * cbLMultHoppingExp(block(orig, 0, col), XBAND, -1, false)
                              + diagmat(ovFacYBAND * cmd)  * cbLMultHoppingExp(block(orig, 1, col), YBAND, -1, false);

        if (OPDIM == 3) {
            block(result, 0, col) += diagmat(ovFacYBAND * max) * cbLMultHoppingExp(block(orig, 3, col), YBAND, -1, false);
            block(result, 1, col) += diagmat(ovFacXBAND * ax)  * cbLMultHoppingExp(block(orig, 2, col), XBAND, -1, false);

            //only a total of three terms each time because of zero blocks in the E^(-dtau*V) matrix
            block(result, 2, col) = diagmat(ovFacYBAND * ax)   * cbLMultHoppingExp(block(orig, 1, col), YBAND, -1, false)
                                  + diagmat(ovFacXBAND * cd)   * cbLMultHoppingExp(block(orig, 2, col), XBAND, -1, false)
                                  + diagmat(ovFacYBAND * mbcx) * cbLMultHoppingExp(block(orig, 3, col), YBAND, -1, false);

            block(result, 3, col) = diagmat(ovFacXBAND * max)  * cbLMultHoppingExp(block(orig, 0, col), XBAND, -1, false)
                                  + diagmat(ovFacXBAND * mbx)  * cbLMultHoppingExp(block(orig, 2, col), XBAND, -1, false)
                                  + diagmat(ovFacYBAND * cmd)  * cbLMultHoppingExp(block(orig, 3, col), YBAND, -1, false);
        }
    }

#undef block
    return result;
}



template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::checkerboardLeftMultiplyBmat(const typename DetSDW<CB, OPDIM>::MatData& A, uint32_t k2, uint32_t k1) {
    assert(k2 > k1);
    assert(k2 <= pars.m);

    MatData result = leftMultiplyBk(A, k1 + 1);

    for (uint32_t k = k1 + 2; k <= k2; ++k) {
        result = leftMultiplyBk(result, k);
    }

    //chemical potential terms included by leftMultiplyBk
    //result *= std::exp(+dtau * (k2 - k1) * mu);

    return result;
}


template<CheckerboardMethod CB, int OPDIM> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::leftMultiplyBkInv(const typename DetSDW<CB, OPDIM>::MatData& orig, uint32_t k) {
    const auto N = pars.N;
    //helper: submatrix block for a matrix
#define block(mat,row,col) mat.submat( (row) * N, (col) * N, ((row) + 1) * N - 1, ((col) + 1) * N - 1)

    const auto& ksinhTermPhi = sinhTermPhi.col(k);
    const auto& kcoshTermPhi = coshTermPhi.col(k);
    const auto& ksinhTermCDWl = sinhTermCDWl.col(k);
    const auto& kcoshTermCDWl = coshTermCDWl.col(k);
    const VecNum cd  = (pars.cdwU ? (kcoshTermPhi % kcoshTermCDWl + ksinhTermCDWl).eval() : kcoshTermPhi);
    const VecNum cmd = (pars.cdwU ? (kcoshTermPhi % kcoshTermCDWl - ksinhTermCDWl).eval() : kcoshTermPhi);

    VecNum ax, max;
    if (OPDIM == 3) {
        const auto& kphi2 = phi.slice(k).col(2);
        ax  = (pars.cdwU ? (kphi2 % ksinhTermPhi % kcoshTermCDWl).eval() : (kphi2 % ksinhTermPhi).eval());
        max = -ax;
    }

    const auto& kphi0 = phi.slice(k).col(0);
    VecData b, bc;
    b.set_size( N);
    bc.set_size(N);
    // b.set_real( kphi0);
    // bc.set_real(kphi0);
    setVectorReal(b,  kphi0);
    setVectorReal(bc, kphi0);
    if (OPDIM >  1) {
        const auto& kphi1 = (OPDIM >  1 ? phi.slice(k).col(1) : kphi0);
        // b.set_imag( -kphi1);
        // bc.set_imag( kphi1);
        setVectorImag(b,  -kphi1);
        setVectorImag(bc,  kphi1);
    }
    VecData bx  = (pars.cdwU ? (b  % ksinhTermPhi % kcoshTermCDWl).eval() : (b  % ksinhTermPhi).eval());
    VecData bcx = (pars.cdwU ? (bc % ksinhTermPhi % kcoshTermCDWl).eval() : (bc % ksinhTermPhi).eval());

    //overall factor for entire matrix for chemical potential
//    num ovFac = std::exp(-pars.dtau*pars.mu);
    checkarray<num, 2> mu;
    mu[XBAND] = pars.mux;
    mu[YBAND] = pars.muy;
    num ovFacXBAND = std::exp(-pars.dtau*mu[XBAND]);
    num ovFacYBAND = std::exp(-pars.dtau*mu[YBAND]);
    
    MatData result(MatrixSizeFactor*N, MatrixSizeFactor*N);

    for (uint32_t col = 0; col < MatrixSizeFactor; ++col) {
        using arma::diagmat;
        block(result, 0, col) = cbLMultHoppingExp(diagmat(ovFacXBAND * cmd) * block(orig, 0, col), XBAND, +1, true)
                              + cbLMultHoppingExp(diagmat(ovFacXBAND * bx)  * block(orig, 1, col), XBAND, +1, true);

        block(result, 1, col) = cbLMultHoppingExp(diagmat(ovFacYBAND * bcx) * block(orig, 0, col), YBAND, +1, true)
                              + cbLMultHoppingExp(diagmat(ovFacYBAND * cd)  * block(orig, 1, col), YBAND, +1, true);

        if (OPDIM == 3) {
            block(result, 0, col) += cbLMultHoppingExp(diagmat(ovFacXBAND * ax)  * block(orig, 3, col), XBAND, +1, true);
            block(result, 1, col) += cbLMultHoppingExp(diagmat(ovFacYBAND * max) * block(orig, 2, col), YBAND, +1, true);

            //only a total of three terms each time because of zero blocks in the E^(+dtau*V) matrix
            block(result, 2, col) = cbLMultHoppingExp(diagmat(ovFacXBAND * max)  * block(orig, 1, col), XBAND, +1, true)
                                  + cbLMultHoppingExp(diagmat(ovFacXBAND * cmd)  * block(orig, 2, col), XBAND, +1, true)
                                  + cbLMultHoppingExp(diagmat(ovFacXBAND * bcx)  * block(orig, 3, col), XBAND, +1, true);

            block(result, 3, col) = cbLMultHoppingExp(diagmat(ovFacYBAND * ax)   * block(orig, 0, col), YBAND, +1, true)
                                  + cbLMultHoppingExp(diagmat(ovFacYBAND * bx)   * block(orig, 2, col), YBAND, +1, true)
                                  + cbLMultHoppingExp(diagmat(ovFacYBAND * cd)   * block(orig, 3, col), YBAND, +1, true);
        }
    }

#undef block
    return result;
}


template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::checkerboardLeftMultiplyBmatInv(const typename DetSDW<CB, OPDIM>::MatData& A, uint32_t k2, uint32_t k1) {
    assert(k2 > k1);
    assert(k2 <= pars.m);

    MatData result = leftMultiplyBkInv(A, k2);

    for (uint32_t k = k2 - 1; k >= k1 + 1; --k) {
        result = leftMultiplyBkInv(result, k);
    }

    //chemical potential terms already included
    //result *= std::exp(-dtau * (k2 - k1) * mu);

    return result;
}

template<CheckerboardMethod CB, int OPDIM> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::rightMultiplyBk(const typename DetSDW<CB, OPDIM>::MatData& orig, uint32_t k) {
    const auto N = pars.N;
    //helper: submatrix block for a matrix
#define block(mat,row,col) mat.submat( (row) * N, (col) * N, ((row) + 1) * N - 1, ((col) + 1) * N - 1)

    const auto& ksinhTermPhi = sinhTermPhi.col(k);
    const auto& kcoshTermPhi = coshTermPhi.col(k);
    const auto& ksinhTermCDWl = sinhTermCDWl.col(k);
    const auto& kcoshTermCDWl = coshTermCDWl.col(k);
    // // define is safer than auto...
    // #define ksinhTermPhi sinhTermPhi.col(k)
    // #define kcoshTermPhi coshTermPhi.col(k)
    // #define ksinhTermCDWl sinhTermCDWl.col(k)
    // #define kcoshTermCDWl coshTermCDWl.col(k)

    const VecNum cd  = (pars.cdwU ? (kcoshTermPhi % kcoshTermCDWl + ksinhTermCDWl).eval() : kcoshTermPhi);
    const VecNum cmd = (pars.cdwU ? (kcoshTermPhi % kcoshTermCDWl - ksinhTermCDWl).eval() : kcoshTermPhi);

    VecNum ax, max;
    if (OPDIM == 3) {
        #define kphi2  phi.slice(k).col(2)
        ax  = (pars.cdwU ? (kphi2 % ksinhTermPhi % kcoshTermCDWl).eval() : (kphi2 % ksinhTermPhi).eval());
        max = -ax;
        #undef kphi2
    }
    #define kphi0 phi.slice(k).col(0)
    // CHECK_VEC_NAN(phi.slice(k).col(0));
    VecData b, bc;
    b.set_size( N);
    bc.set_size(N);
//    b.set_real( kphi0);
//    bc.set_real(kphi0);
    setVectorReal(b,  kphi0);
    setVectorReal(bc, kphi0);
    #undef kphi0
    //DEBUG
    // for (uint32_t count = 0; count < N; ++count) {
    //     b[count].real(kphi0[count]);
    //     bc[count].real(kphi0[count]);
    // }
    // CHECK_VEC_NAN(b);
    // CHECK_VEC_NAN(bc);
    //ENDDEBUG
    if (OPDIM >  1) {
#define kphi1 (phi.slice(k).col(OPDIM >  1 ? 1 : 0))
        // CHECK_VEC_NAN(phi.slice(k).col(OPDIM >  1 ? 1 : 0))
        // b.set_imag( -kphi1);
        // bc.set_imag( kphi1);
        setVectorImag(b, -kphi1);
        setVectorImag(bc, kphi1);
        // CHECK_VEC_NAN(b);
        // CHECK_VEC_NAN(bc);
        #undef kphi1
    }
    VecData mbx  = (pars.cdwU ? (-b  % ksinhTermPhi % kcoshTermCDWl).eval() : (-b  % ksinhTermPhi).eval());
    VecData mbcx = (pars.cdwU ? (-bc % ksinhTermPhi % kcoshTermCDWl).eval() : (-bc % ksinhTermPhi).eval());

    // #undef ksinhTermPhi
    // #undef kcoshTermPhi
    // #undef ksinhTermCDWl
    // #undef kcoshTermCDWl

    //DEBUG
    // CHECK_VEC_NAN(ksinhTermPhi);
    // CHECK_VEC_NAN(kcoshTermPhi);
    // CHECK_VEC_NAN(ksinhTermPhi);
    // CHECK_VEC_NAN(kcoshTermCDWl);
    // CHECK_VEC_NAN(ksinhTermCDWl);
    // CHECK_VEC_NAN(cd);
    // CHECK_VEC_NAN(cmd);
    // CHECK_VEC_NAN(ax);
    // CHECK_VEC_NAN(max);
    // CHECK_VEC_NAN(b);
    // CHECK_VEC_NAN(bc);
    // CHECK_VEC_NAN(mbx);
    // CHECK_VEC_NAN(mbcx);
    //END DEBUG

    //overall factor for entire matrix for chemical potential
//    num ovFac = std::exp(pars.dtau*pars.mu);
    checkarray<num, 2> mu;
    mu[XBAND] = pars.mux;
    mu[YBAND] = pars.muy;
    num ovFacXBAND = std::exp(pars.dtau*mu[XBAND]);
    num ovFacYBAND = std::exp(pars.dtau*mu[YBAND]);

    MatData result(MatrixSizeFactor*N, MatrixSizeFactor*N);

    for (uint32_t row = 0; row < MatrixSizeFactor; ++row) {
        using arma::diagmat;
        block(result, row, 0) = cbRMultHoppingExp(block(orig, row, 0) * ovFacXBAND * diagmat(cd),   XBAND, -1, false)
                              + cbRMultHoppingExp(block(orig, row, 1) * ovFacXBAND * diagmat(mbcx), XBAND, -1, false);

        block(result, row, 1) = cbRMultHoppingExp(block(orig, row, 0) * ovFacYBAND * diagmat(mbx),  YBAND, -1, false)
                              + cbRMultHoppingExp(block(orig, row, 1) * ovFacYBAND * diagmat(cmd),  YBAND, -1, false);

        if (OPDIM == 3) {
            //only a total of three terms each time because of zero blocks in the E^(-dtau*V) matrix
            block(result, row, 0) += cbRMultHoppingExp(block(orig, row, 3) * ovFacXBAND * diagmat(max), XBAND, -1, false);
            block(result, row, 1) += cbRMultHoppingExp(block(orig, row, 2) * ovFacYBAND * diagmat(ax),  YBAND, -1, false);

            block(result, row, 2) = cbRMultHoppingExp(block(orig, row, 1) * ovFacXBAND * diagmat(ax),   XBAND, -1, false)
                                  + cbRMultHoppingExp(block(orig, row, 2) * ovFacXBAND * diagmat(cd),   XBAND, -1, false)
                                  + cbRMultHoppingExp(block(orig, row, 3) * ovFacXBAND * diagmat(mbx),  XBAND, -1, false);

            block(result, row, 3) = cbRMultHoppingExp(block(orig, row, 0) * ovFacYBAND * diagmat(max),  YBAND, -1, false)
                                  + cbRMultHoppingExp(block(orig, row, 2) * ovFacYBAND * diagmat(mbcx), YBAND, -1, false)
                                  + cbRMultHoppingExp(block(orig, row, 3) * ovFacYBAND * diagmat(cmd),  YBAND, -1, false);
        }
    }

#undef block
    return result;
}

template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::checkerboardRightMultiplyBmat(const typename DetSDW<CB, OPDIM>::MatData& A, uint32_t k2, uint32_t k1) {
    assert(k2 > k1);
    assert(k2 <= pars.m);

    MatData result = rightMultiplyBk(A, k2);

    // std::cout << k2 << " "; CHECK_NAN(result);

    for (uint32_t k = k2 - 1; k >= k1 +1; --k) {
        result = rightMultiplyBk(result, k);
        // std::cout << k << " "; CHECK_NAN(result);
    }

    //chemical potential terms included above
    //result *= std::exp(+dtau * (k2 - k1) * mu);

    return result;
}

template<CheckerboardMethod CB, int OPDIM> inline
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::rightMultiplyBkInv(const typename DetSDW<CB, OPDIM>::MatData& orig, uint32_t k) {
    const auto N = pars.N;
    //helper: submatrix block for a matrix
#define block(mat,row,col) mat.submat( (row) * N, (col) * N, ((row) + 1) * N - 1, ((col) + 1) * N - 1)

    const auto& ksinhTermPhi = sinhTermPhi.col(k);
    const auto& kcoshTermPhi = coshTermPhi.col(k);
    const auto& ksinhTermCDWl = sinhTermCDWl.col(k);
    const auto& kcoshTermCDWl = coshTermCDWl.col(k);
    // const VecNum cd  = kcoshTermPhi % kcoshTermCDWl + ksinhTermCDWl;
    // const VecNum cmd = kcoshTermPhi % kcoshTermCDWl - ksinhTermCDWl;
    const VecNum cd  = (pars.cdwU ? (kcoshTermPhi % kcoshTermCDWl + ksinhTermCDWl).eval() : kcoshTermPhi);
    const VecNum cmd = (pars.cdwU ? (kcoshTermPhi % kcoshTermCDWl - ksinhTermCDWl).eval() : kcoshTermPhi);

    VecNum ax, max;
    if (OPDIM == 3) {
        const auto& kphi2 = phi.slice(k).col(2);
        ax  =  (pars.cdwU ? (kphi2 % ksinhTermPhi % kcoshTermCDWl).eval() : (kphi2 % ksinhTermPhi).eval());
        max = -ax;
    }

    const auto& kphi0 = phi.slice(k).col(0);
    VecData b, bc;
    b.set_size( N);
    bc.set_size(N);
    // b.set_real( kphi0);
    // bc.set_real(kphi0);
    setVectorReal(b,  kphi0);
    setVectorReal(bc, kphi0);
    if (OPDIM >  1) {
        const auto& kphi1 = (OPDIM >  1 ? phi.slice(k).col(1) : kphi0);
        // b.set_imag( -kphi1);
        // bc.set_imag( kphi1);
        setVectorImag(b, -kphi1);
        setVectorImag(bc, kphi1);
    }
    VecData bx  = (pars.cdwU ? (b  % ksinhTermPhi % kcoshTermCDWl).eval() : (b  % ksinhTermPhi).eval());
    VecData bcx = (pars.cdwU ? (bc % ksinhTermPhi % kcoshTermCDWl).eval() : (bc % ksinhTermPhi).eval());

    //overall factor for entire matrix for chemical potential
//    num ovFac = std::exp(-pars.dtau*pars.mu);
    checkarray<num, 2> mu;
    mu[XBAND] = pars.mux;
    mu[YBAND] = pars.muy;
    num ovFacXBAND = std::exp(-pars.dtau*mu[XBAND]);
    num ovFacYBAND = std::exp(-pars.dtau*mu[YBAND]);
    
    MatData result(MatrixSizeFactor*N, MatrixSizeFactor*N);

    for (uint32_t row = 0; row < MatrixSizeFactor; ++row) {
        using arma::diagmat;
        block(result, row, 0) = cbRMultHoppingExp(block(orig, row, 0), XBAND, +1, true) * ovFacXBAND * diagmat(cmd)
                              + cbRMultHoppingExp(block(orig, row, 1), YBAND, +1, true) * ovFacYBAND * diagmat(bcx);

        block(result, row, 1) = cbRMultHoppingExp(block(orig, row, 0), XBAND, +1, true) * ovFacXBAND * diagmat(bx)
                              + cbRMultHoppingExp(block(orig, row, 1), YBAND, +1, true) * ovFacYBAND * diagmat(cd);

        if (OPDIM == 3) {
            //only a total of three terms each time because of zero blocks in the E^(-dtau*V) matrix
            block(result, row, 0) += cbRMultHoppingExp(block(orig, row, 3), YBAND, +1, true) * ovFacYBAND * diagmat(ax);
            block(result, row, 1) += cbRMultHoppingExp(block(orig, row, 2), XBAND, +1, true) * ovFacXBAND * diagmat(max);

            block(result, row, 2) = cbRMultHoppingExp(block(orig, row, 1),  YBAND, +1, true) * ovFacYBAND * diagmat(max)
                                  + cbRMultHoppingExp(block(orig, row, 2),  XBAND, +1, true) * ovFacXBAND * diagmat(cmd)
                                  + cbRMultHoppingExp(block(orig, row, 3),  YBAND, +1, true) * ovFacYBAND * diagmat(bx);

            block(result, row, 3) = cbRMultHoppingExp(block(orig, row, 0),  XBAND, +1, true) * ovFacXBAND * diagmat(ax)
                                  + cbRMultHoppingExp(block(orig, row, 2),  XBAND, +1, true) * ovFacXBAND * diagmat(bcx)
                                  + cbRMultHoppingExp(block(orig, row, 3),  YBAND, +1, true) * ovFacYBAND * diagmat(cd);
        }

    }
    return result;
#undef block
}

template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::checkerboardRightMultiplyBmatInv(const typename DetSDW<CB, OPDIM>::MatData& A, uint32_t k2, uint32_t k1) {
    assert(k2 > k1);
    assert(k2 <= pars.m);

    MatData result = rightMultiplyBkInv(A, k1 + 1);

    for (uint32_t k = k1 + 2; k <= k2; ++k) {
        result = rightMultiplyBkInv(result, k);
    }

    //chemical potential terms included above
    //result *= std::exp(-dtau * (k2 - k1) * mu);

    return result;
}






template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateInSlice(uint32_t timeslice) {
    timing.start("sdw-updateInSlice");

    if (not pars.phiFixed) {
    
        //reset normal_distribution -- this way we do not need to worry about its internal
        //state during serialization
        normal_distribution.reset();

        // update phi-fields according to chosen method
        for (uint32_t rep = 0; rep < pars.repeatUpdateInSlice; ++rep) {
            switch (pars.spinProposalMethod) {
            case SpinProposalMethod_Type::BOX:
                ad.lastAccRatioLocal_phi = callUpdateInSlice_for_updateMethod(
                    timeslice,
                    [this](uint32_t site, uint32_t timeslice) -> changedPhiInt {
                        return this->proposeNewPhiBox(site, timeslice);
                    } );
                break;
            case SpinProposalMethod_Type::ROTATE_THEN_SCALE:
                //each sweep, alternate between rotating and scaling
                if (performedSweeps % 2 == 0) {
                    ad.lastAccRatioLocal_phi = callUpdateInSlice_for_updateMethod(
                        timeslice,
                        [this](uint32_t site, uint32_t timeslice) -> changedPhiInt {
                            return this->proposeRotatedPhi(site, timeslice);
                        } );
                } else {
                    ad.lastAccRatioLocal_phi = callUpdateInSlice_for_updateMethod(
                        timeslice,
                        [this](uint32_t site, uint32_t timeslice) -> changedPhiInt {
                            return this->proposeScaledPhi(site, timeslice);
                        } );
                }
                break;
            case SpinProposalMethod_Type::ROTATE_AND_SCALE:
                ad.lastAccRatioLocal_phi = callUpdateInSlice_for_updateMethod(
                    timeslice,
                    [this](uint32_t site, uint32_t timeslice) -> changedPhiInt {
                        return this->proposeRotatedScaledPhi(site, timeslice);
                    } );
                break;
            }
        }


        if (not pars.turnoffFermions and pars.cdwU) { // we have not set cdwU to 0.0 and fermions are on

            //Update discrete cdwl fields
            //currently just discard this accratio

            callUpdateInSlice_for_updateMethod(
                timeslice,
                [this](uint32_t site, uint32_t timeslice) -> changedPhiInt {
                    return this->proposeNewCDWl(site, timeslice);
                } );

        }
    }
    
    timing.stop("sdw-updateInSlice");
}

template<CheckerboardMethod CB, int OPDIM>
template<class Callable>
num DetSDW<CB, OPDIM>::updateInSlice_iterative(uint32_t timeslice, Callable proposeLocalUpdate) {
    num accratio = 0.;
    for (uint32_t site = 0; site < pars.N; ++site) {
        Phi newphi;
        int32_t new_cdwl;
        Changed changed;
        std::tie(changed, newphi, new_cdwl) = proposeLocalUpdate(site, timeslice);
        if (changed == NONE) {
            //reject this change
            continue;
        }

//      VecNum oldphi0 = phi0.col(timeslice);
//      VecNum oldphi1 = phi1.col(timeslice);
//      VecNum oldphi2 = phi2.col(timeslice);
//      debugSaveMatrix(oldphi0, "old_phi0");
//      debugSaveMatrix(oldphi1, "old_phi1");
//      debugSaveMatrix(oldphi2, "old_phi2");

//      VecNum newphi0 = phi0.col(timeslice);
//      VecNum newphi1 = phi1.col(timeslice);
//      VecNum newphi2 = phi2.col(timeslice);
//      newphi0[site] = newphi[0];
//      newphi1[site] = newphi[1];
//      newphi2[site] = newphi[2];
//      debugSaveMatrix(newphi0, "new_phi0");
//      debugSaveMatrix(newphi1, "new_phi1");
//      debugSaveMatrix(newphi2, "new_phi2");

        num probSPhi = 1.;
        if (changed == PHI) {
            num dsphi = deltaSPhi(site, timeslice, newphi);
            probSPhi = std::exp(-dsphi);
        }
//      std::cout << probSPhi << std::endl;

        //delta = e^(-dtau*V_new)*e^(+dtau*V_old) - 1

        num probSFermion = 1.0;
        num prob_cdwl = 1.0;

        checkarray<VecData, MatrixSizeFactor> rows;

        if (not pars.turnoffFermions) {

            MatSmall delta_forsite =
                get_delta_forsite(newphi, new_cdwl, timeslice, site);

            //****
            //Compute the determinant and inverse of I + Delta*(I - G)
            //based on Sherman-Morrison formula / Matrix-Determinant lemma
            //****

            //Delta*(I - G) is a sparse matrix containing just 4 rows:
            //site, site+N, site+2N, site+3N
            //Compute the values of these rows [O(N)]:
            for (uint32_t r = 0; r < MatrixSizeFactor; ++r) {
                //TODO: Here are some unnecessary operations:
                //delta_forsite contains many repeated elements, and even
                //some zeros
                rows[r] = VecData(MatrixSizeFactor*pars.N);
                for (uint32_t col = 0; col < MatrixSizeFactor*pars.N; ++col) {
                    rows[r][col] = -delta_forsite(r,0) * g.col(col)[site];
                }
                rows[r][site] += delta_forsite(r,0);
                for (uint32_t dc = 1; dc < MatrixSizeFactor; ++dc) {
                    for (uint32_t col = 0; col < 4*pars.N; ++col) {
                        rows[r][col] += -delta_forsite(r,dc) * g.col(col)[site + dc*pars.N];
                    }
                    rows[r][site + dc*pars.N] += delta_forsite(r,dc);
                }
            }

            // [I + Delta*(I - G)]^(-1) again is a sparse matrix
            // with two (four) rows site, site+N(, site+2N, site+3N).
            // compute them iteratively, together with the determinant of
            // I + Delta*(I - G)
            // Apart from these rows, the remaining diagonal entries of
            // [I + Delta*(I - G)]^(-1) are 1
            //
            // before this loop rows[] holds the entries of Delta*(I - G),
            // after the loop rows[] holds the corresponding rows of [I + Delta*(I - G)]^(-1)
            DataType det = 1;
            for (uint32_t l = 0; l < MatrixSizeFactor; ++l) {
                VecData row = rows[l];
                for (int k = int(l)-1; k >= 0; --k) {
                    row[site + unsigned(k)*pars.N] = 0;
                }
                for (int k = int(l)-1; k >= 0; --k) {
                    row += rows[l][site + unsigned(k)*pars.N] * rows[(unsigned)k];
                }
                DataType divisor = DataType(1) + row[site + l*pars.N];
                rows[l] = (-1.0/divisor) * row;
                rows[l][site + l*pars.N] += 1;
                for (int k = int(l) - 1; k >= 0; --k) {
                    rows[(unsigned)k] -= (rows[unsigned(k)][site + l*pars.N] / divisor) * row;
                }
                det *= divisor;
            }

            //****DEBUG
//      checkarray<VecCpx, 4> invRows = {{rows[0], rows[1], rows[2], rows[3]}};
            //****END-DEBUG

            //****
            //DEBUG: This slow code was working before.
            //Compare results with the better-performing sherman-morrison code
//      SpMatCpx delta(4*N, 4*N);
//      arma::uvec::fixed<4> idx = {site, site + N, site + 2*N, site + 3*N};
//      //Armadilo lacks non-contiguous submatrix views for sparse matrices
//      uint32_t i = 0;
//      for (auto col: idx) {
//          uint32_t j = 0;
//          for (auto row: idx) {
//              delta(row, col) = deltanonzero(j, i);
//              ++j;
//          }
//          ++i;
//      }


//      MatCpx deltaDense(4*N,4*N);
//      deltaDense = delta;
//      debugSaveMatrix(MatNum(arma::real(deltaDense)), "delta_real");
//      debugSaveMatrix(MatNum(arma::imag(deltaDense)), "delta_imag");

//      //inefficient!
//      static MatCpx eyeCpx = MatCpx(arma::eye(4*N, 4*N), arma::zeros(4*N, 4*N));
//      MatCpx target = eyeCpx + delta * (eyeCpx - g.slice(timeslice));

//      debugSaveMatrix(MatNum(arma::real(target)), "target_real");
//      debugSaveMatrix(MatNum(arma::imag(target)), "target_imag");

//      cpx weightRatio = arma::det(target);
//      std::cout << weightRatio << std::endl;

//      std::cout << weightRatio << " vs. " << det << std::endl;
            //END DEBUG
            //****

            //****
            // DEBUG-CHECK Delta*(I - G) vs. rows
//      MatCpx check = delta * (eyeCpx - g.slice(timeslice));
//      for (uint32_t r = 0; r < 4; ++r) {
//          check.row(site + r*N) -= rows[r].st();
//      }
//      std::cout << "Row check: " << arma::max(arma::max(arma::abs(check))) << std::endl;
//      debugSaveMatrix(MatNum(arma::real(check)), "check_real");
//      debugSaveMatrix(MatNum(arma::imag(check)), "check_imag");
//      exit(0);
            // END-DEBUG-CHECK
            //****

            //****
            //DEBUG-CHECK [I+Delta*(I - G)]^(-1) vs. invRows
//      MatCpx inv = arma::inv(target);
//      inv -= eyeCpx;
//      for (uint32_t r = 0; r < 4; ++r) {
//          inv.row(site + r*N) -= invRows[r].st();
//          inv.row(site + r*N)[site + r*N] += 1;
//      }
//      std::cout << "inv-div: " << arma::max(arma::max(inv)) << std::endl;
            //END-DEBUG-CHECK
            //****

            if (OPDIM == 3) {
                probSFermion = dataReal(det);
            } else {
                //      /G 0 \                .
                //  det \0 G*/ = |det G|^2
                probSFermion = std::pow(std::abs(det), 2);
            }

            //DEBUG: determinant computation from new routine updateInSlice_woodbury:
//        MatCpx::fixed<4,4> g_sub;
//        for (uint32_t a = 0; a < 4; ++a) {
//            for (uint32_t b = 0; b < 4; ++b) {
//                g_sub(a,b) = g(site + a*N, site + b*N);
//            }
//        }
//        MatCpx::fixed<4,4> M = eye4cpx + (eye4cpx - g_sub) * deltanonzero;                        //!
//        std::cout << "det: " << (probSFermion - arma::det(M).real()) / probSFermion << "\n";
            //END-DEBUG: relative difference: 0 or at most ~E-16 --> results are equal

            prob_cdwl = cdwl_gamma(new_cdwl) / cdwl_gamma(cdwl(site, timeslice));

        } else {
            probSFermion = 1.0;
            prob_cdwl = 1.0;
        }

        num prob = probSPhi * probSFermion * prob_cdwl;

        if (prob > 1.0 or rng.rand01() < prob) {
            //count accepted update
            accratio += 1;

//          num phisBefore = phiAction();
            for (uint32_t dim = 0; dim < OPDIM; ++dim) {
                phi(site, dim, timeslice) = newphi[dim];
            }

            if (not pars.turnoffFermions) {

                cdwl(site, timeslice) = new_cdwl;
                updateCoshSinhTerms(site, timeslice);
//          num phisAfter = phiAction();
//          std::cout << std::scientific << dsphi << " vs. " << phisAfter << " - " << phisBefore << " = " <<
//                  (phisAfter - phisBefore) << std::endl;

//          debugSaveMatrix(MatNum(arma::real(g.slice(timeslice))), "gslice_old_real");
//          debugSaveMatrix(MatNum(arma::imag(g.slice(timeslice))), "gslice_old_imag");
//          g.slice(timeslice) *= arma::inv(target);
//          debugSaveMatrix(MatNum(arma::real(g.slice(timeslice))), "gslice_new_real");
//          debugSaveMatrix(MatNum(arma::imag(g.slice(timeslice))), "gslice_new_imag");

                //****
                //DEBUG
//          MatCpx gPrimeRef = g.slice(timeslice) * arma::inv(target);
                //END DEBUG
                //****


                //DEBUG
                //compare with a full-force evaluation of G*[I + Delta*(I - G)]^{-1}
//            MatCpx delta(4*N, 4*N);
//            delta.fill(cpx(0,0));
//            arma::uvec::fixed<4> idx = {site, site + N, site + 2*N, site + 3*N};
//            uint32_t i = 0;
//            for (auto col: idx) {
//                uint32_t j = 0;
//                for (auto row: idx) {
//                    delta(row, col) = deltanonzero(j, i);
//                    ++j;
//                }
//                ++i;
//            }
//            MatCpx g_new_ref = g * arma::inv(
//                    arma::eye(4*N,4*N) + delta*(arma::eye(4*N,4*N) - g));
                //END DEBUG


                //DEBUG
                //Compare green's function updated with the method of updateInSlice_woodbury
                //with this one:
//            MatCpx g_woodbury = g;        //copy
//            MatCpx::fixed<4,4> g_woodbury_sub;
//            for (uint32_t a = 0; a < 4; ++a) {
//                for (uint32_t b = 0; b < 4; ++b) {
//                    g_woodbury_sub(a,b) = g_woodbury(site + a*N, site + b*N);
//                }
//            }
//            MatCpx::fixed<4,4> M = eye4cpx + (eye4cpx - g_woodbury_sub) * deltanonzero;    //!!
//            MatCpx mat_V(4, 4*N);
//            for (uint32_t r = 0; r < 4; ++r) {
//                mat_V.row(r) = g_woodbury.row(site + r*N);
//                mat_V(r, site + r*N) -= 1.0;
//            }
//            MatCpx g_woodbury_times_mat_U(4*N, 4);
//            for (uint32_t c = 0; c < 4; ++c) {
//                g_woodbury_times_mat_U.col(c) = g_woodbury.col(site + c*N); //!!
//            }
//            g_woodbury_times_mat_U = g_woodbury_times_mat_U * deltanonzero;
//            g_woodbury += (g_woodbury_times_mat_U) * (arma::inv(M) * mat_V);
                //END DEBUG

                //DEBUG
                //Compare green's function updated with a large matrix woodbury formula
                //with this one:
//            MatCpx delta(4*N, 4*N);
//            delta.fill(cpx(0,0));
//            arma::uvec::fixed<4> idx = {site, site + N, site + 2*N, site + 3*N};
//            uint32_t i = 0;
//            for (auto col: idx) {
//                uint32_t j = 0;
//                for (auto row: idx) {
//                    delta(row, col) = deltanonzero(j, i);
//                    ++j;
//                }
//                ++i;
//            }
//            MatCpx mat_U_large = -delta;
//            MatCpx mat_V_large = arma::eye(4*N,4*N) - g;
//            MatCpx g_woodbury_large = g * (arma::eye(4*N,4*N) +
//                    mat_U_large *
//                    arma::inv(arma::eye(4*N,4*N) - mat_V_large * mat_U_large) *
//                    mat_V_large);
                //END DEBUG


                //DEBUG
                //Compare green's function updated with a reasonably sized matrix woodbury formula
                //with this one:
//            MatCpx mat_U_reas(4*N,4);
//            mat_U_reas.fill(cpx(0,0));
//            for (uint32_t k = 0; k < 4; ++k) {
//                for (uint32_t l = 0; l < 4; ++l) {
//                    mat_U_reas(site + k*N, l) = -deltanonzero(k, l);
//                }
//            }
//            MatCpx mat_V_reas(4,4*N);
//            for (uint32_t l = 0; l < 4; ++l) {
//                mat_V_reas.row(l) = arma::conv_to<MatCpx>::from((arma::eye(4*N,4*N) - g)).row(site + l*N);
//            }
//            MatCpx::fixed<4,4> g_woodbury_reas_sub;
//            for (uint32_t a = 0; a < 4; ++a) {
//                for (uint32_t b = 0; b < 4; ++b) {
//                    g_woodbury_reas_sub(a,b) = g(site + a*N, site + b*N);
//                }
//            }
//            MatCpx::fixed<4,4> M_rev = eye4cpx + (eye4cpx - g_woodbury_reas_sub) * deltanonzero;
//            //MatCpx g_woodbury_reas = g * (arma::eye(4*N,4*N) +
//            //        mat_U_reas *
//            //        arma::inv(eye4cpx - mat_V_reas * mat_U_reas) *
//            //        mat_V_reas);
//            MatCpx g_woodbury_reas = g * (arma::eye(4*N,4*N) +
//                    mat_U_reas *
//                    arma::inv(M_rev) *
//                    mat_V_reas);
                //END DEBUG


                //compensate for already included diagonal entries of I in invRows
                rows[0][site] -= 1;
                rows[1][site + pars.N] -= 1;
                if (OPDIM == 3) {
                    rows[2][site + 2*pars.N] -= 1;
                    rows[3][site + 3*pars.N] -= 1;
                }
                //compute G' = G * [I + Delta*(I - G)]^(-1) = G * [I + invRows]
                // [O(N^2)]
                MatData gTimesInvRows(MatrixSizeFactor*pars.N,
                                      MatrixSizeFactor*pars.N);
                const auto& G = g;
                for (uint32_t col = 0; col < MatrixSizeFactor*pars.N; ++col) {
                    for (uint32_t row = 0; row < MatrixSizeFactor*pars.N; ++row) {
                        gTimesInvRows(row, col) =
                            G(row, site)            * rows[0][col]
                            + G(row, site + pars.N)   * rows[1][col];
                        if (OPDIM == 3) {
                            gTimesInvRows(row, col) +=
                                G(row, site + 2*pars.N) * rows[2][col]
                                + G(row, site + 3*pars.N) * rows[3][col];
                        }
                    }
                }
                g += gTimesInvRows;

                //DEBUG
//            std::cout << "ref g_mean: " << arma::mean(arma::mean(arma::abs(((g - g_new_ref) / g)))) << "\n";
//            std::cout << "ref g_max: " << arma::max(arma::max(arma::abs(((g - g_new_ref) / g)))) << "\n";
                //compare with a full-force evaluation of G*[I + Delta*(I - G)]^{-1}

                //DEBUG
                //Compare green's function updated with an large woodbury formula
                //with this one:
//            std::cout << "woodbury large g_mean: " << arma::mean(arma::mean(arma::abs(((g - g_woodbury_large) / g)))) << "\n";
//            std::cout << "woodbury large g_max: " << arma::max(arma::max(arma::abs(((g - g_woodbury_large) / g)))) << "\n";
                //END DEBUG

                //DEBUG
                //Compare green's function updated with a reasonably sized matrix woodbury formula
                //with this one:
//            std::cout << "woodbury reas g_mean: " << arma::mean(arma::mean(arma::abs(((g - g_woodbury_reas) / g)))) << "\n";
//            std::cout << "woodbury reas g_max: " << arma::max(arma::max(arma::abs(((g - g_woodbury_reas) / g)))) << "\n";
                //END DEBUG


                //DEBUG
                //Compare green's function updated with the method of updateInSlice_woodbury
                //with this one:
//            std::cout << "woodbury g_mean: " << arma::mean(arma::mean(arma::abs(((g - g_woodbury) / g)))) << "\n";
//            std::cout << "woodbury g_max: " << arma::max(arma::max(arma::abs(((g - g_woodbury) / g)))) << "\n";
                //END DEBUG

                //****
                //DEBUG
//          std::cout << arma::max(arma::max(
//                  (arma::abs(gPrimeRef - g.slice(timeslice))))) << std::endl;
                //END DEBUG
                //****

            } // if (not pars.turnoffFermions)
        }
    }
    accratio /= num(pars.N);
    return accratio;
}


template<CheckerboardMethod CB, int OPDIM>
template<class Callable>
num DetSDW<CB, OPDIM>::updateInSlice_woodbury(uint32_t timeslice,
                                              Callable proposeLocalUpdate) {
    constexpr uint32_t MSF = MatrixSizeFactor;
    num accratio = 0.;
    for (uint32_t site = 0; site < pars.N; ++site) {
        Phi newphi;
        int32_t new_cdwl;
        Changed changed;
        std::tie(changed, newphi, new_cdwl) = proposeLocalUpdate(site, timeslice);
        if (changed == NONE) {
            //reject this change
            continue;
        }

        num probSPhi = 1.;
        if (changed == PHI) {
            num dsphi = deltaSPhi(site, timeslice, newphi);
            probSPhi = std::exp(-dsphi);
        }

        num probSFermion = 1.0;
        num prob_cdwl = 1.0;

        MatSmall delta_forsite;
        MatSmall M;
        if (not pars.turnoffFermions) {

            //delta = e^(-dtau*V_new)*e^(+dtau*V_old) - 1

            delta_forsite = get_delta_forsite(
                newphi, new_cdwl, timeslice, site);

            //Compute the 4x4 (2x2) submatrix of G that corresponds to the
            //site i g_sub = g[i::N, i::N]
            MatSmall g_sub;
            for (uint32_t a = 0; a < MSF; ++a) {
                for (uint32_t b = 0; b < MSF; ++b) {
                    g_sub(a,b) = g(site + a*pars.N, site + b*pars.N);
                }
            }

            //the determinant ratio for the spin update is given by the determinant
            //of the following matrix M
            M = smalleye + (smalleye - g_sub) * delta_forsite; // should work with new workaraound in Armadillo
            //M = (smalleye + (smalleye - g_sub) * delta_forsite).eval(); // work around Intel compiler bug
            DataType det = arma::det(M);

            // consistency check
            if (loggingParams.checkAndLogDetRatio and performedSweeps >= 10 and changed == PHI) {
                num det_abs = std::abs(det);
                num ref_det = computeGreenDetRatioFromScratch(site, timeslice, newphi);
                num diff = ref_det - det_abs;
                num reldiff = diff / ref_det;

                detRatioLogging->writeData("t=" + numToString(timeslice) + ",i=" + numToString(site) +
                                           " ref - woodbury: " +
                                           numToString(ref_det) + " - " +
                                           numToString(det_abs) + " = " +
                                           numToString(diff) + ", relative: " +
                                           numToString(reldiff));
            }


            if (OPDIM == 3) {
                probSFermion = dataReal(det);
            } else {
                //      /G 0 \             .
                //  det \0 G*/ = |det G|^2
                probSFermion = std::pow(std::abs(det), 2);
            }

            prob_cdwl = cdwl_gamma(new_cdwl) / cdwl_gamma(cdwl(site, timeslice));
        } else {
            probSFermion = 1.0;
            prob_cdwl = 1.0;
        }

        num prob = probSPhi * probSFermion * prob_cdwl;

        if (prob > 1.0 or rng.rand01() < prob) {
            //count accepted update
            accratio += 1.0;

            for (uint32_t dim = 0; dim < OPDIM; ++dim) {
                phi(site, dim, timeslice) = newphi[dim];
            }

            if (not pars.turnoffFermions) {

                //consistency check
                std::unique_ptr<MatData> ref_g;
                if (loggingParams.checkAndLogGreen and performedSweeps >= 10 and changed == PHI) {
                    ref_g = std::unique_ptr<MatData>(new MatData(computeGreenFromScratch(site, timeslice, newphi)));
                }

                cdwl(site, timeslice) = new_cdwl;
                updateCoshSinhTerms(site, timeslice);

                //update g

                MatData mat_V(MSF, MSF*pars.N);
                for (uint32_t r = 0; r < MSF; ++r) {
                    mat_V.row(r) = g.row(site + r*pars.N);
                    mat_V(r, site + r*pars.N) -= 1.0;
                }

                //TODO: is it a good idea to do this copy? or would it be better to
                //compute the product directly with a non-contiguous subview?
                MatData g_times_mat_U(MSF*pars.N, MSF);
                for (uint32_t c = 0; c < MSF; ++c) {
                    g_times_mat_U.col(c) = g.col(site + c*pars.N);
                }
                g_times_mat_U = g_times_mat_U * delta_forsite;

                g += (g_times_mat_U) * (arma::inv(M) * mat_V);

                //consistency check
                if (loggingParams.checkAndLogGreen and performedSweeps >= 10 and changed == PHI) {
                    MatNum abs_diff = arma::abs(g - *ref_g);
                    num mean_rel_abs_diff = arma::mean(arma::mean(abs_diff / arma::abs(*ref_g)));
                    num max_diff = arma::max(arma::max(abs_diff));
                    num mean_diff = arma::mean(arma::mean(abs_diff));
                    greenLogging->writeData("t=" + numToString(timeslice) + ",i=" + numToString(site) +
                                            " ref - woodbury: " +
                                            "max diff: " + numToString(max_diff) +
                                            " mean diff: " + numToString(mean_diff) +
                                            " mean rel diff: " + numToString(mean_rel_abs_diff));
                    delete ref_g.release();
                }
            }
        }
    }
    accratio /= num(pars.N);
    return accratio;
}

template<CheckerboardMethod CB, int OPDIM>
template<class Callable>
num DetSDW<CB, OPDIM>::updateInSlice_delayed(uint32_t timeslice, Callable proposeLocalUpdate) {
    assert(not pars.turnoffFermions); // makes no sense to use delayed updates

    constexpr auto MSF = MatrixSizeFactor;
    const auto N = pars.N;
    num accratio = 0.;

    auto getX = [this, MSF](uint32_t step) {
        return dud.X.cols(MSF*step, MSF*step + MSF-1);
    };
    auto getY = [this, MSF](uint32_t step) {
        return dud.Y.rows(MSF*step, MSF*step + MSF-1);
    };

    // these two helper functions return submatrices containing just 2
    // (OPDIM == 1 or 2) or 4 (OPDIM == 3) rows or columns
    auto takesomerows = [this, N, MSF](MatData& target, const MatData& source, uint32_t for_site) {
        for (uint32_t r = 0; r < MSF; ++r) {
            target.row(r) = source.row(for_site + r*N);
        }
    };
    auto takesomecols = [this, N, MSF](MatData& target, const MatData& source, uint32_t for_site) {
        for (uint32_t c = 0; c < MSF; ++c) {
            target.col(c) = source.col(for_site + c*N);
        }
    };

    uint32_t site = 0;
    while (site < N) {
        uint32_t delayStepsNow = std::min(pars.delaySteps, N - site);
        dud.X.set_size(MSF*N, MSF*delayStepsNow);
        dud.Y.set_size(MSF*delayStepsNow, MSF*N);
        uint32_t j = 0;
        while (j < delayStepsNow and site < N) {
            Phi newphi;
            int32_t new_cdwl;
            Changed changed;
            std::tie(changed, newphi, new_cdwl) = proposeLocalUpdate(site, timeslice);

            if (changed != NONE) {
                //local update is not rejected immeadiately, figure out if we should accept it
            	num probSPhi = 1.0;
            	if (changed == PHI) {
                    num dsphi = deltaSPhi(site, timeslice, newphi);
                    probSPhi = std::exp(-dsphi);
            	}

                MatSmall delta_forsite =
                    get_delta_forsite(newphi, new_cdwl, timeslice, site);

                takesomerows(dud.Rj, g, site);
                for (uint32_t l = 0; l < j; ++l) {
                    takesomerows(dud.tempBlock, getX(l), site);
                    dud.Rj += dud.tempBlock * getY(l);
                }

                takesomecols(dud.Sj, dud.Rj, site);

                dud.Mj = smalleye - dud.Sj * delta_forsite + delta_forsite; // should work with new workaround in Armadillo
                // dud.Mj = (smalleye - dud.Sj * delta_forsite + delta_forsite).eval(); // work around Intel compiler bug

                DataType det = arma::det(dud.Mj);

                // consistency check
                if (loggingParams.checkAndLogDetRatio and performedSweeps >= 10 and changed == PHI) {
                    num det_abs = std::abs(det);
                    num ref_det = computeGreenDetRatioFromScratch(site, timeslice, newphi);
                    num diff = ref_det - det_abs;
                    num reldiff = diff / ref_det;

                    detRatioLogging->writeData("t=" + numToString(timeslice) + ",i=" + numToString(site) +
                                               " ref - delayed: " +
                                               numToString(ref_det) + " - " +
                                               numToString(det_abs) + " = " +
                                               numToString(diff) + ", relative: " +
                                               numToString(reldiff));
                }

                num probSFermion;
                if (OPDIM == 3) {
                    probSFermion = dataReal(det);
                } else {
                    //      /G 0 \             .
                    //  det \0 G*/ = |det G|^2
                    probSFermion = std::pow(std::abs(det), 2);
                }

                num prob_cdwl = cdwl_gamma(new_cdwl) / cdwl_gamma(cdwl(site, timeslice));

                num prob = probSPhi * probSFermion * prob_cdwl;
                if (prob > 1.0 or rng.rand01() < prob) {
                    //count accepted update
                    accratio += 1.0;
                    for (uint32_t dim = 0; dim < OPDIM; ++dim) {
                        phi(site, dim, timeslice) = newphi[dim];
                    }
                    cdwl(site, timeslice) = new_cdwl;
                    updateCoshSinhTerms(site, timeslice);

                    //we need Cj only to update X
                    takesomecols(dud.Cj, g, site);
                    for (uint32_t l = 0; l < j; ++l) {
                        takesomecols(dud.tempBlock, getY(l), site);
                        dud.Cj += getX(l) * dud.tempBlock;
                    }
                    //Rj is now Rj - \Id_j, for updating Y
                    for (uint32_t rc = 0; rc < MSF; ++rc) {
                        uint32_t entry = site + rc * N;
                        dud.Rj(rc, entry) -= DataType(1);
                    }

                    //update X and Y
                    getX(j) = dud.Cj * delta_forsite;
                    getY(j) = arma::inv(dud.Mj) * dud.Rj;
                    //count successful delayed update
                    j += 1;
                }
            }
            ++site;
        }
        if (j > 0) {
            if (j < delayStepsNow) {
                dud.X.resize(MSF*N, MSF*j);
                dud.Y.resize(MSF*j, MSF*N);
            }

            //consistency check
            std::unique_ptr<MatData> ref_g;
            if (loggingParams.checkAndLogGreen and performedSweeps >= 10) {
                ref_g = std::unique_ptr<MatData>(new MatData(computeGreenFromScratch(timeslice, phi)));
            }

            //carry out the delayed updates of the Green's function
            g += dud.X*dud.Y;

            //consistency check
            if (loggingParams.checkAndLogGreen and performedSweeps >= 10) {
                MatNum abs_diff = arma::abs(g - *ref_g);
                num mean_rel_abs_diff = arma::mean(arma::mean(abs_diff / arma::abs(*ref_g)));
                num max_diff = arma::max(arma::max(abs_diff));
                num mean_diff = arma::mean(arma::mean(abs_diff));
                greenLogging->writeData("t=" + numToString(timeslice) + ",i=" + numToString(site) +
                                        " ref - delayed: " +
                                        "max diff: " + numToString(max_diff) +
                                        " mean diff: " + numToString(mean_diff) +
                                        " mean rel diff: " + numToString(mean_rel_abs_diff));
                delete ref_g.release();
            }
        }
    }
    accratio /= num(N);
    return accratio;
}

template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatSmall
DetSDW<CB, OPDIM>::get_delta_forsite(
    Phi newphi, int32_t new_cdwl, uint32_t timeslice, uint32_t site) {
    //delta = e^(-dtau*V_new)*e^(+dtau*V_old) - 1

    //compute non-zero elements of delta
    // delta_forsite is \Delta^i from the notes
    //
    //evMatrix(): yield a 4x4 matrix containing the entries for the
    //current lattice site and time slice of e^(sign*dtau*V)
    auto evMatrix = [this](
        int sign,
        Phi kphi,
        num kcoshTermPhi, num ksinhTermPhi,
        num kcoshTermCDWl, num ksinhTermCDWl) -> MatSmall
        {
            MatSmall ev;

            typedef DetSDW<CB,OPDIM> D;

            D::setReal(ev(0,0), kcoshTermPhi * kcoshTermCDWl - sign * ksinhTermCDWl);
            D::setReal(ev(1,1), kcoshTermPhi * kcoshTermCDWl + sign * ksinhTermCDWl);
//        ev_real(0,1) = ev_real(1,0) = ev_real(2,3) = ev_real(3,2) = 0;  //zeros
            D::setReal(ev(0,1), sign * kphi[0] * ksinhTermPhi * kcoshTermCDWl);
            D::setReal(ev(1,0), sign * kphi[0] * ksinhTermPhi * kcoshTermCDWl);

            if (OPDIM == 3) {
                D::setReal(ev(2,2), std::real(ev(0,0)));
                D::setReal(ev(3,3), std::real(ev(1,1)));
                D::setReal(ev(0,3), sign * kphi[2] * ksinhTermPhi * kcoshTermCDWl);
                D::setReal(ev(3,0), std::real(ev(0,3)));
                D::setReal(ev(3,2), std::real(ev(0,1)));
                D::setReal(ev(2,3), std::real(ev(1,0)));
                D::setReal(ev(2,1), -sign * kphi[2] * ksinhTermPhi * kcoshTermCDWl);
                D::setReal(ev(1,2), std::real(ev(2,1)));
            }

            if (OPDIM > 1) {
                MatNum::fixed<MatrixSizeFactor, MatrixSizeFactor> ev_imag;
                ev_imag.zeros();
                ev_imag(0,1) = -sign * kphi[1] * ksinhTermPhi * kcoshTermCDWl;
                ev_imag(1,0) =  sign * kphi[1] * ksinhTermPhi * kcoshTermCDWl;

                if (OPDIM == 3) {
                    ev_imag(2,3) =  sign * kphi[1] * ksinhTermPhi * kcoshTermCDWl;
                    ev_imag(3,2) = -sign * kphi[1] * ksinhTermPhi * kcoshTermCDWl;
                }
                ev.set_imag(ev_imag);
            }

            return ev;
        };
    MatSmall evOld = ( pars.cdwU ?
                       evMatrix(
                           +1,
                           getPhi(site, timeslice),
                           coshTermPhi(site, timeslice), sinhTermPhi(site, timeslice),
                           coshTermCDWl(site, timeslice), sinhTermCDWl(site, timeslice)
                           ) :
                       evMatrix(
                           +1,
                           getPhi(site, timeslice),
                           coshTermPhi(site, timeslice), sinhTermPhi(site, timeslice),
                           1.0, 0.0
                           ) );
    // //DEBUG
    // VecNum debug_phi0 = phi0.col(timeslice);
    // VecNum debug_phi1 = phi1.col(timeslice);
    // VecNum debug_phi2 = phi2.col(timeslice);
    // VecInt debug_cdwl = cdwl.col(timeslice);
    // MatData evOld_big = computePotentialExponential(
    //     +1, debug_phi0, debug_phi1, debug_phi2, debug_cdwl);
    // arma::uvec indices;
    // indices << site << site+N << site+2*N << site+3*N;
    // MatData::fixed<MSF,MSF> evOld_big_sub = evOld_big.submat(indices, indices);
    // print_matrix_diff(evOld, evOld_big_sub, "evOld diff");
    // //DEBUG -- OK

    num coshTermPhi_new, sinhTermPhi_new, coshTermCDWl_new, sinhTermCDWl_new;
    std::tie(coshTermPhi_new, sinhTermPhi_new) = getCoshSinhTermPhi(newphi);
    if (pars.cdwU) {
        std::tie(coshTermCDWl_new, sinhTermCDWl_new) = getCoshSinhTermCDWl(new_cdwl);
    }
    MatSmall emvNew = ( pars.cdwU ?
                        evMatrix(
                            -1,
                            newphi,
                            coshTermPhi_new, sinhTermPhi_new,
                            coshTermCDWl_new, sinhTermCDWl_new
                            ) :
                        evMatrix(
                            -1,
                            newphi,
                            coshTermPhi_new, sinhTermPhi_new,
                            1.0, 0.0
                            ) );

    // //DEBUG
    // debug_phi0[site] = newphi[0];
    // debug_phi1[site] = newphi[1];
    // debug_phi2[site] = newphi[2];
    // debug_cdwl[site] = new_cdwl;
    // MatData emvNew_big = computePotentialExponential(
    //     -1, debug_phi0, debug_phi1, debug_phi2, debug_cdwl);
    // MatData::fixed<MSF,MSF> emvNew_big_sub = emvNew_big.submat(indices, indices);
    // print_matrix_diff(emvNew, emvNew_big_sub, "emvNew diff");
    // //DEBUG -- OK now

    MatSmall delta_forsite = emvNew * evOld;
    delta_forsite.diag() -= DataType(1);
    return delta_forsite;
}



template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateInSliceThermalization(uint32_t timeslice) {
    updateInSlice(timeslice);

    if (pars.phiFixed) return;

    enum { ADAPT_BOX, ADAPT_ROTATE, ADAPT_SCALE } adapting_what = ADAPT_BOX;
    if (pars.spinProposalMethod == SpinProposalMethod_Type::BOX) {
        adapting_what = ADAPT_BOX;
    } else if (pars.spinProposalMethod == SpinProposalMethod_Type::ROTATE_THEN_SCALE) {
        // the following needs to match the order of moves as
        // used in updateInSlice()
        if (performedSweeps % 2 == 0) {
            // we did rotate moves last
            adapting_what = ADAPT_ROTATE;
        } else {
            // we did scale moves last
            adapting_what = ADAPT_SCALE;
        }
    } else if (pars.spinProposalMethod == SpinProposalMethod_Type::ROTATE_AND_SCALE) {
        // after every interval of AccRatioAdjustmentSamples we alternate between
        // adjusting the parameter for the rotate and scale moves
        if (performedSweeps % (2 * ad.AccRatioAdjustmentSamples) < ad.AccRatioAdjustmentSamples) {
            adapting_what = ADAPT_ROTATE;
        } else {
            adapting_what = ADAPT_SCALE;
        }
    }
    //Hold a reference to the accRatioLocal_*_RA we currently need
    std::reference_wrapper<RunningAverage> ra(ad.accRatioLocal_box_RA);
    switch (adapting_what) {
    case ADAPT_BOX: ra = std::ref(ad.accRatioLocal_box_RA); break;
    case ADAPT_ROTATE: ra = std::ref(ad.accRatioLocal_rotate_RA); break;
    case ADAPT_SCALE: ra = std::ref(ad.accRatioLocal_scale_RA); break;
    }

    ra.get().addValue(ad.lastAccRatioLocal_phi);
    using std::cout;
    if (unsigned(ra.get().getSamplesAdded()) % ad.AccRatioAdjustmentSamples == 0) {
        num avgAccRatio = ra.get().get();
        switch (adapting_what) {
        case ADAPT_BOX:
            if (avgAccRatio < ad.targetAccRatioLocal_phi) {
                ad.phiDelta *= ad.phiDeltaShrinkFactor;
            } else if (avgAccRatio > ad.targetAccRatioLocal_phi) {
                ad.phiDelta *= ad.phiDeltaGrowFactor;
            }
//            cout << "box, acc: " << avgAccRatio << ", ad.phiDelta = " << ad.phiDelta << '\n';
            break;
        case ADAPT_ROTATE:
            // angleDelta <=> cosine of spherical angle theta
            // reducing angleDelta <=> opening up the angle <=> reducing acceptance ratio
            if (avgAccRatio < ad.targetAccRatioLocal_phi and ad.angleDelta < ad.MaxAngleDelta) {
                ad.curminAngleDelta = ad.angleDelta;
                ad.angleDelta += (ad.curmaxAngleDelta - ad.angleDelta) / 2;
            }
            else if (avgAccRatio > ad.targetAccRatioLocal_phi and ad.angleDelta > ad.MinAngleDelta) {
                ad.curmaxAngleDelta = ad.angleDelta;
                ad.angleDelta -= (ad.angleDelta - ad.curminAngleDelta) / 2;
            }
//            cout << "rotate, acc: " << avgAccRatio << ", angleDelta = " << ad.angleDelta << '\n';
            break;
        case ADAPT_SCALE:
            if (not pars.adaptScaleVariance) {
                //do not change scaleDelta at all
                break;
            }
            // scaleDelta <=> width of gaussian distribution to select new radius
            // reducing scaleDelta <=> increasing acceptance ratio
            if (avgAccRatio > ad.targetAccRatioLocal_phi and ad.scaleDelta < ad.MaxScaleDelta) {
                //I'd say it's unlikely to get such big acceptance ratios with such a wide gaussian
                ad.curminScaleDelta = ad.scaleDelta;
                ad.scaleDelta += (ad.curmaxScaleDelta - ad.scaleDelta) / 2;
            }
            else if (avgAccRatio > ad.targetAccRatioLocal_phi and ad.scaleDelta > ad.MinScaleDelta) {
                ad.curmaxScaleDelta = ad.scaleDelta;
                ad.scaleDelta -= (ad.scaleDelta - ad.curminScaleDelta) / 2;
            }
//            cout << "scale, acc: " << avgAccRatio << ", scaleDelta = " << ad.scaleDelta << '\n';
            break;
        }
    }
}



template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::updateInSlice_overRelaxation(uint32_t timeslice) {
    assert(pars.turnoffFermions); // makes no sense with fermions

    const auto N = pars.N;
    const auto dtau = pars.dtau;
    const auto c = pars.c;

    // pick random sites
    for (uint32_t site_counter = 0; site_counter < N; ++site_counter) {
        uint32_t site = rng.randInt(0, N-1);

#ifdef MAX_DEBUG
        num old_action = phiAction();
#endif

        Phi old_phi = getPhi(site, timeslice);

        Phi b_eff = (1.0 / (c * c * dtau)) * ( getPhi(site, timeNeigh(ChainDir::MINUS, timeslice)) +
                                               getPhi(site, timeNeigh(ChainDir::PLUS,  timeslice)) );       
        for (auto iter = spaceNeigh.beginNeighbors(site); iter != spaceNeigh.endNeighbors(site); ++iter) {
            uint32_t neigh_site = *iter;
            b_eff += dtau * getPhi(neigh_site, timeslice);
        }

        Phi new_phi = -old_phi + (2. * arma::dot(old_phi, b_eff) / arma::dot(b_eff, b_eff)) * b_eff;

        // this does not change the bosonic action
        setPhi(site, timeslice, new_phi);

#ifdef MAX_DEBUG
        num new_action = phiAction();
        assert( std::abs(old_action - new_action) < 1E-10 );
#endif
    }
}


template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::overRelaxationSweep() {
    timing.start("sdw-overRelaxationSweep");

    assert(pars.turnoffFermions); // makes no sense with fermions

    const auto N = pars.N;
    const auto dtau = pars.dtau;
    const auto m = pars.m;    
    const auto c = pars.c;

    for (uint32_t timeslice = 1; timeslice <= m; ++timeslice) {
        for (uint32_t site = 0; site < N; ++site) {
#ifdef MAX_DEBUG
            num old_action = phiAction();
#endif

            Phi old_phi = getPhi(site, timeslice);

            Phi b_eff = (1.0 / (c * c * dtau)) * ( getPhi(site, timeNeigh(ChainDir::MINUS, timeslice)) +
                                                   getPhi(site, timeNeigh(ChainDir::PLUS,  timeslice)) );       
            for (auto iter = spaceNeigh.beginNeighbors(site); iter != spaceNeigh.endNeighbors(site); ++iter) {
                uint32_t neigh_site = *iter;
                b_eff += dtau * getPhi(neigh_site, timeslice);
            }

            Phi new_phi = -old_phi + (2. * arma::dot(old_phi, b_eff) / arma::dot(b_eff, b_eff)) * b_eff;

            // this does not change the bosonic action
            setPhi(site, timeslice, new_phi);

#ifdef MAX_DEBUG
            num new_action = phiAction();
            assert( std::abs(old_action - new_action) < 1E-10 );
#endif
            
        }
    }
    
    timing.stop("sdw-overRelaxationSweep");
}


template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::globalMove() {
    timing.start("sdw-globalMove");
    
    //This is called before the sweep, i.e. before performedSweeps is updated
    if (not pars.phiFixed and ((performedSweeps) % pars.globalUpdateInterval == 0)) {
        //the current sweep count is a multiple of globalMoveInterval
        if (pars.globalShift) {
            attemptGlobalShiftMove();
        }
        if (pars.wolffClusterUpdate) {
            attemptWolffClusterUpdate();
        }
        if (pars.wolffClusterShiftUpdate) {
            attemptWolffClusterShiftUpdate();
        }
    }
    if (pars.turnoffFermions and pars.overRelaxation) {
        for (uint32_t count = 0; count < pars.repeatOverRelaxation; ++count) {
            overRelaxationSweep();
        }
        
    }    
    
    timing.stop("sdw-globalMove");
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::attemptWolffClusterUpdate() {
    using std::exp; using std::cout;
    timing.start("sdw-attemptWolffClusterUpdate");

    //UdV storage must be valid! attemptGlobalShiftMove() needs to be called
    //after sweepUp.
    if (not pars.turnoffFermions) {
        assert(currentTimeslice == pars.m);
    }

    // The product of the singular values of g^{-1} is equal to the
    // absolute value of its determinant.  Don't compute the whole
    // product explicitly because it contains both very large and very
    // small numbers --> over/underflows!  Instead use the fact that
    // the SV's are sorted by magnitude and compare them term by term
    // with the SV's of the updated inverse Green's function.
    // Do this computation logarithmically for stability, else scales
    // would be mixed even if ordered.

    globalMoveStoreBackups();

    const VecNum& old_g_inv_sv = gmd.g_inv_sv; // backed up: old singular values

    std::vector<uint32_t> cluster_sizes;
    for (uint32_t c = 0; c < pars.repeatWolffPerSweep; ++c) {
        uint32_t cluster_size = buildAndFlipCluster(true); // need to update cosh/sinh terms
        cluster_sizes.push_back(cluster_size);
    }

    num prob_fermion = 1.0;

    if (not pars.turnoffFermions) {

        //recompute Green's function
        setupUdVStorage_and_calculateGreen();  //    g = greenFromEye_and_UdV((*UdVStorage)[0][n]);

        // compute transition probability.
        // avoid mixing large and small numbers -> use logarithms!

        uint32_t count = MatrixSizeFactor * pars.N;
        num log_prob = 0.;
        for (uint32_t j = 0; j < count; ++j) {
            // log of g_inv_sv[j] / old_g_inv_sv[j]        {   g ~ [weight]^-1 --> g^{-1} ~ [weight]   }
            num log_diff = std::log(g_inv_sv[j]) - std::log(old_g_inv_sv[j]);
            log_prob += log_diff;
        }
        prob_fermion = std::exp(log_prob);


        if (OPDIM < 3) {
            //      /G 0 \              .
            //  det \0 G*/ = |det G|^2
            prob_fermion = std::pow(prob_fermion, 2);
        }

    }

//    std::cout << "Cluster: " << cluster_size << "  " << prob_fermion << "\n";

    us.attemptedWolffClusterUpdates += 1;
    if (prob_fermion >= 1. or rng.rand01() < prob_fermion) {
        //update accepted
        us.acceptedWolffClusterUpdates += 1;
        for (uint32_t cluster_size : cluster_sizes) {
            us.addedWolffClusterSize += num(cluster_size);
        }
        //std::cout << "accept cluster\n";
    } else {
        //update rejected, restore previous state
        globalMoveRestoreBackups();
        //std::cout << "reject cluster\n";
    }

    timing.stop("sdw-attemptWolffClusterUpdate");
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::attemptGlobalShiftMove() {
    timing.start("sdw-attemptGlobalShiftMove");

    // compute current weight
    num old_scalar_action = phiAction();
    //UdV storage must be valid! attemptGlobalShiftMove() needs to be called
    //after sweepUp.
    if (not pars.turnoffFermions) {
        assert(currentTimeslice == pars.m);
    }

    // The product of the singular values of g^{-1} is equal to the
    // absolute value of its determinant.  Don't compute the whole
    // product explicitly because it contains both very large and very
    // small numbers --> over/underflows!  Instead use the fact that
    // the SV's are sorted by magnitude and compare them term by term
    // with the SV's of the updated inverse Green's function.
    // Do this computation logarithmically for stability, else scales
    // would be mixed even if ordered.

    globalMoveStoreBackups();

    const VecNum& old_g_inv_sv = gmd.g_inv_sv; // backed up: old singular values

    // shift fields by a random, constant displacement
    addGlobalRandomDisplacement();

    if (not pars.turnoffFermions) {

        updateCoshSinhTermsPhi();

        //recompute Green's function and its singular values
        setupUdVStorage_and_calculateGreen();

    }

    //compute new weight, transition probability
    num new_scalar_action = phiAction();

    num prob_scalar = std::exp(-(new_scalar_action - old_scalar_action));

    num prob_fermion = 1.0;

    if (not pars.turnoffFermions) {

        // compute transition probability.
        // avoid mixing large and small numbers -> use logarithms!

        uint32_t count = MatrixSizeFactor * pars.N;
        num log_prob = 0.;
        for (uint32_t j = 0; j < count; ++j) {
            // log of g_inv_sv[j] / old_g_inv_sv[j]        {   g ~ [weight]^-1 --> g^{-1} ~ [weight]   }
            num log_diff = std::log(g_inv_sv[j]) - std::log(old_g_inv_sv[j]);
            log_prob += log_diff;
        }
        prob_fermion = std::exp(log_prob);


        if (OPDIM < 3) {
            //      /G 0 \              .
            //  det \0 G*/ = |det G|^2
            prob_fermion = std::pow(prob_fermion, 2);
        }

    }

    num prob = prob_scalar * prob_fermion;

    us.attemptedGlobalShifts += 1;
    if (prob >= 1. or rng.rand01() < prob) {
        //update accepted
        us.acceptedGlobalShifts += 1;
        // std::cout << "\naccept globalShift\n\n";
    } else {
        //update rejected, restore previous state
        globalMoveRestoreBackups();
        // std::cout << "\nreject globalShift\n\n";
    }

    timing.stop("sdw-attemptGlobalShiftMove");
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::attemptWolffClusterShiftUpdate() {
    timing.start("sdw-attemptWolffClusterShiftMove");

    //UdV storage must be valid! attemptGlobalShiftMove() needs to be called
    //after sweepUp.

    if (not pars.turnoffFermions) {
        assert(currentTimeslice == pars.m);
    }

    // The product of the singular values of g^{-1} is equal to the
    // absolute value of its determinant.  Don't compute the whole
    // product explicitly because it contains both very large and very
    // small numbers --> over/underflows!  Instead use the fact that
    // the SV's are sorted by magnitude and compare them term by term
    // with the SV's of the updated inverse Green's function.
    // Do this computation logarithmically for stability, else scales
    // would be mixed even if ordered.

    if (not pars.turnoffFermions) {
        // in case the move is rejected finally due to the fermion
        // determinant, we need to restore the situation before the
        // cluster flips
        globalMoveStoreBackups();
    }

    const VecNum& old_g_inv_sv = gmd.g_inv_sv; // backed up: old singular values [only used with fermions turned on]

    std::vector<uint32_t> cluster_sizes;
    for (uint32_t c = 0; c < pars.repeatWolffPerSweep; ++c) {
        uint32_t cluster_size = buildAndFlipCluster(false);
        cluster_sizes.push_back(cluster_size);
    }

    if (pars.turnoffFermions) {
        // in case the global shift move is rejected for a purely
        // bosonic model, we need to restore the situation after the
        // cluster flips
        globalMoveStoreBackups();
    }

    // compute current bosonic weight [after having flipped the cluster]
    num old_scalar_action = phiAction();

    addGlobalRandomDisplacement();

    //compute new weight
    num new_scalar_action = phiAction();
    //compute transition probability
    num prob_scalar = std::exp(-(new_scalar_action - old_scalar_action));

    num prob_fermion = 1.0;

    if (not pars.turnoffFermions) {

        updateCoshSinhTermsPhi();

        //recompute Green's function
        setupUdVStorage_and_calculateGreen();

        // compute transition probability.
        // avoid mixing large and small numbers -> use logarithms!

        uint32_t count = MatrixSizeFactor * pars.N;
        num log_prob = 0.;
        for (uint32_t j = 0; j < count; ++j) {
            // log of g_inv_sv[j] / old_g_inv_sv[j]        {   g ~ [weight]^-1 --> g^{-1} ~ [weight]   }
            num log_diff = std::log(g_inv_sv[j]) - std::log(old_g_inv_sv[j]);
            log_prob += log_diff;
        }
        prob_fermion = std::exp(log_prob);


        if (OPDIM < 3) {
            //      /G 0 \              .
            //  det \0 G*/ = |det G|^2
            prob_fermion = std::pow(prob_fermion, 2);
        }

    }

    num prob = prob_scalar * prob_fermion;

    // std::cout << "Shift + Cluster: " << cluster_size << "\n";
    // std::cout << prob_scalar << "  " << prob_fermion << "\n";

    us.attemptedWolffClusterShiftUpdates += 1;
    if (prob >= 1. or rng.rand01() < prob) {
        //update accepted
        us.acceptedWolffClusterShiftUpdates += 1;
        for (uint32_t cluster_size : cluster_sizes) {
            us.addedWolffClusterSize += num(cluster_size);
        }
        //std::cout << "accept cluster and shift\n";
    } else {
        //update rejected, restore previous state
        globalMoveRestoreBackups();
        //std::cout << "reject cluster and shift\n";
    }

    timing.stop("sdw-attemptWolffClusterShiftMove");
}

//helper functions for global updates:

// works directly on phi0,phi1,phi2:
template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::addGlobalRandomDisplacement() {
    // shift fields by a random, constant displacement
    for (uint32_t dim = 0; dim < OPDIM; ++dim) {
        num r = rng.randRange(-ad.phiDelta, +ad.phiDelta);
        phi( arma::span::all,
             arma::span(dim, dim),
             arma::span::all       ) += r;
    }
}


template<int OPDIM>
struct randomDirection {
    static VecNum::fixed<OPDIM> give(RngWrapper& rng) {
        (void) rng;
        static_assert(OPDIM == 1 or OPDIM == 2 or OPDIM == 3,
                      "unsupported OPDIM");
    }
};
template<>
struct randomDirection<1> {
    static VecNum::fixed<1> give(RngWrapper& rng) {
        VecNum::fixed<1> isingDir;
        if (rng.rand01() <= 0.5) {
            isingDir[0] = -1.0;
        } else {
            isingDir[0] = +1.0;
        }
        return isingDir;
    }
};
template<>
struct randomDirection<2> {
    static VecNum::fixed<2> give(RngWrapper& rng) {
        VecNum::fixed<2> circleDir;
        std::tie(circleDir[0], circleDir[1]) = rng.randPointOnCircle();
        return circleDir;
    }
};
template<>
struct randomDirection<3> {
    static VecNum::fixed<3> give(RngWrapper& rng) {
        VecNum::fixed<3> sphereDir;
        std::tie(sphereDir[0], sphereDir[1], sphereDir[2]) = rng.randPointOnSphere();
        return sphereDir;
    }
};

template<CheckerboardMethod CB, int OPDIM>
uint32_t DetSDW<CB, OPDIM>::buildAndFlipCluster(bool updateCoshSinh) {
    // choose random direction
    Phi rd = randomDirection<OPDIM>::give(rng);

    auto flippedPhi = [&](uint32_t site, uint32_t timeslice) -> Phi {
        // phi -> phi - 2* (phi . r) * r
        Phi phi = this->getPhi(site, timeslice);
        return phi - 2. * arma::dot(phi, rd) * rd;
    };
    auto projectedPhi = [&](uint32_t site, uint32_t timeslice) -> num {
        return arma::dot(this->getPhi(site,timeslice), rd);
    };
    auto setPhi = [&](uint32_t site, uint32_t timeslice, Phi this_phi) -> void {
        for (uint32_t dim = 0; dim < OPDIM; ++dim) {
            phi(site, dim, timeslice) = this_phi[dim];
        }
        if (updateCoshSinh) {
            this->updateCoshSinhTermsPhi(site, timeslice);
        }
    };
    auto flipPhi = [&](uint32_t site, uint32_t timeslice) -> void {
        // phi -> phi - 2* (phi . rd) * rd
        setPhi(site, timeslice, flippedPhi(site, timeslice));
    };

    // construct cluster
    gmd.visited.zeros(pars.N, pars.m+1);
    typedef typename GlobalMoveData::SpaceTimeIndex STI;
    //next_sites contains the sites for which we still need to check the neighbors
    gmd.next_sites = std::stack<STI>();

    // cluster seed:
    uint32_t timeslice = uint32_t(rng.randInt(1, (int)pars.m));
    uint32_t site = uint32_t(rng.randInt(0, (int)pars.N-1));
    flipPhi(site, timeslice);
    gmd.visited(site, timeslice) = 1;
    gmd.next_sites.push( STI(site, timeslice) );
    uint32_t cluster_size = 1;
    do {
        std::tie(site, timeslice) = gmd.next_sites.top();
        gmd.next_sites.pop();
        // std::cout << site << "," << timeslice << "  ";

        // probability to add neighbors to cluster:
        // p = 1. - exp( min[0, bond_arg] )

        // neigboring in space, equal time
        for (auto site_neigh_iter = spaceNeigh.beginNeighbors(site);
             site_neigh_iter != spaceNeigh.endNeighbors(site);
             ++site_neigh_iter) {
            uint32_t neigh_site = *site_neigh_iter;
            if (not gmd.visited(neigh_site, timeslice)) {
                num bond_arg = 2.* pars.dtau * projectedPhi(site, timeslice)
                    * projectedPhi(neigh_site, timeslice);
                if (bond_arg < 0 and rng.rand01() <= (1. - exp(bond_arg))) {
                    flipPhi(neigh_site, timeslice);
                    gmd.visited(neigh_site, timeslice) = 1;
                    gmd.next_sites.push( STI(neigh_site, timeslice) );
                    ++cluster_size;
                }
            }
        }
        //neighboring in time, equal space
        uint32_t time_neighbors[] = { timeNeigh(ChainDir::PLUS, timeslice), timeNeigh(ChainDir::MINUS, timeslice) };
        for (uint neigh_time : time_neighbors) {
            if (not gmd.visited(site, neigh_time)) {
                num bond_arg = (2. / pars.dtau) * projectedPhi(site, timeslice)
                    * projectedPhi(site, neigh_time);
                if (bond_arg < 0 and rng.rand01() <= (1. - exp(bond_arg))) {
                    flipPhi(site, neigh_time);
                    gmd.visited(site, neigh_time) = 1;
                    gmd.next_sites.push( STI(site, neigh_time) );
                    ++cluster_size;
                }
            }
        }
    } while (not gmd.next_sites.empty());

    return cluster_size;
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::globalMoveStoreBackups() {
    // Backup phi, Green's function and UdV-storage.  For Quantities
    // which are recomputed entirely in each global update, we just
    // swap the contents.
    gmd.phi = phi;

    if (not pars.turnoffFermions) {

        gmd.coshTermPhi = coshTermPhi;
        gmd.sinhTermPhi = sinhTermPhi;
        gmd.g.swap(g);
        gmd.g_inv_sv.swap(g_inv_sv);
        gmd.UdVStorage.swap(UdVStorage);

    }

}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::globalMoveRestoreBackups() {
    phi.swap(gmd.phi);

    if (not pars.turnoffFermions) {

        coshTermPhi.swap(gmd.coshTermPhi);
        sinhTermPhi.swap(gmd.sinhTermPhi);
        g.swap(gmd.g);
        g_inv_sv.swap(gmd.g_inv_sv);
        UdVStorage.swap(gmd.UdVStorage);

    }
}


template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::changedPhiInt
DetSDW<CB, OPDIM>::proposeNewPhiBox(uint32_t site, uint32_t timeslice) {
    Phi phi = getPhi(site, timeslice);

    for (auto& phi_comp: phi) {
        num r = rng.randRange(-ad.phiDelta, +ad.phiDelta);
        phi_comp += r;
    }

    return std::make_tuple(PHI, phi, cdwl(site,timeslice));
}


template<int OPDIM>
struct proposeRandomRotatedVector {
    static VecNum::fixed<OPDIM> give(RngWrapper& rng, num angleDelta, VecNum::fixed<OPDIM> oldvec) {
        (void) rng; (void)angleDelta; (void)oldvec;
        throw_GeneralError("proposeRandomRotatedVector is only supported for the O(3) model");
        return oldvec;
    }
};
template<>
struct proposeRandomRotatedVector<3> {
    static VecNum::fixed<3> give(RngWrapper& rng, num angleDelta, VecNum::fixed<3> vec) {
        using std::pow; using std::sqrt; using std::cos; using std::sin;
        //old orientation
        num x = vec[0];
        num y = vec[1];
        num z = vec[2];
        //squares:
        num x2 = pow(x, 2.0);
        num y2 = pow(y, 2.0);
        num z2 = pow(z, 2.0);
        //squared length
        num r2 = x2 + y2 + z2;
        //length
        num r = sqrt(r2);

        //new angular coordinates:
        num cosTheta = rng.rand01() * (1.0 - angleDelta) + angleDelta;     // \in [angleDelta, 1.0] since rand() \in [0, 1.0]
        num phi = rng.rand01() * 2.0 * M_PI;
        num sinTheta = sqrt(1.0 - pow(cosTheta, 2.0));
        num cosPhi = cos(phi);
        num sinPhi = sin(phi);

        // To find the new orientation, we first consider the normalized old spin
        num x2n = x2 / r2;
        num y2n = y2 / r2;
        num xn = x / r;
        num yn = y / r;
        num zn = z / r;

        //new spin (rotated so that cone from which the new spin is chosen has its center axis precisely aligned with the old spin);
        // this gives a normalized vector
        num newx = (sinTheta / (x2n+y2n)) * ((x2n*zn + y2n)*cosPhi + (zn-1)*xn*yn*sinPhi) + xn*cosTheta;
        num newy = (sinTheta / (x2n+y2n)) * ((zn-1)*xn*yn*cosPhi + (x2n + y2n*zn)*sinPhi) + yn*cosTheta;
        num newz = -sinTheta * (xn*cosPhi + yn*sinPhi) + zn*cosTheta;

        // Then we set the length of the new spin appropriately
        newx *= r;
        newy *= r;
        newz *= r;

        //new orientation
        vec[0] = newx;
        vec[1] = newy;
        vec[2] = newz;

        return vec;
    }
};


template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::changedPhiInt
DetSDW<CB, OPDIM>::proposeRotatedPhi(uint32_t site, uint32_t timeslice) {
    assert(OPDIM == 3);
    Phi newphi = proposeRandomRotatedVector<OPDIM>::give(
        rng, ad.angleDelta, getPhi(site, timeslice));
    return std::make_tuple(PHI, newphi, cdwl(site,timeslice));
}




template<int OPDIM>
struct proposeRandomScaledVector {
    static std::tuple<VecNum::fixed<OPDIM>, bool> give (NormalDistribution& normal_distribution,
                                                        num scaleDelta, VecNum::fixed<OPDIM> vec) {
        (void) normal_distribution; (void) scaleDelta; (void) vec;
        throw_GeneralError("proposeRandomScaledVector is only supported for the O(3) model");
        return std::make_tuple(vec, false);
    }
};
template<> struct proposeRandomScaledVector<3> {
    static std::tuple<VecNum::fixed<3>, bool> give(NormalDistribution&
                                                   normal_distribution, num scaleDelta,
                                                   VecNum::fixed<3> vec) {
        using std::pow; using std::abs;
        //old orientation
        num x = vec[0];
        num y = vec[1];
        num z = vec[2];
        //squares
        num x2 = pow(x, 2);
        num y2 = pow(y, 2);
        num z2 = pow(z, 2);
        //cubed length
        num r3 = pow(x2 + y2 + z2, 3.0/2.0);

        //Choose a new cubed length from the Gaussian distribution around the original cubed length.
        //We use scaleDelta as the standard deviation of that distribution.
        //It is nececssary to consider the cubed length, as we have in spherical coordinates for
        //the infinitesimal volume element: dV = d(r^3 / 3) d\phi d(\cos\theta), and we do not
        //want to bias against long lengths
        num new_r3 = normal_distribution.get(scaleDelta, r3);
        num scale  = 1.0;
        bool valid = true;
        // The gaussian-distributed new r^3 might be negative or zero, in that case the proposed new spin must
        // be rejected -- we sample r only from (0, inf).  In this case we just return the original spin again
        // and declare the update as to be rejected.
        // Otherwise re scale the original spin appropriately.
        if (new_r3 <= 0) {
            scale = 1.0;
            valid = false;
        } else {
            scale = pow((new_r3 / r3), (1.0 / 3.0));
            valid = true;
        }

        num new_x = x * scale;
        num new_y = y * scale;
        num new_z = z * scale;

        vec[0] = new_x;
        vec[1] = new_y;
        vec[2] = new_z;

        return std::make_tuple(vec, valid);
    }
};


template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::changedPhiInt
DetSDW<CB, OPDIM>::proposeScaledPhi(uint32_t site, uint32_t timeslice) {
    bool valid;
    Phi new_phi;
    std::tie(new_phi, valid) = proposeRandomScaledVector<OPDIM>::give(
        normal_distribution, ad.scaleDelta, getPhi(site, timeslice));
    return std::make_tuple(valid ? PHI : NONE, new_phi, cdwl(site,timeslice));
}


template<int OPDIM>
struct proposeRandomRotatedScaledVector {
    static std::tuple<VecNum::fixed<OPDIM>, bool> give(NormalDistribution& normal_distribution,
                                                RngWrapper& rng, num angleDelta, num scaleDelta,
                                                VecNum::fixed<OPDIM> vec) {
        (void) normal_distribution; (void) rng;
        (void) scaleDelta; (void) angleDelta; (void) vec;
        throw_GeneralError("proposeRandomRotatedScaledVector is only supported for the O(3) model");
        return std::make_tuple(vec, false);
    }
};
template<>
struct proposeRandomRotatedScaledVector<3> {
    static std::tuple<VecNum::fixed<3>, bool> give(NormalDistribution& normal_distribution,
                                                   RngWrapper& rng, num angleDelta, num scaleDelta,
                                                   VecNum::fixed<3> vec) {
        using std::pow; using std::sqrt; using std::cos; using std::sin;

        //old orientation
        num x = vec[0];
        num y = vec[1];
        num z = vec[2];
        //squares:
        num x2 = pow(x, 2);
        num y2 = pow(y, 2);
        num z2 = pow(z, 2);
        //squared length
        num r2 = x2 + y2 + z2;
        //length
        num r = sqrt(r2);
        //cubed length
        num r3 = pow(r, 3);

        //Choose a new cubed length from the Gaussian distribution around the original cubed length.
        //We use scaleDelta as the standard deviation of that distribution.
        //It is nececssary to consider the cubed length, as we have in spherical coordinates for
        //the infinitesimal volume element: dV = d(r^3 / 3) d\phi d(\cos\theta), and we do not
        //want to bias against long lengths
        num new_r3 = normal_distribution.get(scaleDelta, r3);
        if (new_r3 <= 0) {
            // The gaussian-distributed new r^3 might be negative or zero, in that case the proposed new spin must
            // be rejected -- we sample r only from (0, inf).  In this case we just return the original spin again
            // and declare it as to be rejected.

            //new orientation = old orientation
            vec[0] = x;
            vec[1] = y;
            vec[2] = z;
            return std::make_tuple(vec, false);
        } else {
            // otherwise we scale the spin appropriately and also change its orientation

            //new angular coordinates:
            num cosTheta = rng.rand01() * (1.0 - angleDelta) + angleDelta;     // \in [angleDelta, 1.0] since rand() \in [0, 1.0]
            num phi = rng.rand01() * 2.0 * M_PI;
            num sinTheta = sqrt(1.0 - pow(cosTheta, 2.0));
            num cosPhi = cos(phi);
            num sinPhi = sin(phi);

            // To find the new orientation, we first consider the normalized old spin
            num x2n = x2 / r2;
            num y2n = y2 / r2;
            num xn = x / r;
            num yn = y / r;
            num zn = z / r;
            // new spin (rotated so that cone from which the new spin is chosen has its center axis precisely aligned with the old spin);
            // this gives a normalized vector
            num newx = (sinTheta / (x2n+y2n)) * ((x2n*zn + y2n)*cosPhi + (zn-1)*xn*yn*sinPhi) + xn*cosTheta;
            num newy = (sinTheta / (x2n+y2n)) * ((zn-1)*xn*yn*cosPhi + (x2n + y2n*zn)*sinPhi) + yn*cosTheta;
            num newz = -sinTheta * (xn*cosPhi + yn*sinPhi) + zn*cosTheta;

            // Then we set the length of the new spin appropriately
            num new_r = pow(new_r3, (1.0 / 3.0));
            newx *= new_r;
            newy *= new_r;
            newz *= new_r;

            //new orientation
            vec[0] = newx;
            vec[1] = newy;
            vec[2] = newz;
            return std::make_tuple(vec, true);
        }
    }
};

template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::changedPhiInt DetSDW<CB, OPDIM>::proposeRotatedScaledPhi(uint32_t site, uint32_t timeslice) {
    bool changedPhi;
    Phi new_phi;
    std::tie(new_phi, changedPhi) = proposeRandomRotatedScaledVector<OPDIM>::give(
        normal_distribution, rng, ad.angleDelta, ad.scaleDelta,
        getPhi(site, timeslice));
    return std::make_tuple(
        (changedPhi ? PHI : NONE), new_phi, cdwl(site,timeslice));
}

template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::changedPhiInt DetSDW<CB, OPDIM>::proposeNewCDWl(
    uint32_t site, uint32_t timeslice) {
    int32_t cdwl_new;
    num r = rng.rand01();
    if      (r <= 0.25) cdwl_new = +2;
    else if (r <= 0.5)  cdwl_new = -2;
    else if (r <= 0.75) cdwl_new = +1;
    else                cdwl_new = -1;
    return std::make_tuple(CDWL, getPhi(site,timeslice), cdwl_new);
}


template<CheckerboardMethod CB, int OPDIM>
num DetSDW<CB, OPDIM>::deltaSPhi(uint32_t site, uint32_t timeslice, const Phi newphi) {
    using arma::dot;

    const auto dtau = pars.dtau;
    const auto r = pars.r;
    const auto u = pars.u;
    const auto c = pars.c;
    const auto z = pars.d * 2;

    Phi oldphi = getPhi(site, timeslice);

    Phi phiDiff = newphi - oldphi;

    num oldphiSq = dot(oldphi, oldphi);
    num newphiSq = dot(newphi, newphi);
    num phiSqDiff = newphiSq - oldphiSq;

    if (pars.phi2bosons) {
        num delta = dtau * 0.5 * r * phiSqDiff;
        return delta;
    }
    else {
        num oldphiPow4 = oldphiSq * oldphiSq;
        num newphiPow4 = newphiSq * newphiSq;
        num phiPow4Diff = newphiPow4 - oldphiPow4;

        uint32_t kEarlier = timeNeigh(ChainDir::MINUS, timeslice);
        Phi phiEarlier = getPhi(site, kEarlier);
        uint32_t kLater = timeNeigh(ChainDir::PLUS, timeslice);
        Phi phiLater = getPhi(site, kLater);
        Phi phiTimeNeigh = phiLater + phiEarlier;

        Phi phiZero;
        phiZero.zeros();
        Phi phiSpaceNeigh = std::accumulate(
            spaceNeigh.beginNeighbors(site),
            spaceNeigh.endNeighbors(site),
            phiZero,
            [this, timeslice] (Phi accum, uint32_t neighSite) -> Phi{
                typedef DetSDW<CB,OPDIM> D;
                accum += D::getPhi(neighSite, timeslice);
                return accum;
            }
            );

        num delta1 = (1.0 / (c * c * dtau)) * (phiSqDiff - dot(phiTimeNeigh, phiDiff));

        num delta2 = 0.5 * dtau * (z * phiSqDiff - 2.0 * dot(phiSpaceNeigh, phiDiff));

        num delta3 = dtau * (0.5 * r * phiSqDiff + 0.25 * u * phiPow4Diff);

        return delta1 + delta2 + delta3;
    }
}


template<CheckerboardMethod CB, int OPDIM>
num DetSDW<CB, OPDIM>::phiAction() {
    const auto dtau = pars.dtau;
    const auto r = pars.r;
    const auto u = pars.u;
    const auto c = pars.c;
    const auto N = pars.N;
    const auto m = pars.m;
    //switched to asymmetric numerical derivative
    arma::field<Phi> phiCopy(N, m+1);
    for (uint32_t timeslice = 1; timeslice <= m; ++timeslice) {
        for (uint32_t site = 0; site < N; ++site) {
            for (uint32_t dim = 0; dim < OPDIM; ++dim) {
                phiCopy(site, timeslice)[dim] = phi(site, dim, timeslice);
            }
        }
    }
    num action = 0;
    for (uint32_t timeslice = 1; timeslice <= m; ++timeslice) {
        for (uint32_t site = 0; site < N; ++site) {
            if (not pars.phi2bosons) {
                Phi timeDerivative =
                    (phiCopy(site, timeslice) - phiCopy(site, timeNeigh(ChainDir::MINUS, timeslice)))
                    / dtau;
                action += (dtau / (2.0 * c * c)) * arma::dot(timeDerivative, timeDerivative);

                //count only neighbors in PLUS-directions: no global overcounting of bonds
                // std::cout << "site, timeslice: " << site << ", " << timeslice << std::endl;
                // std::cout << "spaceNeigh(XPLUS, site), timeslice: " << spaceNeigh(XPLUS, site) << ", " << timeslice << std::endl;
                // std::cout << "phiCopy(site, timeslice): " << phiCopy(site, timeslice) << std::endl;
                // std::cout << "phiCopy(spaceNeigh(XPLUS, site), timeslice): " << phiCopy(spaceNeigh(XPLUS, site), timeslice) << std::endl;
                // std::cout << "phiCopy(site, timeslice) - phiCopy(spaceNeigh(XPLUS, site), timeslice): " <<
                //     (phiCopy(site, timeslice) - phiCopy(spaceNeigh(XPLUS, site), timeslice)) << std::endl;

                Phi xneighDiff = phiCopy(site, timeslice) -
                    phiCopy(spaceNeigh(XPLUS, site), timeslice);
                // Phi ph1 = phiCopy(site, timeslice);
                // Phi ph2 = phiCopy(spaceNeigh(XPLUS, site), timeslice);
                // Phi xneighDiff =  ph1 - ph2;
                    
                // std::cout << "Phi xneighDiff: " << xneighDiff << std::endl;
                
                action += 0.5 * dtau * arma::dot(xneighDiff, xneighDiff);
                Phi yneighDiff = phiCopy(site, timeslice) -
                    phiCopy(spaceNeigh(YPLUS, site), timeslice);
                action += 0.5 * dtau * arma::dot(yneighDiff, yneighDiff);
            }

            num phisq = arma::dot(phiCopy(site, timeslice), phiCopy(site, timeslice));
            action += 0.5 * dtau * r * phisq;

            if (not pars.phi2bosons) {
                action += 0.25 * dtau * u * std::pow(phisq, 2);
            }
        }
    }
    return action;
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::thermalizationOver(int processIndex) {
    std::string prefix;
    if (processIndex == -1) {
        prefix = "";
    } else {
        prefix = "p" + numToString(processIndex) + ": r"
            + numToString(pars.r) + " ";
    }

    std::cout << prefix
              << "After thermalization: phiDelta = " << ad.phiDelta << '\n'
              << prefix
              << "recent local accRatio = " << ad.accRatioLocal_box_RA.get()
              << std::endl;
    if (pars.globalShift) {
        num ratio = 0;
        if (us.attemptedGlobalShifts) {
            ratio = num(us.acceptedGlobalShifts) / num(us.attemptedGlobalShifts);
        }
        std::cout << prefix
                  << "globalShiftMove acceptance ratio = " << ratio
                  << std::endl;
    }
    if (pars.wolffClusterUpdate) {
        num ratio = 0;
        if (us.attemptedWolffClusterUpdates) {
            ratio = num(us.acceptedWolffClusterUpdates) /
                num(us.attemptedWolffClusterUpdates);
        }
        num avgsize = 0.;
        if (us.acceptedWolffClusterUpdates) {
            avgsize = us.addedWolffClusterSize / num(pars.repeatWolffPerSweep * 
                                                     us.acceptedWolffClusterUpdates);
        }
        std::cout << prefix
                  << "wolffClusterUpdate acceptance ratio = " << ratio
                  << ", average accepted size = " << avgsize << "\n"
                  << std::endl;
    }
    if (pars.wolffClusterShiftUpdate) {
        num ratio = 0.;
        if (us.attemptedWolffClusterShiftUpdates != 0) {
            ratio = num(us.acceptedWolffClusterShiftUpdates) /
                num(us.attemptedWolffClusterShiftUpdates);
        }
        num avgsize = 0;
        if (us.acceptedWolffClusterShiftUpdates != 0) {
            avgsize = us.addedWolffClusterSize / num(pars.repeatWolffPerSweep *
                                                     us.acceptedWolffClusterShiftUpdates);
        }
        std::cout << prefix
                  << "wolffClusterShiftUpdate acceptance ratio = " << ratio
                  << ", average accepted size = " << avgsize << "\n"
                  << std::endl;
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::thermalizationOver() {
    thermalizationOver(-1);
}



template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::sweepSimple(bool takeMeasurements) {

    if (not pars.turnoffFermions) {

        sweepSimple_skeleton(takeMeasurements,
                             sdwComputeBmat(this),
                             [this](uint32_t timeslice) {this->updateInSlice(timeslice);},
                             [this]() {this->initMeasurements();},
                             [this](uint32_t timeslice) {this->measure(timeslice);},
                             [this]() {this->finishMeasurements();});

    } else {
        // sweepSimple_skeleton without the Green's function updates

        if (takeMeasurements) {
            initMeasurements();
        }
        for (uint32_t timeslice = 1; timeslice <= m; ++timeslice) {
            this->updateInSlice(timeslice);
            if (takeMeasurements) {
                measure(timeslice);
            }
        }
        if (takeMeasurements) {
            finishMeasurements();
        }

    }

    ++performedSweeps;
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::sweepSimpleThermalization() {

    if (not pars.turnoffFermions) {

        sweepSimpleThermalization_skeleton(
            sdwComputeBmat(this),
            [this](uint32_t timeslice) {
                this->updateInSliceThermalization(timeslice);
            });

    } else {

        // sweepSimple_skeleton without the Green's function updates
        for (uint32_t timeslice = 1; timeslice <= m; ++timeslice) {
            this->updateInSliceThermalization(timeslice);
        }

    }

    ++performedSweeps;
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::sweep(bool takeMeasurements) {

    // temp. DEBUG
    try {

        //std::cout << "sweep " << performedSweeps << '\n';
        // CHECK_NAN(g);           // temp. DEBUG

        if (not pars.turnoffFermions) {

            sweep_skeleton(takeMeasurements,
                           sdwLeftMultiplyBmat(this), sdwRightMultiplyBmat(this),
                           sdwLeftMultiplyBmatInv(this), sdwRightMultiplyBmatInv(this),
                           [this](uint32_t timeslice) {this->updateInSlice(timeslice);},
                           [this]() {this->initMeasurements();},
                           [this](uint32_t timeslice) {this->measure(timeslice);},
                           [this]() {this->finishMeasurements();},
                           [this]() {this->globalMove();},
                           [this](const MatData& g1, const MatData& g2, SweepDirection cur_sweep_dir) {
                               this->greenConsistencyCheck(g1, g2, cur_sweep_dir);
                           });

            ++performedSweeps;

        } else {

            if (lastSweepDir == SweepDirection::Up) {
                this->globalMove();
                sweepSimple(takeMeasurements);
                lastSweepDir = SweepDirection::Down;
            } else {
                sweepSimple(takeMeasurements);
                lastSweepDir = SweepDirection::Up;
            }

        }

    } catch (const GeneralError& err) {
        std::cerr << "Caught GeneralError. Failed SVD? Saving field configurations to disk! \n"
                  << "The message is: " << err.what() << "\n";
        for (uint32_t k = 0; k <= m; ++k) {
            std::string fname = "phi_k" + numToString(k);
            debugSaveMatrixRealOrCpx(phi.slice(k), fname);
        }
        throw err;
    }

}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::sweepThermalization() {

    if (not pars.turnoffFermions) {

        sweepThermalization_skeleton(sdwLeftMultiplyBmat(this), sdwRightMultiplyBmat(this),
                                     sdwLeftMultiplyBmatInv(this), sdwRightMultiplyBmatInv(this),
                                     [this](uint32_t timeslice) {
                                         this->updateInSliceThermalization(timeslice);
                                     },
                                     [this]() {this->globalMove();},
                                     [this](const MatData& g1, const MatData& g2, SweepDirection cur_sweep_dir) {
                                         this->greenConsistencyCheck(g1, g2, cur_sweep_dir);
                                     });

        ++performedSweeps;

    } else {

        if (lastSweepDir == SweepDirection::Up) {
            this->globalMove();
            sweepSimpleThermalization();
            lastSweepDir = SweepDirection::Down;
        } else {
            sweepSimpleThermalization();
            lastSweepDir = SweepDirection::Up;
        }

    }

}


template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::shiftGreenSymmetric() {
    typedef arma::subview<DataType> SubMatData;            //don't do references or const-references of this type
    if (CB == CB_NONE){
        //non-checkerboard
        return shiftGreenSymmetric_impl(
            //rightMultiply
            [this](SubMatData output, SubMatData input, Band band) -> void {
                output = input * propK_half_inv[band];
            },
            //leftMultiply
            [this](SubMatData output, SubMatData input, Band band) -> void {
                output = propK_half[band] * input;
            }
            );
    }
    else if (CB == CB_ASSAAD_BERG) {
        // here, we do not need to multiply the chemical potential
        // terms, since those multiplied from the left and right would
        // cancel.  e^{-mu dtau / 2} \Id from one side, e^{+mu dtau / 2} \Id
        if (not pars.weakZflux) {
            return shiftGreenSymmetric_impl(
                //rightMultiply
                // output and input are NxN blocks of a complex matrix
                // this effectively multiplies [Input] * e^{+ dtau K^band_1 / 2} e^{+ dtau K^band_0 / 2}
                // to the right of input and stores the result in output
                [this](SubMatData output, SubMatData input, Band band) -> void {
                    output = input;      //copy
                    this->cb_assaad_applyBondFactorsRight(output, 1, coshHopHorHalf[band], +sinhHopHorHalf[band],
                                                          coshHopVerHalf[band], +sinhHopVerHalf[band]);
                    this->cb_assaad_applyBondFactorsRight(output, 0, coshHopHorHalf[band], +sinhHopHorHalf[band],
                                                          coshHopVerHalf[band], +sinhHopVerHalf[band]);
                },
                //leftMultiply
                // output and input are NxN blocks of a complex matrix
                // this effectively multiplies e^{- dtau K^band_1 / 2} e^{- dtau K^band_0 / 2} * [Input]
                // to the left of input and stores the result in output
                [this](SubMatData output, SubMatData input, Band band) -> void {
                    output = input;      //copy
                    this->cb_assaad_applyBondFactorsLeft(output, 1, coshHopHorHalf[band], -sinhHopHorHalf[band],
                                                         coshHopVerHalf[band], -sinhHopVerHalf[band]);
                    this->cb_assaad_applyBondFactorsLeft(output, 0, coshHopHorHalf[band], -sinhHopHorHalf[band],
                                                         coshHopVerHalf[band], -sinhHopVerHalf[band]);
                }
                );
        } else {
            return shiftGreenSymmetric_impl(
                //rightMultiply
                // output and input are NxN blocks of a complex matrix
                // this effectively multiplies [Input] * e^{+ dtau K^band_1 / 2} e^{+ dtau K^band_0 / 2}
                // to the right of input and stores the result in output
                [this](SubMatData output, SubMatData input, Band band) -> void {
                    output = input;      //copy
                    this->cb_assaad_applyBondFactorsRight_precalcedMatrices(output, 1, expHop4Site_plusHalf[band]);
                    this->cb_assaad_applyBondFactorsRight_precalcedMatrices(output, 0, expHop4Site_plusHalf[band]);
                },
                //leftMultiply
                // output and input are NxN blocks of a complex matrix
                // this effectively multiplies e^{- dtau K^band_1 / 2} e^{- dtau K^band_0 / 2} * [Input]
                // to the left of input and stores the result in output
                [this](SubMatData output, SubMatData input, Band band) -> void {
                    output = input;      //copy
                    this->cb_assaad_applyBondFactorsLeft_precalcedMatrices(output, 1, expHop4Site_minusHalf[band]);
                    this->cb_assaad_applyBondFactorsLeft_precalcedMatrices(output, 0, expHop4Site_minusHalf[band]);
                }
                );
        }
    }
}

//RightMultiply and LeftMultiply should be functors for complex|real matrix subviews,
//that take parameters (output, input, [BAND]).  Armadillo submatrix views apparently do not have
//any const correctness, and passing them by reference makes no sense (they are rich
//references in a sense)
template<CheckerboardMethod CB, int OPDIM>
template<class RightMultiply, class LeftMultiply>
typename DetSDW<CB, OPDIM>::MatData
DetSDW<CB, OPDIM>::shiftGreenSymmetric_impl(RightMultiply rightMultiply, LeftMultiply leftMultiply) {
    const auto N = pars.N;
    //submatrix view helper for a 4N*4N or 2N*2N matrix
#define block(matrix, row, col) matrix.submat((row) * N, (col) * N, ((row) + 1) * N - 1, ((col) + 1) * N - 1)
    MatData tempG(MatrixSizeFactor*N, MatrixSizeFactor*N);
    const MatData& oldG = g;
    //multiply e^(dtau/2 K) from the right
    for (uint32_t row = 0; row < MatrixSizeFactor; ++row) {
        //block(tempG, row, 0) = block(oldG, row, 0) * propKx_half_inv;
        rightMultiply( block(tempG, row, 0), block(oldG, row, 0), XBAND );
        rightMultiply( block(tempG, row, 1), block(oldG, row, 1), YBAND );
        if (OPDIM == 3) {
            rightMultiply( block(tempG, row, 2), block(oldG, row, 2), XBAND );
            rightMultiply( block(tempG, row, 3), block(oldG, row, 3), YBAND );
        }
    }
    //multiply e^(-dtau/2 K) from the left
    MatData newG(MatrixSizeFactor*N, MatrixSizeFactor*N);
    for (uint32_t col = 0; col < MatrixSizeFactor; ++col) {
        //block(newG, 0, col) = propKx_half * block(tempG, 0, col);
        leftMultiply( block(newG, 0, col), block(tempG, 0, col), XBAND );
        leftMultiply( block(newG, 1, col), block(tempG, 1, col), YBAND );
        if (OPDIM == 3) {
            leftMultiply( block(newG, 2, col), block(tempG, 2, col), XBAND );
            leftMultiply( block(newG, 3, col), block(tempG, 3, col), YBAND );
        }
    }
#undef block
    return newG;
}


// virtual function, called during sweep in base class DetModelGC
template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::consistencyCheck() {
    if (pars.turnoffFermions) {
        return;
    }

    const auto N = pars.N;
    const auto m = pars.m;
    // phi*, coshTerm*, sinhTerm*
    for (uint32_t k = 1; k <= m; ++k) {
        for (uint32_t site = 0; site < N; ++site) {
            num coshTermPhiBefore = coshTermPhi(site, k);
            num sinhTermPhiBefore = sinhTermPhi(site, k);
            num coshTermCDWlBefore = coshTermCDWl(site, k);
            num sinhTermCDWlBefore = sinhTermCDWl(site, k);
            updateCoshSinhTerms(site, k);
            num relDiffSinh, relDiffCosh;
            if (std::abs(coshTermPhiBefore) > 1E-10) {
                relDiffCosh = std::abs((coshTermPhi(site, k) - coshTermPhiBefore) / coshTermPhiBefore);
                if (relDiffCosh > 1E-10) {
                    throw_GeneralError("coshTermPhi is inconsistent");
                }
            }
            if (std::abs(sinhTermPhiBefore) > 1E-10) {
                relDiffSinh = std::abs((sinhTermPhi(site, k) - sinhTermPhiBefore) / sinhTermPhiBefore);
                if (relDiffSinh > 1E-10) {
                    throw_GeneralError("sinhTermPhi is inconsistent");
                }
            }
            if (std::abs(coshTermCDWlBefore) > 1E-10) {
                relDiffCosh = std::abs((coshTermCDWl(site, k) - coshTermCDWlBefore) / coshTermCDWlBefore);
                if (relDiffCosh > 1E-10) {
                    throw_GeneralError("coshTermCDWl is inconsistent");
                }
            }
            if (std::abs(sinhTermCDWlBefore) > 1E-10) {
                relDiffSinh = std::abs((sinhTermCDWl(site, k) - sinhTermCDWlBefore) / sinhTermCDWlBefore);
                if (relDiffSinh > 1E-10) {
                    throw_GeneralError("sinhTermCDWl is inconsistent");
                }
            }
        }
    }
    // cdwl
    for (uint32_t k = 1; k <= m; ++k) {
        for (uint32_t site = 0; site < N; ++site) {
            int32_t l = cdwl(site, k);
            if (l != +2 and l != -2 and l != +1 and l != -1) {
                throw_GeneralError("cdwl is inconsistent");
            }
        }
    }
    // compare bmat-evaluation
    if (loggingParams.checkCheckerboardConsistency) {
        constexpr auto MSF = MatrixSizeFactor;
        for (uint32_t k = 1; k <= m; ++k) {
            MatData bk = computeBmatSDW(k, k-1);
            MatData bk_inv = arma::inv(bk);
            MatData checkbk_left = checkerboardLeftMultiplyBmat(
                arma::eye<MatData>(MSF*N,MSF*N),
                k, k-1);
            MatData checkbk_right = checkerboardRightMultiplyBmat(
                arma::eye<MatData>(MSF*N,MSF*N),
                k, k-1);
            MatData checkbk_inv_left = checkerboardLeftMultiplyBmatInv(
                arma::eye<MatData>(MSF*N,MSF*N),
                k, k-1);
            MatData checkbk_inv_right = checkerboardRightMultiplyBmatInv(
                arma::eye<MatData>(MSF*N,MSF*N),
                k, k-1);
            std::cout << "cb:" << CB << " " << k << "\n";
            print_matrix_diff(bk, checkbk_left, "bk_left");
            print_matrix_diff(bk_inv, checkbk_inv_left, "bk_inv_left");
            print_matrix_diff(bk, checkbk_right, "bk_right");
            print_matrix_diff(bk_inv, checkbk_inv_right, "bk_inv_right");
            checkarray<VecNum, OPDIM> phik;
            phik[0] = phi.slice(k).col(0);
            if (OPDIM > 1) {
                phik[1] = phi.slice(k).col(1);
            }
            if (OPDIM > 2) {
                phik[2] = phi.slice(k).col(2);
            }
            MatData emv = computePotentialExponential(-1, phik, cdwl.col(k));
            MatData propK_whole(MSF*N, MSF*N);
            propK_whole.zeros();
#define block(matrix, row, col) matrix.submat((row) * N, (col) * N, ((row) + 1) * N - 1, ((col) + 1) * N - 1)
            block(propK_whole, 0, 0) = propKx;
            block(propK_whole, 1, 1) = propKy;   // !
            if (OPDIM == 3) {
                block(propK_whole, 2, 2) = propKx;   // !
                block(propK_whole, 3, 3) = propKy;
            }
            MatData bk_ref = emv * propK_whole;
            print_matrix_diff(bk, bk_ref, "bk_ref");
#undef block
            MatData bk_ref_inv = arma::inv(bk_ref);
            print_matrix_diff(bk_inv, bk_ref_inv, "bk_ref_inv");
            // spaceneigh.save();
            // debugSaveMatrix(phi.slice(k).col(0), "phi0");
            // debugSaveMatrix(phi.slice(k).col(1), "phi1");
            // debugSaveMatrix(phi.slice(k).col(2), "phi2");
            // debugSaveMatrix(cdwl.col(k), "cdwl");
            // debugSaveMatrix(propKx, "propkx");
            // debugSaveMatrix(propKy, "propky");
            // debugSaveMatrixCpx(emv, "emv");
            // debugSaveMatrixCpx(bk, "bk");
            // debugSaveMatrixCpx(bk_inv, "bk_inv");
            // debugSaveMatrixCpx(checkbk_left, "check_bk_left");
            // debugSaveMatrixCpx(checkbk_inv_left, "check_bk_inv_left");
            // debugSaveMatrixCpx(checkbk_right, "check_bk_right");
            // debugSaveMatrixCpx(checkbk_inv_right, "check_bk_inv_right");
            // debugSaveMatrixCpx(bk_ref, "bk_ref");
            // debugSaveMatrixCpx(bk_ref_inv, "bk_ref_inv");
            // exit(0);
            //// save a single B-matrix 
            // debugSaveMatrixCpx(bk, "bk");
            // exit(0);
        }
    }

//     // Verify get_delta_forsite
//     for (uint32_t k = 1; k <= m; ++k) {
//     	for (auto stage : {1, 2}) {
//             uint32_t site = rng.randInt(0, N-1);
//             std::cout << k << " " << stage << " " << site << "\n";
//             Phi new_phi;
//             int32_t new_cdwl;
//             Changed changed;
//             switch (stage) {
//             case 1:
//                 std::tie(changed, new_phi, new_cdwl) = proposeNewPhiBox(site, k);
//                 break;
//             case 2:
//             default:
//                 std::tie(changed, new_phi, new_cdwl) = proposeNewCDWl(site, k);
//                 break;
//             }
//             MatSmall delta = get_delta_forsite(new_phi, new_cdwl, k, site);
//             checkarray<VecNum, OPDIM> o_phi;
// 	    o_phi[0] = phi.slice(k).col(0);
// 	    o_phi[1] = phi.slice(k).col(1);
// 	    o_phi[2] = phi.slice(k).col(2);
//             checkarray<VecNum, OPDIM> n_phi;
// 	    n_phi[0] = phi.slice(k).col(0);
// 	    n_phi[1] = phi.slice(k).col(1);
// 	    n_phi[2] = phi.slice(k).col(2);
// 	    n_phi[0][site] = new_phi[0];
// 	    n_phi[1][site] = new_phi[1];
// 	    n_phi[2][site] = new_phi[2];
//             VecInt n_cdwl = cdwl.col(k);
//             n_cdwl[site] = new_cdwl;
//             MatCpx big_delta =
//                 computePotentialExponential(-1, n_phi, n_cdwl)
//                 *
//                 computePotentialExponential(+1, o_phi, cdwl.col(k))
//                 -
//                 arma::eye<MatCpx>(MatrixSizeFactor*N, MatrixSizeFactor*N);
//             arma::uvec indices;
//             if (OPDIM == 3) {
//                 indices << site << site+N << site+2*N << site+3*N;
//             } else {
//                 indices << site << site+N;
//             }
//             MatCpx::fixed<MatrixSizeFactor,MatrixSizeFactor> big_delta_sub = big_delta.submat(indices, indices);
//             // std::cout << (new_phi[0] - phi0(site, k)) << " "
//             //           << (new_phi[1] - phi1(site, k)) << " "
//             //           << (new_phi[2] - phi2(site, k)) << std::endl;
//             // std::cout << "cdwl: " << cdwl(site,k) << " -> " << new_cdwl << ", gamma: " << cdwl_gamma(cdwl(site,k))
//             //           << " -> " << cdwl_gamma(new_cdwl)
//             //           << std::endl;
// //    		python_matshow2(arma::real(big_delta).eval(), "real(big_delta)",
// //    				        arma::real(delta).eval(), "real(delta)");
//             //python_matshow(arma::imag(big_delta).eval());
//             print_matrix_diff(delta, big_delta_sub, "delta");
// //    		python_matshow(arma::real(delta - big_delta_sub).eval());
// //    		python_matshow(arma::imag(delta - big_delta_sub).eval());
//     	}
//     }

   // //UdV storage -- unitarity
   // for (uint32_t l = 0; l <= n; ++l) {
   // 	const MatCpx& U   = (*UdVStorage)[0][l].U;
   // 	const MatCpx& V_t = (*UdVStorage)[0][l].V_t;
   // 	print_matrix_diff(
   // 			(U*U.t()).eval(),
   // 			eye_gc,
   // 			"U l=" + numToString(l)
   // 	);
   // 	print_matrix_diff(
   // 			(V_t.t()*V_t).eval(),
   // 			eye_gc,
   // 			"V l=" + numToString(l)
   // 	);
   // }
}


template<CheckerboardMethod CB, int OPDIM>
DetSDW<CB, OPDIM>::GreenConsistencyLogger::GreenConsistencyLogger(const std::string& logfiledir_, bool enabled)
    : logfiledir(logfiledir_) {
    if (enabled) {
        if (logfiledir == "") logfiledir = ".";
        fs::create_directories(logfiledir);
        fs::path up_log_path = fs::path(logfiledir) /
            fs::path("up_log.txt");
        fs::path down_log_path = fs::path(logfiledir) /
            fs::path("down_log.txt");
        up_log.open(up_log_path.c_str(), std::ios::app);
        down_log.open(down_log_path.c_str(), std::ios::app);
    }
}


template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::greenConsistencyCheck(const MatData& g1, const MatData& g2, SweepDirection cur_sweep_dir) {
    (void)(g1); (void)(g2); (void)(cur_sweep_dir);
    if (loggingParams.logGreenConsistency) {
        const auto N = pars.N;
        // log max total difference, mean difference, max difference on the block diagonals
        num diag_diff = 0.0;
        for (uint32_t colblock = 0; colblock < MatrixSizeFactor; ++colblock) {
            for (uint32_t rowblock = 0; rowblock < MatrixSizeFactor; ++rowblock) {
                for (uint32_t site = 0; site < N; ++ site) {
                    uint32_t colentry = site + colblock*N;
                    uint32_t rowentry = site + rowblock*N;
                    num diff = std::abs(g1(rowentry, colentry) - g2(rowentry, colentry));
                    if (diff > diag_diff) {
                        diag_diff = diff;
                    }
                }
            }
        }
        if (cur_sweep_dir == SweepDirection::Up) {
            greenConsistencyLogger.up_log << diag_diff << '\n';
        }
        else {
            greenConsistencyLogger.down_log << diag_diff << '\n';
        }
    }
}


template<CheckerboardMethod CB, int OPDIM>
num DetSDW<CB, OPDIM>::computeGreenDetRatioFromScratch(uint32_t timeslice, const CubeNum& newPhi) {
    // store data for the situation before switching to newPhi
    globalMoveStoreBackups();

    // compute the old Green's function from scratch to get its
    // singular values
    setupUdVStorage_and_calculateGreen_forTimeslice(timeslice);
    VecNum old_g_inv_sv = g_inv_sv;

    // temporarily go to newPhi
    phi = newPhi;
    updateCoshSinhTerms();

    // recompute new Green's function and its singular values
    setupUdVStorage_and_calculateGreen_forTimeslice(timeslice);

    // compute determinant ratio: (new weight) / (old weight) = det(G_old) / det(G_new)
    uint32_t count = MatrixSizeFactor * pars.N;
    num log_prob = 0.;
    for (uint32_t j = 0; j < count; ++j) {
        // log of g_inv_sv[j] / old_g_inv_sv[j]        {   g ~ [weight]^-1 --> g^{-1} ~ [weight]   }
        num log_diff = std::log(g_inv_sv[j]) - std::log(old_g_inv_sv[j]);
        log_prob += log_diff;
    }
    num det_ratio = std::exp(log_prob);

    // restore original situation
    globalMoveRestoreBackups();

    return det_ratio;
}


template<CheckerboardMethod CB, int OPDIM>
num DetSDW<CB, OPDIM>::computeGreenDetRatioFromScratch(uint32_t site, uint32_t timeslice, Phi singleNewPhi) {
    CubeNum newPhi = phi;
    for (uint32_t dim = 0; dim < OPDIM; ++dim) {
        newPhi(site, dim, timeslice) = singleNewPhi[dim];
    }
    return computeGreenDetRatioFromScratch(timeslice, newPhi);
}


// reference computation of the new Green's function after
// switching to the new phi-spin configuration
template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatData DetSDW<CB, OPDIM>::computeGreenFromScratch(uint32_t timeslice, const CubeNum& newPhi) {
    // store data for the situation before switching to newPhi
    globalMoveStoreBackups();

    // temporarily go to newPhi
    phi = newPhi;
    updateCoshSinhTerms();

    // recompute new Green's function and its singular values
    setupUdVStorage_and_calculateGreen_forTimeslice(timeslice);
    MatData new_green = g;

    // restore original situation
    globalMoveRestoreBackups();

    return new_green;
}


// helper wrapping the above for a single spin update
template<CheckerboardMethod CB, int OPDIM>
typename DetSDW<CB, OPDIM>::MatData DetSDW<CB, OPDIM>::computeGreenFromScratch(uint32_t site, uint32_t timeslice, Phi singleNewPhi) {
    CubeNum newPhi = phi;
    for (uint32_t dim = 0; dim < OPDIM; ++dim) {
        newPhi(site, dim, timeslice) = singleNewPhi[dim];
    }
    return computeGreenFromScratch(timeslice, newPhi);
}




//Write out current system configuration samples to disk: ASCII or
//binary.
//----------------------------------------------------------------

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::saveConfigurationStreamText(const std::string& directory) {
    fs::path phi_filepath = fs::path(directory) /
        fs::path("configs-phi.textstream");

    std::ofstream phi_output(phi_filepath.c_str(), std::ios::app);
    if (not phi_output) {
        std::cerr << "Could not open file " << phi_filepath.string() << " for writing.\n";
        std::cerr << "Error code: " << strerror(errno) << "\n";
    } else {
        phi_output.precision(14);
        phi_output.setf(std::ios::scientific, std::ios::floatfield);

        for (uint32_t ix = 0; ix < pars.L; ++ix) {
            for (uint32_t iy = 0; iy < pars.L; ++iy) {
                uint32_t i = iy*pars.L + ix;
                for (uint32_t k = 1; k <= pars.m; ++k) {
                    for (uint32_t dim = 0; dim < OPDIM; ++dim) {
                        phi_output << phi(i, dim, k) << "\n";
                    }
                }
            }
        }
        phi_output.flush();
    }

    if (pars.cdwU) {                 // we have not assigned 0.0 to cdwU
        fs::path cdwl_filepath = fs::path(directory) /
            fs::path("configs-l.textstream");

        std::ofstream cdwl_output(cdwl_filepath.c_str(), std::ios::app);
        if (not cdwl_output) {
            std::cerr << "Could not open file " << cdwl_filepath.string() << " for writing.\n";
            std::cerr << "Error code: " << strerror(errno) << "\n";
        } else {
            for (uint32_t ix = 0; ix < pars.L; ++ix) {
                for (uint32_t iy = 0; iy < pars.L; ++iy) {
                    uint32_t i = iy*pars.L + ix;
                    for (uint32_t k = 1; k <= pars.m; ++k) {
                        cdwl_output << cdwl(i, k) << "\n";
                    }
                }
            }
            cdwl_output.flush();
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::saveConfigurationStreamBinary(const std::string& directory) {
    fs::path phi_filepath = fs::path(directory) /
        fs::path("configs-phi.binarystream");

    std::ofstream phi_output(phi_filepath.c_str(), std::ios::binary | std::ios::app);
    if (not phi_output) {
        std::cerr << "Could not open file " << phi_filepath.string() << " for writing.\n";
        std::cerr << "Error code: " << strerror(errno) << "\n";
    } else {
        for (uint32_t ix = 0; ix < pars.L; ++ix) {
            for (uint32_t iy = 0; iy < pars.L; ++iy) {
                uint32_t i = iy*pars.L + ix;
                for (uint32_t k = 1; k <= pars.m; ++k) {
                    for (uint32_t dim = 0; dim < OPDIM; ++dim) {
                        phi_output.write(reinterpret_cast<const char*>(&(phi(i, dim, k))),
                                         sizeof(phi(i, dim, k)));
                    }
                }
            }
        }
        phi_output.flush();
    }

    if (pars.cdwU) {                 // we have not assigned 0.0 to cdwU
        fs::path cdwl_filepath = fs::path(directory) /
            fs::path("configs-l.binarystream");

        std::ofstream cdwl_output(cdwl_filepath.c_str(), std::ios::binary | std::ios::app);
        if (not cdwl_output) {
            std::cerr << "Could not open file " << cdwl_filepath.string() << " for writing.\n";
            std::cerr << "Error code: " << strerror(errno) << "\n";
        } else {
            for (uint32_t ix = 0; ix < pars.L; ++ix) {
                for (uint32_t iy = 0; iy < pars.L; ++iy) {
                    uint32_t i = iy*pars.L + ix;
                    for (uint32_t k = 1; k <= pars.m; ++k) {
                        cdwl_output.write(reinterpret_cast<const char*>(&(cdwl(i, k))),
                                          sizeof(cdwl(i, k)));
                    }
                }
            }
            cdwl_output.flush();
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::saveConfigurationStreamTextHeader(
    const std::string& simInfoHeaderText, const std::string& directory) {
    fs::path phi_filepath = fs::path(directory) /
        fs::path("configs-phi.textstream");
    // only write the header if the file does not exist yet
    if (not fs::exists(phi_filepath)) {
        std::ofstream phi_output(phi_filepath.c_str(), std::ios::out);
        if (not phi_output) {
            std::cerr << "Could not open file " << phi_filepath.string() << " for writing.\n";
            std::cerr << "Error code: " << strerror(errno) << "\n";
        } else {
            phi_output << simInfoHeaderText;
            phi_output << "## phi configuration stream\n";
            phi_output.flush();
        }
    }

    if (pars.cdwU) {                 // we have not assigned 0.0 to cdwU
        fs::path cdwl_filepath = fs::path(directory) /
            fs::path("configs-l.textstream");
        // only write the header if the file does not exist yet
        if (not fs::exists(cdwl_filepath)) {
            std::ofstream cdwl_output(cdwl_filepath.c_str(), std::ios::out);
            if (not cdwl_output) {
                std::cerr << "Could not open file " << cdwl_filepath.string() << " for writing.\n";
                std::cerr << "Error code: " << strerror(errno) << "\n";
            } else {
                cdwl_output << simInfoHeaderText;
                cdwl_output << "## l configuration stream\n";
                cdwl_output.flush();
            }
        }
    }
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::saveConfigurationStreamBinaryHeaderfile(
    const std::string& simInfoHeaderText,
    const std::string& directory) {
    fs::path phi_filepath = fs::path(directory) /
        fs::path("configs-phi.infoheader");
    // only write the header if the file does not exist yet
    if (not fs::exists(phi_filepath)) {
        std::ofstream phi_output(phi_filepath.c_str(), std::ios::out);
        if (not phi_output) {
            std::cerr << "Could not open file " << phi_filepath.string() << " for writing.\n";
            std::cerr << "Error code: " << strerror(errno) << "\n";
        } else {
            phi_output << simInfoHeaderText;
            phi_output << "## binary phi configuration stream (64 bit double precision floats) in file configs-phi.binarystream\n";
            phi_output.flush();
        }
    }

    if (pars.cdwU) {                 // we have not assigned 0.0 to cdwU
        fs::path cdwl_filepath = fs::path(directory) /
            fs::path("configs-l.infoheader");
        // only write the header if the file does not exist yet
        if (not fs::exists(cdwl_filepath)) {
            std::ofstream cdwl_output(cdwl_filepath.c_str(), std::ios::out);
            if (not cdwl_output) {
                std::cerr << "Could not open file " << cdwl_filepath.string() << " for writing.\n";
                std::cerr << "Error code: " << strerror(errno) << "\n";
            } else {
                cdwl_output << simInfoHeaderText;
                cdwl_output << "## binary l configuration stream (32 bit signed integers) in file configs-l.binarystream\n";
                cdwl_output.flush();
            }
        }
    }
}



//Methods used to implement writing out system configurations in a
//replica exchange simulation (used by DetQMCPT).
//----------------------------------------------------------------
template<CheckerboardMethod CB, int OPDIM>
DetSDW_SystemConfig DetSDW<CB, OPDIM>::getCurrentSystemConfiguration() {
    if (pars.cdwU) {                 // we have not assigned 0.0 to cdwU
        return DetSDW_SystemConfig(pars, phi, cdwl);
    } else {
        return DetSDW_SystemConfig(pars, phi);
    }
}

template<CheckerboardMethod CB, int OPDIM>
DetSDW_SystemConfig_FileHandle DetSDW<CB, OPDIM>::prepareSystemConfigurationStreamFileHandle(
        bool binaryStream, bool textStream, const std::string& directory) {
    if (not (binaryStream or textStream)) {
        throw_GeneralError("binaryStream or textStream must be sepcified to create file handle");
    }

    DetSDW_SystemConfig_FileHandle file_handle;

    typedef DetSDW_SystemConfig_FileHandle::OfstreamPointer OfstreamPointer;

    if (binaryStream) {
        fs::path phi_filepath = fs::path(directory) /
            fs::path("configs-phi.binarystream");
        file_handle.phi_output_binary = OfstreamPointer(
            new std::ofstream(phi_filepath.c_str(), std::ios::binary | std::ios::app) );
        if (file_handle.phi_output_binary->fail()) {
            std::cerr << "Could not open file " << phi_filepath.string() << " for writing.\n";
            std::cerr << "Error code: " << strerror(errno) << "\n";
        }
    }
    if (textStream) {
        fs::path phi_filepath = fs::path(directory) /
            fs::path("configs-phi.textstream");
        file_handle.phi_output_text = OfstreamPointer(
            new std::ofstream(phi_filepath.c_str(), std::ios::app) );
        if (file_handle.phi_output_text->fail()) {
            std::cerr << "Could not open file " << phi_filepath.string() << " for writing.\n";
            std::cerr << "Error code: " << strerror(errno) << "\n";
        }
        file_handle.phi_output_text->precision(14);
        file_handle.phi_output_text->setf(std::ios::scientific, std::ios::floatfield);
    }
    
    if (pars.cdwU) {
        if (binaryStream) {
            fs::path cdwl_filepath = fs::path(directory) /
                fs::path("configs-l.binarystream");

            file_handle.cdwl_output_binary = OfstreamPointer(
                new std::ofstream(cdwl_filepath.c_str(), std::ios::binary | std::ios::app));
            if (file_handle.cdwl_output_binary->fail()) {
                std::cerr << "Could not open file " << cdwl_filepath.string() << " for writing.\n";
                std::cerr << "Error code: " << strerror(errno) << "\n";
            }            
        }
        if (textStream) {
            fs::path cdwl_filepath = fs::path(directory) /
                fs::path("configs-l.textstream");

            file_handle.cdwl_output_text = OfstreamPointer(
                new std::ofstream(cdwl_filepath.c_str(), std::ios::app));
            if (file_handle.cdwl_output_text->fail()) {
                std::cerr << "Could not open file " << cdwl_filepath.string() << " for writing.\n";
                std::cerr << "Error code: " << strerror(errno) << "\n";
            }
        }
    }

    return file_handle;
}


//Methods to implement a replica-exchange / parallel tempering scheme

template<CheckerboardMethod CB, int OPDIM>
num DetSDW<CB, OPDIM>::get_exchange_parameter_value() const {
    return pars.r;
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::set_exchange_parameter_value(num r) {
    pars.r = r;
}

template<CheckerboardMethod CB, int OPDIM>
const char* DetSDW<CB, OPDIM>::get_exchange_parameter_name() const {
    return "r";
}

template<CheckerboardMethod CB, int OPDIM>
num DetSDW<CB, OPDIM>::get_exchange_action_contribution() const {
    num contrib = 0.0;
    for (uint32_t k = 1; k <= pars.m; ++k) {
        for (uint32_t i = 0; i < pars.N; ++i) {
            Phi phi = getPhi(i, k);
            num phiSquared = arma::dot(phi, phi);
            contrib += phiSquared;
        }
    }
    contrib *= 0.5 * pars.dtau;
    return contrib;
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::get_control_data(std::string& buffer) const {
    //serialize objects into a std::basic_string
    //cf. http://stackoverflow.com/questions/3015582/direct-boost-serialization-to-char-array
    namespace ios = boost::iostreams;
    ios::back_insert_device<std::string> inserter(buffer);
    ios::stream<ios::back_insert_device<std::string> > s(inserter);
    boost::archive::binary_oarchive oa(s);

    oa & us & ad;

    s.flush();
}

template<CheckerboardMethod CB, int OPDIM>
void DetSDW<CB, OPDIM>::set_control_data(const std::string& buffer) {
    //wrap buffer inside a stream and deserialize into objects
    //cf. http://stackoverflow.com/questions/3015582/direct-boost-serialization-to-char-array
    namespace ios = boost::iostreams;
    ios::basic_array_source<char> device(buffer.data(), buffer.size());
    ios::stream<ios::basic_array_source<char> > s(device);
    boost::archive::binary_iarchive ia(s);

    ia & us & ad;
}




// Related free-standing function

//unfortunately need to pull out code here to avoid duplication --
//partial function template specializations are not allowed
static inline num get_replica_exchange_probability_implementation_detsdw(
    num parameter_1, num action_contribution_1,
    num parameter_2, num action_contribution_2)
{
    // defined as in Hukushima, Nemoto (1996)
    num delta = (parameter_1 - parameter_2) *
        (action_contribution_2 - action_contribution_1);

    if (delta <= 0.0) {
        return 1.0;
    } else {
        return std::exp(-delta);
    }
}

template<>
num get_replica_exchange_probability<DetSDW<CB_NONE, 1> >(
    num parameter_1, num action_contribution_1,
    num parameter_2, num action_contribution_2)
{
    return get_replica_exchange_probability_implementation_detsdw(
        parameter_1, action_contribution_1,
        parameter_2, action_contribution_2);
}

template<>
num get_replica_exchange_probability<DetSDW<CB_ASSAAD_BERG, 1> >(
    num parameter_1, num action_contribution_1,
    num parameter_2, num action_contribution_2)
{
    return get_replica_exchange_probability_implementation_detsdw(
        parameter_1, action_contribution_1,
        parameter_2, action_contribution_2);
}


template<>
num get_replica_exchange_probability<DetSDW<CB_NONE, 2> >(
    num parameter_1, num action_contribution_1,
    num parameter_2, num action_contribution_2)
{
    return get_replica_exchange_probability_implementation_detsdw(
        parameter_1, action_contribution_1,
        parameter_2, action_contribution_2);
}

template<>
num get_replica_exchange_probability<DetSDW<CB_ASSAAD_BERG, 2> >(
    num parameter_1, num action_contribution_1,
    num parameter_2, num action_contribution_2)
{
    return get_replica_exchange_probability_implementation_detsdw(
        parameter_1, action_contribution_1,
        parameter_2, action_contribution_2);
}


template<>
num get_replica_exchange_probability<DetSDW<CB_NONE, 3> >(
    num parameter_1, num action_contribution_1,
    num parameter_2, num action_contribution_2)
{
    return get_replica_exchange_probability_implementation_detsdw(
        parameter_1, action_contribution_1,
        parameter_2, action_contribution_2);
}

template<>
num get_replica_exchange_probability<DetSDW<CB_ASSAAD_BERG, 3> >(
    num parameter_1, num action_contribution_1,
    num parameter_2, num action_contribution_2)
{
    return get_replica_exchange_probability_implementation_detsdw(
        parameter_1, action_contribution_1,
        parameter_2, action_contribution_2);
}



// to speed up compilation: allow to define some of the following
// macros if we do not use all possible instantiaions in our code
// (see detsdwo{1,2,3}.cpp)

#ifndef DETSDW_NO_O1
template class DetSDW<CB_NONE,1>;
template class DetSDW<CB_ASSAAD_BERG,1>;
#endif //DETSDW_NO_O1
#ifndef DETSDW_NO_O2
template class DetSDW<CB_NONE,2>;
template class DetSDW<CB_ASSAAD_BERG,2>;
#endif //DETSDW_NO_O2
#ifndef DETSDW_NO_O3
template class DetSDW<CB_NONE,3>;
template class DetSDW<CB_ASSAAD_BERG,3>;
#endif //DETSDW_NO_O3
