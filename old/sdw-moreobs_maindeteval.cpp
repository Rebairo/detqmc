/*
 * deteval.cpp
 *
 *  Created on: Apr 29, 2013
 *      Author: gerlach
 */

// Evaluate time series generated by detqmc.
// Call in directory containing timeseries files.

#include <iostream>
#include <algorithm>
#include <iterator>
#include <memory>
#include <map>
#include <cmath>
#include <vector>
#include <string>
#include "boost/program_options.hpp"
#include "git-revision.h"
#include "tools.h"                      //glob
#include "dataseriesloader.h"
#include "datamapwriter.h"
#include "metadata.h"
#include "exceptions.h"
#include "statistics.h"

int main(int argc, char **argv) {
    uint32_t discard = 0;
    uint32_t subsample = 1;
    uint32_t jkBlocks = 1;
    bool notau = false;
    bool noexp = false;


    //parse command line options
    namespace po = boost::program_options;
    po::options_description evalOptions("Time series evaluation options");
    evalOptions.add_options()
            ("help", "print help on allowed options and exit")
            ("version,v", "print version information (git hash, build date) and exit")
            ("discard,d", po::value<uint32_t>(&discard)->default_value(0),
                    "number of initial time series entries to discard (additional thermalization)")
            ("subsample,s", po::value<uint32_t>(&subsample)->default_value(1),
                    "take only every s'th sample into account")
            ("jkblocks,j", po::value<uint32_t>(&jkBlocks)->default_value(1),
                    "number of jackknife blocks to use")
            ("notau", po::bool_switch(&notau)->default_value(false),
                    "switch of estimation of integrated autocorrelation times")
            ("noexp", po::bool_switch(&noexp)->default_value(false),
                    "switch of estimation of expectation values and errorbars")
            ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, evalOptions), vm);
    po::notify(vm);
    bool earlyExit = false;
    if (vm.count("help")) {
        std::cout << "Evaluate time series generated by detqmc.  Call in directory containing timeseries files.\n"
                  << "Will write results to files eval-results.values and eval-tauint.values\n\n"
                  << evalOptions << std::endl;
        earlyExit = true;
    }
    if (vm.count("version")) {
        std::cout << "Build info:\n"
                  << metadataToString(collectVersionInfo())
                  << std::endl;
        earlyExit = true;
    }
    if (earlyExit) {
        return 0;
    }

    //take simulation metadata from file info.dat, remove some unnecessary parts
    MetadataMap meta = readOnlyMetadata("info.dat");
    std::string keys[] = {"buildDate", "buildHost", "buildTime",
            "cppflags", "cxxflags", "gitBranch", "gitRevisionHash",
            "sweepsDone", "sweepsDoneThermalization", "totalWallTimeSecs"};
    for ( std::string key : keys) {
        if (meta.count(key)) {
            meta.erase(key);
        }
    }
    uint32_t guessedLength = static_cast<uint32_t>(fromString<double>(meta.at("sweeps")) /
            fromString<double>(meta.at("measureInterval")));

    //Store averages / nonlinear estimates, jackknife errors,
    //integrated autocorrelation times here
    //key: observable name
    typedef std::map<std::string, double> ObsValMap;
    ObsValMap estimates, errors, tauints;
    //jackknife-block wise estimates
    typedef std::map<std::string, std::vector<double>> ObsVecMap;
    ObsVecMap jkBlockEstimates;

    uint32_t evalSamples = 0;

    //process time series files
    std::vector<std::string> filenames = glob("*.series");
    for (std::string fn : filenames) {
        std::cout << "Processing " << fn << ", ";
        DoubleSeriesLoader reader;
        reader.readFromFile(fn, subsample, discard, guessedLength);
        if (reader.getColumns() != 1) {
            throw GeneralError("File " + fn + " does not have exactly 1 column");
        }

        std::vector<double>* data = reader.getData();       //TODO: smart pointers!
        std::string obsName;
        reader.getMeta("observable", obsName);      //TODO: change class to yield return values, not output parameters
        std::cout << "observable: " << obsName << "..." << std::flush;

        if (not noexp) {
            estimates[obsName] = average(*data);
            jkBlockEstimates[obsName] = jackknifeBlockEstimates(*data, jkBlocks);
            if (obsName == "normPhi") {
                using std::pow;
                estimates["normPhiSquared"] = average<double>( [](double v) { return pow(v, 2); }, *data);
                jkBlockEstimates["normPhiSquared"] = jackknifeBlockEstimates<double>(
                        [](double v) { return pow(v, 2); },
                        *data, jkBlocks );
                estimates["normPhiFourth"] = average<double>( [](double v) { return pow(v, 4); }, *data);
                jkBlockEstimates["normPhiFourth"] = jackknifeBlockEstimates<double>(
                        [](double v) { return pow(v, 4); },
                        *data, jkBlocks );
                estimates["normPhiBinder"] = 1.0 - (3.0*estimates["normPhiFourth"]) /
                        (5.0*pow(estimates["normPhiSquared"], 2));
                jkBlockEstimates["normPhiBinder"] = std::vector<double>(jkBlocks, 0);
                for (uint32_t jb = 0; jb < jkBlocks; ++jb) {
                    jkBlockEstimates["normPhiBinder"][jb] =
                            1.0 - (3.0*jkBlockEstimates["normPhiFourth"][jb]) /
                            (5.0*pow(jkBlockEstimates["normPhiSquared"][jb], 2));
                }
            }
        }
//      std::copy(std::begin(jkBlockEstimates[obsName]), std::end(jkBlockEstimates[obsName]), std::ostream_iterator<double>(std::cout, " "));
//      std::cout << std::endl;
//      std::cout << average(jkBlockEstimates[obsName]);

        if (not notau) {
            tauints[obsName] = tauint(*data);
        }

        evalSamples = data->size();

        reader.deleteData();

        std::cout << std::endl;
    }

    // Compute observables combined from multiple time series
    if (meta["model"] == "sdw") {
    	std::cout << "Computing combined observables: ";
    	// 1) occupation number
    	auto addOcc = [&](const std::string& occRes, const std::string& occ1, const std::string& occ2) -> void {
    		estimates[occRes] = estimates.at(occ1) + estimates.at(occ2);
    		jkBlockEstimates[occRes] = jkBlockEstimates.at(occ1);
    		for (std::size_t i = 0; i < jkBlockEstimates.at(occRes).size(); ++i) {
    			jkBlockEstimates.at(occRes)[i] += jkBlockEstimates.at(occ2)[i];
    		}
    		std::cout << occRes << ", ";
    	};
    	addOcc("occX", "occXUp", "occXDown");
    	addOcc("occY", "occYUp", "occYDown");
    	addOcc("occUp", "occXUp", "occYUp");
    	addOcc("occDown", "occXDown", "occYDown");
    	addOcc("occ", "occX", "occY");
    	// 2) spin/band resolved double occupancy
    	auto addDoubleOcc = [&](const std::string& doubleOccRes,
    			const std::string& doubleOcc1, const std::string& doubleOcc2,
    			const std::string& doubleOcc3, const std::string& doubleOcc4) -> void {
    		estimates[doubleOccRes] = estimates.at(doubleOcc1) + estimates.at(doubleOcc2)
    			                		+ estimates.at(doubleOcc3) + estimates.at(doubleOcc4);
    		jkBlockEstimates[doubleOccRes] = jkBlockEstimates.at(doubleOcc1);
    		for (std::size_t i = 0; i < jkBlockEstimates.at(doubleOccRes).size(); ++i) {
    			jkBlockEstimates.at(doubleOccRes)[i] += jkBlockEstimates.at(doubleOcc2)[i]
    			                                      + jkBlockEstimates.at(doubleOcc3)[i]
    			                                      + jkBlockEstimates.at(doubleOcc4)[i];
    		}
    		std::cout << doubleOccRes << ", ";
    	};
    	addDoubleOcc("doubleOccXY", "doubleOccXUpYUp", "doubleOccXUpYDown", "doubleOccXDownYUp", "doubleOccXDownYDown");
    	addDoubleOcc("doubleOccUpDown", "doubleOccXUpXDown", "doubleOccXUpYDown", "doubleOccXDownYUp", "doubleOccYUpYDown");
    	// attention for XX, YY, UpUp, DownDown double occupancy: Use occupation at some points
    	addDoubleOcc("doubleOccXX", "occXUp", "doubleOccXUpXDown", "doubleOccXUpXDown", "occXDown");
    	addDoubleOcc("doubleOccYY", "occYUp", "doubleOccYUpYDown", "doubleOccYUpYDown", "occYDown");
    	addDoubleOcc("doubleOccUpUp", "occXUp", "doubleOccXUpYUp", "doubleOccXUpYUp", "occYUp");
    	addDoubleOcc("doubleOccDownDown", "occXDown", "doubleOccXDownYDown", "doubleOccXDownYDown", "occYDown");
    	// 3) squared local moment
    	auto computeSqLocMom = [&](const std::string& bs1, const std::string& bs2) -> void {
    		std::string bsc = bs1 + bs2;
    		std::string sqlm = "sqLocMom" + bsc;
    		std::string occ1 = "occ" + bs1;
    		std::string occ2 = "occ" + bs2;
    		std::string doubleOcc = "doubleOcc" + bs1 + bs2;
    		estimates[sqlm] = estimates.at(occ1) - 2.0*estimates.at(doubleOcc) + estimates.at(occ2);
    		jkBlockEstimates[sqlm] = jkBlockEstimates.at(occ1);
    		for (std::size_t i = 0; i < jkBlockEstimates.at(sqlm).size(); ++i) {
    			jkBlockEstimates.at(sqlm)[i] += jkBlockEstimates.at(occ2)[i]
    			                               - 2.0 * jkBlockEstimates.at(doubleOcc)[i];
    		}
    		std::cout << sqlm << ", ";
    	};
    	computeSqLocMom("XUp", "XDown");
    	computeSqLocMom("YUp", "YDown");
    	computeSqLocMom("XUp", "YUp");
    	computeSqLocMom("XDown", "YDown");
    	computeSqLocMom("XUp", "YDown");
    	computeSqLocMom("XDown", "YUp");
    	auto computeSqLocMomAdded = [&](const std::string& bs1, const std::string& bs2) -> void {
    		std::string bsc = bs1 + bs2;
    		std::string sqlm = "sqLocMom" + bsc;
    		std::string docc1 = "doubleOcc" + bs1 + bs1;
    		std::string docc2 = "doubleOcc" + bs2 + bs2;
    		std::string doccmix = "doubleOcc" + bs1 + bs2;
    		estimates[sqlm] = estimates.at(docc1) - 2.0*estimates.at(doccmix) + estimates.at(docc2);
    		jkBlockEstimates[sqlm] = jkBlockEstimates.at(docc1);
    		for (std::size_t i = 0; i < jkBlockEstimates.at(sqlm).size(); ++i) {
    			jkBlockEstimates.at(sqlm)[i] += jkBlockEstimates.at(docc2)[i]
    			                               - 2.0 * jkBlockEstimates.at(doccmix)[i];
    		}
    		std::cout << sqlm << ", ";
    	};
    	computeSqLocMomAdded("X", "Y");
    	computeSqLocMomAdded("Up", "Down");
    	// 4) (staggered) magnetic structure factor for wave vectors (0,0) and (pi,pi)
    	auto computeStructureFactor = [&](const std::string& base) -> void {
    		std::string out = base;
    		std::string xx = base + "XX";
    		std::string xy = base + "XY";
    		std::string yy = base + "YY";
    		estimates[out] = estimates.at(xx) + 2.0*estimates.at(xy) + estimates.at(yy);
    		jkBlockEstimates[out] = jkBlockEstimates.at(xx);
    		for (std::size_t i = 0; i < jkBlockEstimates.at(out).size(); ++i) {
    			jkBlockEstimates.at(out)[i] += jkBlockEstimates.at(yy)[i]
    			                            + 2.0 * jkBlockEstimates.at(xy)[i];
    		}
    		std::cout << out << ", ";
    	};
    	computeStructureFactor("magStruct00");
    	computeStructureFactor("magStructPiPi");
    	computeStructureFactor("staggeredMagStruct00");
    	computeStructureFactor("staggeredMagStructPiPi");
    	// all combined observables
    	std::cout << "OK" << std::endl;
    }

    //calculate error bars from jackknife block estimates
    if (not noexp and jkBlocks > 1) {
        for (auto const& nameBlockPair : jkBlockEstimates) {
            const std::string obsName = nameBlockPair.first;
            const std::vector<double> blockEstimates = nameBlockPair.second;
            errors[obsName] = jackknife(blockEstimates, estimates[obsName]);
        }
    }

    if (not noexp) {
        StringDoubleMapWriter resultsWriter;
        resultsWriter.addMetadataMap(meta);
        resultsWriter.addMeta("eval-jackknife-blocks", jkBlocks);
        resultsWriter.addMeta("eval-discard", discard);
        resultsWriter.addMeta("eval-subsample", subsample);
        resultsWriter.addMeta("eval-samples", evalSamples);
        if (jkBlocks > 1) {
            resultsWriter.addHeaderText("Averages and jackknife error bars computed from time series");
            resultsWriter.setData(std::make_shared<ObsValMap>(estimates));
            resultsWriter.setErrors(std::make_shared<ObsValMap>(errors));
        } else {
            resultsWriter.addHeaderText("Averages computed from time series");
            resultsWriter.setData(std::make_shared<ObsValMap>(estimates));
        }
        resultsWriter.writeToFile("eval-results.values");
    }

    if (not notau) {
        StringDoubleMapWriter tauintWriter;
        tauintWriter.addMetadataMap(meta);
        tauintWriter.addMeta("eval-discard", discard);
        tauintWriter.addMeta("eval-subsample", subsample);
        tauintWriter.addMeta("eval-samples", evalSamples);
        tauintWriter.addHeaderText("Tauint estimates computed from time series");
        tauintWriter.setData(std::make_shared<ObsValMap>(tauints));
        tauintWriter.writeToFile("eval-tauint.values");
    }

    std::cout << "Done!" << std::endl;

    return 0;
}