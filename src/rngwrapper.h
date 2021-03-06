/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  See the enclosed file LICENSE for a copy or if
 * that was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2017 Max H. Gerlach
 * 
 * */


/*
 * rngwrapper.h
 *
 *  Created on: Oct 12, 2011
 *      Author: gerlach
 *
 */

// C++ Wrapper around DSFMT random number generator

#ifndef RNGWRAPPER_H_
#define RNGWRAPPER_H_

#include <string>
extern "C" {
#include "dsfmt/dSFMT.h"
}

#include <tuple>
#include <cmath>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include "boost/serialization/string.hpp"
#include "boost/serialization/access.hpp"
#include "boost/serialization/export.hpp"
#include "boost/serialization/split_member.hpp"
#pragma GCC diagnostic pop

class RngWrapper {
    uint32_t seed;
    uint32_t processIndex;
    dsfmt_t dsfmt;
public:
    RngWrapper(uint32_t seed_ = 0, uint32_t processIndex_ = 0);
    virtual ~RngWrapper() { }

    std::string getName() const;

    //return a floating point random number from (0, 1), uniformly distributed
    double rand01() {
        //dSFTM
        return dsfmt_genrand_open_open(&dsfmt);
    }

    //return a floating point random number from (low, high), uniformly distributed
    double randRange(double low, double high) {
        return low + (high - low) * rand01();
    }

    //return an integer random number from the discrete uniform distribution over the integers
    //{low, low + 1, . . . , high}. One call to rand01()
    int randInt(int low, int high) {
        return low + static_cast<int> ((high - low + 1.0) * rand01());
    }

    std::tuple<double,double,double> randPointOnSphere() {
    	double phi = randRange(0., 2.*M_PI);
    	double costheta = randRange(-1., 1.0);
    	double sintheta = std::sqrt(1. - costheta*costheta);
    	double cosphi = std::cos(phi);
    	double sinphi = std::sin(phi);
    	double x = cosphi * sintheta;
    	double y = sinphi * sintheta;
    	double z = costheta;
    	return std::make_tuple(x, y, z);
    }

    std::tuple<double,double> randPointOnCircle() {
    	double phi = randRange(0., 2.*M_PI);
    	double x = std::cos(phi);
    	double y = std::sin(phi);
        return std::make_tuple(x, y);
    }

    //saving/loading state without Boost serialization
    void saveState() const;
    void loadState();

private:
    //for serialization with Boost
    friend class boost::serialization::access;

    std::string stateToString() const;
    void stringToState(const std::string& stateString);

    template<class Archive>
    void save(Archive& ar, const uint32_t /* version */) const {
        ar << seed << processIndex;
        std::string stateString = stateToString();
        ar << stateString;
    }

    template<class Archive>
    void load(Archive& ar, const uint32_t /* version */) {
        ar >> seed >> processIndex;
        std::string stateString;
        ar >> stateString;
        stringToState(stateString);

        std::cout << "Deserialized RNG wrapper from saved state -- original seed was: " << seed
        		  << ", processIndex: " << processIndex << std::endl;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

#endif /* RNGWRAPPER_H_ */
