#ifndef DETHUBBARDPARAMS_H
#define DETHUBBARDPARAMS_H

#include <string>
#include <set>
#include "metadata.h"
#include "exceptions.h"
#include "detmodelparams.h"


class DetHubbard;


template<>
struct ModelParams<DetHubbard>> {
    std::string model;          // should be hubbard

    bool checkerboard;

    num t;
    num U;
    num mu;

    uint32_t L;
    uint32_t d;
    num beta;
    uint32_t m;     //either specify number of timeslices 'm'
    num dtau;       //or timeslice separation 'dtau'
    uint32_t s;     //separation of timeslices where the Green function is calculated
                    //from scratch
    std::string bc;

    std::set<std::string> specified;

    ModelParams() :
        model("hubbard"), checkerboard(),
        t(), U(), mu(), L(), d(), beta(), m(), dtau(), s(), bc(),
        specified()
    { }
    
    void check();
    MetadataMap prepareMetaDataMap() const;
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const uint32_t version) {
        (void)version;
        ar  & model & checkerboard
            t & U & mu & L & d & beta & m
            & dtau & s & bc
            & specified;            
    }    
};

#endif /* DETHUBBARDPARAMS_H */
