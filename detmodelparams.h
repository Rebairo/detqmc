#ifndef DETMODELPARAMS_H
#define DETMODELPARAMS_H

#include <string>
#include <set>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include "boost/serialization/string.hpp"
#include "boost/serialization/set.hpp"
#pragma GCC diagnostic pop            

// Collect various structs defining various parameters.

// The set specified included in each struct contains string representations
// of all parameters actually specified.  This allows throwing an exception
// at the appropriate point in the program if a parameter is missing.


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
typedef double num;     //possibility to switch to single precision if ever desired
#pragma GCC diagnostic pop


// Template for struct representing model specific parameters
// -- needs to have a proper specialization for each model considered, which actually
//    implements the functions and provides data members
// for a class derived of DetModelGC this should at least be beta, m, s, dtau
template<class Model>
struct ModelParams {
    void check() { }
    MetadataMap prepareMetadataMap() { return MetadataMap(); }
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const uint32_t version) {
        (void)version; (void)ar;
    }    
};


// specializations in separate header files: DetHubbardParams, DetSDWParams

struct ModelParams {
    //TODO: split this up so that model specific parameters are in substructs

    std::string model;
//    bool timedisplaced;     //also evaluate timedisplaced Green functions and derived quantities
    bool checkerboard;      //SDW: use a checkerboard decomposition for computing the propagator
    std::string updateMethod;			//SDW: "iterative", "woodbury", or "delayed"
    std::string spinProposalMethod;		//SDW: "box", "rotate_then_scale", or "rotate_and_scale"
    bool adaptScaleVariance;			//SDW: valid unless spinProposalMethod=="box" -- this controls if the variance of the spin updates should be adapted during thermalization
    uint32_t delaySteps;		//SDW: parameter in case updateMethod is "delayed"
    num t;      //Hubbard
    num U;      //Hubbard
    num r;      //SDW
    num lambda;	//SDW: fermion-boson coupling strength
    num txhor;  //SDW: hopping constants depending on direction and band
    num txver;  //SDW
    num tyhor;  //SDW
    num tyver;  //SDW
    num cdwU;	//SDW -- hoping to get a CDW transition
    num mu;
    uint32_t L;
    uint32_t d;
    num beta;
    uint32_t m;     //either specify number of timeslices 'm'
    num dtau;       //or timeslice separation 'dtau'
    uint32_t s;     //separation of timeslices where the Green function is calculated
                    //from scratch
    num accRatio;   //for SDW: target acceptance ratio for tuning spin update box size

    std::string bc; //boundary conditions: For SDW: "pbc", "apbc-x", "apbc-y" or "apbc-xy"

    //SDW:
    uint32_t globalUpdateInterval; //attempt global move every # sweeps
    bool globalShift;         //perform a global constant shift move?
    bool wolffClusterUpdate;  //perform a Wolff single cluster update?
    bool wolffClusterShiftUpdate; // perform a combined global constant shift and Wolff single cluster update

    //SDW:
    uint32_t repeatUpdateInSlice;	//how often to repeat updateInSlice for eacht timeslice per sweep, default: 1

    std::set<std::string> specified;

    ModelParams() :
        model(), /*timedisplaced(),*/ checkerboard(),
        updateMethod(), spinProposalMethod(), adaptScaleVariance(),
        delaySteps(),
        t(), U(), r(), lambda(),
        txhor(), txver(), tyhor(), tyver(),
        mu(), L(), d(),
        beta(), m(), dtau(), s(), accRatio(), bc("pbc"),
        globalUpdateInterval(), globalShift(), wolffClusterUpdate(),
        wolffClusterShiftUpdate(),
        repeatUpdateInSlice(),
        specified() {
    }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const uint32_t version) {
        (void)version;
        ar  & model & checkerboard /*& timedisplaced*/& updateMethod
            & spinProposalMethod & adaptScaleVariance & delaySteps
            & t & U & r & lambda & txhor & txver & tyhor & tyver
            & cdwU
            & mu & L & d & beta & m & dtau & s & accRatio & bc
            & globalUpdateInterval & globalShift & wolffClusterUpdate
            & wolffClusterShiftUpdate
            & repeatUpdateInSlice
            & specified;
    }
};


#endif /* DETMODELPARAMS_H */
