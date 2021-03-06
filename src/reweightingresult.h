/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  See the enclosed file LICENSE for a copy or if
 * that was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2017 Max H. Gerlach
 * 
 * */

/*
 * reweightingresult.h
 *
 *  Created on: Jun 29, 2011
 *      Author: gerlach
 */


// generalized for SDW DQMC (2015-02-06 - )


#ifndef REWEIGHTINGRESULT_H_
#define REWEIGHTINGRESULT_H_

#include "histogram.h"
#include "tools.h"

//this structure holds mean values, susceptibility, binder parameter and their errors
//determined at a certain beta by multi-reweighting
struct ReweightingResult {
    double energyAvg;           // for SDW: r/2 sum {phi^2}
    double energyError;
    double heatCapacity;        // will not be sensible for SDW
    double heatCapacityError;
    double obsAvg;
    double obsError;
    double obsSuscPart;         // systemSize * (<o^2>)
    double obsSuscPartError;
    double obsSusc;             // systemSize * (<o^2> - <o>^2)
    double obsSuscError;
    double obsBinder;           // 1 - <o^4> / (3 <o^2>^2)
    double obsBinderError;
    double obsBinderRatio;      // <o^4> / <o^2>^2
    double obsBinderRatioError;


    //optional pointers to reweighted histograms
    HistogramDouble* energyHistogram;
    HistogramDouble* obsHistogram;

    //default constructor: all zero
    ReweightingResult() :
        energyAvg(0), energyError(0), heatCapacity(0), heatCapacityError(0),
        obsAvg(0), obsError(0), obsSuscPart(0), obsSuscPartError(0),
        obsSusc(0), obsSuscError(0), obsBinder(0),
        obsBinderError(0), obsBinderRatio(0),
        obsBinderRatioError(0),
        energyHistogram(0), obsHistogram(0)
    { }

    //constructor setting only mean values, not error margins
    // ReweightingResult(double energy, double obs, double susc, double binder,
    //     ) :
    //     energyAvg(energy), energyError(-1), heatCapacity(-1),
    //             heatCapacityError(-1), obsAvg(obs), obsError(-1),
    //             obsSusc(susc), obsSuscError(-1), obsBinder(binder),
    //             obsBinderError(-1), energyHistogram(0), obsHistogram(0)
    // { }
    ReweightingResult(double energy, double heatCapacity, double obs,
                      double obsSuscPart,
                      double susc, double binder, double binderRatio) :
        energyAvg(energy), energyError(-1), heatCapacity(heatCapacity),
        heatCapacityError(-1), obsAvg(obs), obsError(-1),
        obsSuscPart(obsSuscPart), obsSuscPartError(-1),
        obsSusc(susc), obsSuscError(-1), obsBinder(binder),
        obsBinderError(-1), obsBinderRatio(binderRatio),
        obsBinderRatioError(-1),
        energyHistogram(0), obsHistogram(0)
    { }

    //constructor setting values and errors
    // ReweightingResult(double energy, double energyErr, double obs,
    //         double obsErr, double susc, double suscErr, double binder,
    //         double binderErr) :
    //     energyAvg(energy), energyError(energyErr), heatCapacity(-1),
    //             heatCapacityError(-1), obsAvg(obs), obsError(obsErr),
    //             obsSusc(susc), obsSuscError(suscErr), obsBinder(binder),
    //             obsBinderError(binderErr), energyHistogram(0), obsHistogram(0)
    // { }
    ReweightingResult(double energy, double energyErr, double heatCapacity,
                      double heatCapacityError, double obs, double obsErr,
                      double obsSuscPart, double obsSuscPartErr,
                      double susc, double suscErr,
                      double binder, double binderErr,
                      double binderRatio, double binderRatioErr) :
        energyAvg(energy), energyError(energyErr), heatCapacity(heatCapacity),
        heatCapacityError(heatCapacityError), obsAvg(obs),
        obsError(obsErr), obsSuscPart(obsSuscPart), obsSuscPartError(obsSuscPartErr),
        obsSusc(susc), obsSuscError(suscErr),
        obsBinder(binder), obsBinderError(binderErr),
        obsBinderRatio(binderRatio), obsBinderRatioError(binderRatioErr),        
        energyHistogram(0), obsHistogram(0)
    { }

    void freeMemory() {
        destroy(energyHistogram);
        destroy(obsHistogram);
    }

};


#endif /* REWEIGHTINGRESULT_H_ */
