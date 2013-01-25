/*
 * dethubbard.cpp
 *
 *  Created on: Dec 3, 2012
 *      Author: gerlach
 */

#include <vector>
#include <cmath>
#include <cassert>
#include <boost/assign/std/vector.hpp>    // 'operator+=()' for vectors
#include "tools.h"
#include "exceptions.h"
#include "rngwrapper.h"
#include "dethubbard.h"
#include "parameters.h"

//using std::acosh;    //Intel compiler chokes with std::acosh


void debugSaveMatrix(const MatNum& matrix, const std::string& basename) {
	matrix.save(basename + ".csv", arma::csv_ascii);
	matrix.save(basename + ".txt", arma::arma_ascii);
}

void debugSaveMatrix(const MatInt& matrix, const std::string& basename) {
	matrix.save(basename + ".csv", arma::csv_ascii);
	matrix.save(basename + ".txt", arma::arma_ascii);
}


std::unique_ptr<DetHubbard> createDetHubbard(RngWrapper& rng, const ModelParams& pars) {
	//check parameters
	using namespace boost::assign;
	std::vector<std::string> neededModelPars;
	neededModelPars += "t", "U", "mu", "L", "d", "beta", "m";
	for (auto p = neededModelPars.cbegin(); p != neededModelPars.cend(); ++p) {
		if (pars.specified.count(*p) == 0) {
			throw ParameterMissing(*p);
		}
	}

	return std::unique_ptr<DetHubbard>(new DetHubbard(rng,
			pars.t, pars.U, pars.mu, pars.L, pars.d, pars.beta, pars.m)	);
}



DetHubbard::DetHubbard(RngWrapper& rng_,
		num t, num U, num mu, unsigned L, unsigned d, num beta, unsigned m) :
		rng(rng_),
		t(t), U(U), mu(mu), L(L), d(d),
		z(2*d), //coordination number: 2*d
		N(static_cast<unsigned>(uint_pow(L,d))),
		beta(beta), m(m), dtau(beta/m),
		alpha(acosh(std::exp(dtau * U * 0.5))),
		nearestNeigbors(z, N),
		proptmat(N,N),
		auxfield(N, m),              //m columns of N rows
		gUp(N,N,m), gDn(N,N,m),      //m slices of N columns x N rows
		gFwdUp(N,N,m), gFwdDn(N,N,m),
		gBwdUp(N,N,m), gBwdDn(N,N,m),
		obsNames(), obsShorts(), obsValRefs(), obsCount(0)
{
	createNeighborTable();
	setupRandomAuxfield();
	setupPropTmat();
	setupUdVStorage();
	lastSweepDir = SweepDirection::Up;		//first sweep will be downwards

	using namespace boost::assign;         // bring operator+=() into scope
	using std::cref;
	obsNames += "occupationUp", "occupationDown", "totalOccupation",
			"doubleOccupation", "localMoment",
			"kineticEnergy", "potentialEnergy", "totalEnergy", "susceptibilityQ0";
	obsShorts += "nUp", "nDown", "n", "n2", "m^2", "e_t", "e_U", "e", "chi_q0";
	obsValRefs += cref(occUp), cref(occDn), cref(occTotal), cref(occDouble), cref(localMoment),
			cref(eKinetic), cref(ePotential), cref(eTotal), cref(suscq0);
	assert(obsNames.size() == obsShorts.size());
	assert(obsNames.size() == obsValRefs.size());
	obsCount = obsNames.size();

	vecObsNames += "spinzCorrelationFunction";
	vecObsShorts += "zcorr";
	vecObsValRefs += cref(zcorr);
	assert(vecObsNames.size() == vecObsShorts.size());
	assert(vecObsNames.size() == vecObsValRefs.size());
	vecObsCount = vecObsNames.size();

	zcorr.zeros(N);
}

DetHubbard::~DetHubbard() {
}


MetadataMap DetHubbard::prepareModelMetadataMap() {
	MetadataMap meta;
	meta["model"] = "hubbard";
#define META_INSERT(VAR) meta[#VAR] = numToString(VAR)
	META_INSERT(t);
	META_INSERT(U);
	META_INSERT(mu);
	META_INSERT(L);
	META_INSERT(d);
	META_INSERT(N);
	META_INSERT(beta);
	META_INSERT(m);
	META_INSERT(dtau);
	META_INSERT(alpha);
#undef META_INSERT
	return meta;
}

void DetHubbard::updateInSlice(unsigned timeslice) {
	// picking sites linearly: system seemed to alternate between two configurations
	// sweep after sweep
//	for (unsigned site = 0; site < N; ++site) {
	for (unsigned count = 0; count < N; ++count) {
		unsigned site = rng.randInt(0, N-1);

		num ratio =  weightRatioSingleFlip(site, timeslice);

//		//DEBUG: comparison of weight ratio calculation
//		MatInt newAuxfield = auxfield;
//		newAuxfield(site, timeslice - 1) *= -1;
//		num refRatio = weightRatioGenericNaive(auxfield, newAuxfield);
//		std::cout << ratio << " vs. " << refRatio << std::endl;

		assert(ratio > 0.0);

		//Metropolis
		if (ratio > 1.0 or rng.rand01() < ratio) {
			//			if (refRatio > 1 or rng.rand01() < refRatio) {
			//				std::cout << "acc" << '\n';
			//				auxfield = newAuxfield;

			updateGreenFunctionWithFlip(site, timeslice);
			auxfield(site, timeslice-1) *= -1;
		}
	}
}


void DetHubbard::sweepSimple() {
	for (unsigned timeslice = 1; timeslice <= m; ++timeslice) {
		gUp.slice(timeslice-1) = computeGreenFunctionNaive(timeslice, Spin::Up);
		gDn.slice(timeslice-1) = computeGreenFunctionNaive(timeslice, Spin::Down);
		updateInSlice(timeslice);
	}
}

DetHubbard::UdV DetHubbard::svd(const MatNum& mat) {
	UdV result;
	MatNum V_transpose;
	arma::svd(result.U, result.d, V_transpose, mat, "standard");
	result.V = V_transpose.t();			//potentially it may be advisable to not do this generally
//	result.U = mat;
//	result.d = arma::ones(N);
//	result.V = arma::eye(N,N);
	return result;
}

unsigned DetHubbard::getSystemN() const {
	return N;
}

DetHubbard::MatNum4 DetHubbard::greenFromUdV_timedisplaced(
		const UdV& UdV_l, const UdV& UdV_r) const {
	//Ul vs Vl to be compatible with labeling in the notes
	const MatNum& Ul = UdV_l.V;   //!
	const VecNum& dl = UdV_l.d;
	const MatNum& Vl = UdV_l.U;   //!
	const MatNum& Ur = UdV_r.U;
	const VecNum& dr = UdV_r.d;
	const MatNum& Vr = UdV_r.V;

	//submatrix view helpers for 2*N x 2*N matrices
	//#define upleft(m) m.submat(0,0, N-1,N-1)
	//#define upright(m) m.submat(0,N, N-1,2*N-1)
	//#define downleft(m) m.submat(N,0, 2*N-1,N-1)
	//#define downright(m) m.submat(N,N, 2*N-1,2*N-1)
	auto upleft = [N](MatNum& m) {
		return m.submat(0,0, N-1,N-1);
	};
	auto upright = [N](MatNum& m) {
		return m.submat(0,N, N-1,2*N-1);
	};
	auto downleft = [N](MatNum& m) {
		return m.submat(N,0, 2*N-1,N-1);
	};
	auto downright = [N](MatNum& m) {
		return m.submat(N,N, 2*N-1,2*N-1);
	};

	MatNum temp(2*N,2*N);
	upleft(temp)    = arma::inv(Vr * Vl);
	upright(temp)   = arma::diagmat(dl);
	downleft(temp)  = arma::diagmat(-dr);
	downright(temp) = arma::inv(Ul * Ur);
	UdV tempUdV = svd(temp);

	MatNum left(2*N,2*N);
	upleft(left) = arma::inv(Vr);
	upright(left).zeros();
	downleft(left).zeros();
	downright(left) = arma::inv(Ul);

	MatNum right(2*N,2*N);
	upleft(right) = arma::inv(Vl);
	upright(right).zeros();
	downleft(right).zeros();
	downright(right) = arma::inv(Ur);

	MatNum result = (left * arma::inv(tempUdV.V)) * arma::diagmat(1.0 / tempUdV.d)
					* (arma::inv(tempUdV.U) * right);
	return MatNum4(upleft(result), upright(result),
				   downleft(result), downright(result));
}

MatNum DetHubbard::greenFromUdV(const UdV& UdV_l, const UdV& UdV_r) const {
	//variable names changed according to labeling in names
	const MatNum& V_l = UdV_l.U;   //!
	const VecNum& d_l = UdV_l.d;
	const MatNum& U_l = UdV_l.V;   //!
	const MatNum& U_r = UdV_r.U;
	const VecNum& d_r = UdV_r.d;
	const MatNum& V_r = UdV_r.V;

	//check if alternative method for stable calculation works better:

	using arma::inv; using arma::diagmat; using arma::eye;

	UdV UdV_temp = svd( inv(U_l * U_r) + diagmat(d_r) * (V_r * V_l) * diagmat(d_l) );

	MatNum green = inv(UdV_temp.V * U_l) * diagmat(1.0 / UdV_temp.d) * inv(U_r * UdV_temp.U);

	return green;
}



void DetHubbard::debugCheckBeforeSweepDown() {
	std::cout << "Before sweep down:\n";
	std::cout << "up: ";
	for (unsigned timeslice = 1; timeslice <= m; ++timeslice) {
		UdV udv = UdVStorageUp[timeslice];
		MatNum diff = computeBmatNaive(timeslice, 0, Spin::Up) - udv.U * arma::diagmat(udv.d) * udv.V;
		std::cout << diff.max() << " ";
	}
	std::cout << "\n";
	std::cout << "down: ";
	for (unsigned timeslice = 1; timeslice <= m; ++timeslice) {
		UdV udv = UdVStorageDn[timeslice];
		MatNum diff = computeBmatNaive(timeslice, 0, Spin::Down) - udv.U * arma::diagmat(udv.d) * udv.V;
		std::cout << diff.max() << " ";
	}
	std::cout << "\n\n";
}

void DetHubbard::debugCheckBeforeSweepUp() {
	std::cout << "Before sweep up:\n";
	std::cout << "up: ";
	for (unsigned timeslice = 0; timeslice <= m; ++timeslice) {
		UdV udv = UdVStorageUp[timeslice];
		MatNum diff = computeBmatNaive(m, timeslice, Spin::Up) - udv.U * arma::diagmat(udv.d) * udv.V;
		std::cout << diff.max() << " ";
	}
	std::cout << "\n";
	std::cout << "down: ";
	for (unsigned timeslice = 0; timeslice <= m; ++timeslice) {
		UdV udv = UdVStorageDn[timeslice];
		MatNum diff = computeBmatNaive(m, timeslice, Spin::Down) - udv.U * arma::diagmat(udv.d) * udv.V;
		std::cout << diff.max() << " ";
	}
	std::cout << "\n\n";
}



void DetHubbard::sweep() {
	using std::tie; using std::ignore; using std::get;
	if (lastSweepDir == SweepDirection::Up) {
//		debugCheckBeforeSweepDown();
		//to compute green function for timeslice tau=beta:
		//we need VlDlUl = B(beta, beta) = I and UrDrVr = B(beta, 0).
		//The latter is given in storage slice m from the last sweep.
		tie(ignore, gBwdUp.slice(m - 1), gFwdUp.slice(m - 1), gUp.slice(m - 1)) =
				greenFromUdV_timedisplaced(eye_UdV, UdVStorageUp[m]);
		tie(ignore, gBwdDn.slice(m - 1), gFwdDn.slice(m - 1), gDn.slice(m - 1)) =
				greenFromUdV_timedisplaced(eye_UdV, UdVStorageDn[m]);

		UdVStorageUp[m] = eye_UdV;
		UdVStorageDn[m] = eye_UdV;
		for (unsigned k = m; k >= 1; --k) {
			updateInSlice(k);
			//update the green function in timeslice k-1:
			auto advanceDownGreen = [this](unsigned k, std::vector<UdV>& storage,
					                       CubeNum& green, CubeNum& greenFwd,
					                       CubeNum& greenBwd, Spin spinz) {
//				debugSaveMatrix(auxfield, "auxfield");
				MatNum B_k = computeBmatNaive(k, k - 1, spinz);
//				debugSaveMatrix(B_k, "b_" + numToString(k) + "_" + numToString(k - 1) + "_s"
//						+ numToString(int(spinz)));

				//U_k, d_k, V_k correspond to B(beta,k*tau) [set in the last step]
				const MatNum& U_k = storage[k].U;
				const VecNum& d_k = storage[k].d;
				const MatNum& V_k = storage[k].V;

				//UdV_L will correspond to B(beta,(k-1)*tau)
				UdV UdV_L = svd(arma::diagmat(d_k) * (V_k * B_k));
				UdV_L.U = U_k * UdV_L.U;

				//UdV_R corresponds to B((k-1)*tau,0) [set in last sweep]
				const UdV& UdV_R = storage[k - 1];

				if (k - 1 > 0) {      //TODO: this if handled correctly?
					unsigned next = k - 1 - 1;
					tie(ignore, greenBwd.slice(next), greenFwd.slice(next), green.slice(next)) =
							greenFromUdV_timedisplaced(UdV_L, UdV_R);
				}

				storage[k - 1] = UdV_L;
			};
			advanceDownGreen(k, UdVStorageUp, gUp, gFwdUp, gBwdUp, Spin::Up);
			advanceDownGreen(k, UdVStorageDn, gDn, gFwdDn, gBwdDn, Spin::Down);
		}
		lastSweepDir = SweepDirection::Down;
	} else if (lastSweepDir == SweepDirection::Down) {
//		debugCheckBeforeSweepUp();
		//The Green function at tau=0 is equal to that at tau=beta.
		//Here we compute it for tau=0 and store it in the slice for tau=beta.
//		gUp.slice(m - 1) = get<3>(greenFromUdV(UdVStorageUp[0], eye_UdV));     //necessary?
//		gUp.slice(m - 1) = get<3>(greenFromUdV(UdVStorageDn[0], eye_UdV));
		//proceed like Assaad
		UdVStorageUp[0] = eye_UdV;
		UdVStorageDn[0] = eye_UdV;
		for (unsigned k = 0; k <= m - 1; ++k) {
			//update the green function in timeslice k+1
			auto advanceUpGreen = [this](unsigned k, const std::vector<UdV>& storage,
											CubeNum& green, CubeNum& greenFwd,
											CubeNum& greenBwd, Spin spinz) {
				MatNum B_kp1 = computeBmatNaive(k + 1, k, spinz);

				//The following is B(beta, (k+1) tau), valid from the last sweep
				const UdV& UdV_kp1 = storage[k + 1];

				//from the last step the following are B(k*tau, 0):
				const MatNum& U_k = storage[k].U;
				const VecNum& d_k = storage[k].d;
				const MatNum& V_k = storage[k].V;

				//UdV_temp will be the new B((k+1)*tau, 0):
				UdV UdV_temp = svd(((B_kp1 * U_k) * arma::diagmat(d_k)));
				UdV_temp.V *= V_k;

				unsigned next = k + 1 - 1;
				tie(ignore, greenBwd.slice(next), greenFwd.slice(next), green.slice(next)) =
						greenFromUdV_timedisplaced(UdV_kp1, UdV_temp);

				//storage[k + 1] = UdV_temp;    //storage will be wrong after updateInSlice!
			};
			//Given B(k*tau, 0) from the last step in the storage, compute
			//B((k+1)*tau, 0) and put it into storage
			auto updateAdvanceStorage = [this](unsigned k, std::vector<UdV>& storage,
					Spin spinz) {
				MatNum B_kp1 = computeBmatNaive(k + 1, k, spinz);
				//from the last step the following are B(k*tau, 0):
				const MatNum& U_k = storage[k].U;
				const VecNum& d_k = storage[k].d;
				const MatNum& V_k = storage[k].V;
				//the new B((k+1)*tau, 0):
				storage[k+1] = svd(((B_kp1 * U_k) * arma::diagmat(d_k)));
				storage[k+1].V *= V_k;
			};
			advanceUpGreen(k, UdVStorageUp, gUp, gFwdUp, gBwdUp, Spin::Up);
			advanceUpGreen(k, UdVStorageDn, gDn, gFwdDn, gBwdDn, Spin::Down);
			updateInSlice(k + 1);
			updateAdvanceStorage(k, UdVStorageUp, Spin::Up);
			updateAdvanceStorage(k, UdVStorageDn, Spin::Down);
		}
		lastSweepDir = SweepDirection::Up;
	}
}

void DetHubbard::measure() {
	//used to measure occupation:
	num sum_GiiUp = 0;
	num sum_GiiDn = 0;
	//used to measure kinetic energy:
	num sum_GneighUp = 0;
	num sum_GneighDn = 0;
	//used to measure potential energy:
	num sum_GiiUpDn = 0;
	//FORMULA-TEST -- made no difference
//	num sum_doubleoccupancy = 0;
	for (unsigned timeslice = 1; timeslice <= m; ++timeslice) {
		for (unsigned site = 0; site < N; ++site) {
			//use diagonal elements of Green functions:
			sum_GiiUp += gUp(site, site, timeslice - 1);
			sum_GiiDn += gDn(site, site, timeslice - 1);
			sum_GiiUpDn += gUp(site, site, timeslice - 1) * gDn(site, site, timeslice - 1);
			//use nearest neighbor elements of Green functions:
			for (unsigned neighIndex = 0; neighIndex < z; ++neighIndex) {
				unsigned neigh = nearestNeigbors(neighIndex, site);
				sum_GneighUp += gUp(site, neigh, timeslice - 1);
				sum_GneighDn += gDn(site, neigh, timeslice - 1);
			}
			//FORMULA-TEST -- made no difference
//			sum_doubleoccupancy += (1 - gUp(site,site, timeslice - 1))
//								 * (1 - gDn(site,site, timeslice - 1));
		}
	}
	occUp = 1.0 - (1.0 / (N*m)) * sum_GiiUp;
	occDn = 1.0 - (1.0 / (N*m)) * sum_GiiDn;
	occTotal = occUp + occDn;

	//FORMULA-TEST -- made no difference
//	std::cout << (1.0 / (N*m)) * sum_doubleoccupancy;
	occDouble = 1.0 + (1.0 / (N*m)) * (sum_GiiUpDn - sum_GiiUp - sum_GiiDn);
//	std::cout << " vs. " << occDouble << std::endl;

	localMoment = occTotal - 2*occDouble;

//	ePotential = (U / (N*m)) * (sum_GiiUpDn + 0.5 * sum_GiiUp + 0.5 * sum_GiiDn);
	ePotential = U * ( 0.25 + (1.0 / (N*m)) * (sum_GiiUpDn - 0.5 * (sum_GiiUp + sum_GiiDn)) );

	//Note: chemical potential term included in kinetic energy:
	eKinetic   = (t / (N*m)) * (sum_GneighUp + sum_GneighDn) - mu * occTotal;
	eTotal = eKinetic + ePotential;

	//susceptibility
	auto sumTrace = [m](const CubeNum& green) {
		num sum = 0;
		for (unsigned timeslice = 1; timeslice <= m; ++timeslice) {
			sum += arma::trace(green.slice(timeslice - 1));
		}
		return sum;
	};
	num sumTrGreenUp = sumTrace(gUp);
	num sumTrGreenDn = sumTrace(gDn);
	auto sumProdTrace = [m](const CubeNum& green1, const CubeNum& green2) {
		num sum = 0;
		for (unsigned timeslice = 1; timeslice <= m; ++timeslice) {
			sum += arma::trace(green1.slice(timeslice - 1) * green2.slice(timeslice - 1));
		}
		return sum;
	};
	num sumTrGreenDisplacedUp = sumProdTrace(gBwdUp, gFwdUp);
	num sumTrGreenDisplacedDn = sumProdTrace(gBwdDn, gFwdDn);
	num trGreenUp_0 = arma::trace(gUp.slice(m - 1));			//g(beta) = g(0)
	num trGreenDn_0 = arma::trace(gDn.slice(m - 1));
	suscq0 = dtau * (  (trGreenUp_0 - trGreenDn_0) * (sumTrGreenUp - sumTrGreenDn)
					 - (sumTrGreenDisplacedUp + sumTrGreenDisplacedDn)
					);

	// vector observables
	zcorr.zeros();
	for (unsigned timeslice = 1; timeslice <= m; ++timeslice) {
		num gUp_00 = gUp(0,0, timeslice - 1);
		num gDn_00 = gDn(0,0, timeslice - 1);
		zcorr[0] += -2.0 * gUp_00 * gDn_00 + gUp_00 + gDn_00;
		for (unsigned siteJ = 1; siteJ < N; ++siteJ) {
			num gUp_0j = gUp(0,siteJ,     timeslice - 1);
			num gDn_0j = gDn(0,siteJ,     timeslice - 1);
			num gUp_jj = gUp(siteJ,siteJ, timeslice - 1);
			num gDn_jj = gDn(siteJ,siteJ, timeslice - 1);
			using std::pow;
			zcorr[siteJ] += gUp_00 * gUp_jj - gUp_00 * gDn_jj + gDn_00 * gDn_jj - gDn_00 * gUp_jj
					- pow(gUp_0j, 2) - pow(gDn_0j, 2);
		}
	}
	zcorr /= num(m);
}


unsigned DetHubbard::getNumberOfObservables() const {
	return obsCount;
}

num DetHubbard::obsNormalized(unsigned obsIndex) const {
	if (obsIndex < obsCount) {
		return obsValRefs[obsIndex];
	} else {
		throw WrongObsIndex(obsIndex);
	}
}

std::string DetHubbard::getObservableName(unsigned obsIndex) const {
	if (obsIndex < obsCount) {
		return obsNames[obsIndex];
	} else {
		throw WrongObsIndex(obsIndex);
	}
}

std::string DetHubbard::getObservableShort(unsigned obsIndex) const {
	if (obsIndex < obsCount) {
		return obsShorts[obsIndex];
	} else {
		throw WrongObsIndex(obsIndex);
	}
}

unsigned DetHubbard::getNumberOfVectorObservables() const {
	return vecObsCount;
}

VecNum DetHubbard::vecObsNormalized(unsigned vecObsIndex) const {
	if (vecObsIndex < vecObsCount) {
		return vecObsValRefs[vecObsIndex];
	} else {
		throw WrongObsIndex(vecObsIndex, true);
	}
}

std::string DetHubbard::getVectorObservableName(unsigned vecObsIndex) const {
	if (vecObsIndex < vecObsCount) {
		return vecObsNames[vecObsIndex];
	} else {
		throw WrongObsIndex(vecObsIndex, true);
	}
}

std::string DetHubbard::getVectorObservableShort(unsigned vecObsIndex) const {
	if (vecObsIndex < vecObsCount) {
		return vecObsShorts[vecObsIndex];
	} else {
		throw WrongObsIndex(vecObsIndex, true);
	}
}



inline unsigned DetHubbard::coordsToSite(const std::vector<unsigned>& coords) const {
    const unsigned dimensions = coords.size();
    int site = 0;
    for (unsigned dim = 0; dim < dimensions; ++dim) {
        site += coords[dim] * uint_pow(L, dim);
    }
    return site;
}

void DetHubbard::createNeighborTable() {
	using std::floor;
	nearestNeigbors.resize(2*d, N);
    std::vector<unsigned> curCoords(d);     //holds the x, y, z coordinate components of the current site
    std::vector<unsigned> newCoords(d);     //newly calculated coords of the neighbor
    for (unsigned site = 0; site < N; ++site) {
        int reducedSite = site;
        for (int dim = d - 1; dim >= 0; --dim) {
            curCoords[dim] = floor(reducedSite / uint_pow(L, dim));
            reducedSite -= curCoords[dim] * uint_pow(L, dim);
        }
        assert(reducedSite == 0);
        for (unsigned dim = 0; dim < d; ++dim) {
        	//neighbor in + direction, periodic
        	newCoords = curCoords;
        	newCoords[dim] = (newCoords[dim] + 1) % L;
        	nearestNeigbors(dim * 2, site) = coordsToSite(newCoords);
        	//neighbor in - direction, periodic
        	newCoords = curCoords;
        	newCoords[dim] = (newCoords[dim] - 1 + L) % L;
        	nearestNeigbors(dim * 2 + 1, site) = coordsToSite(newCoords);
        }
    }
}


void DetHubbard::setupRandomAuxfield() {
	for (unsigned timeslice = 1; timeslice <= m; ++timeslice) {
		for (unsigned site = 0; site < N; ++site) {
			if (rng.rand01() <= 0.5) {
				auxfield(site, timeslice - 1) = +1;
			} else {
				auxfield(site, timeslice - 1) = -1;
			}
		}
	}
}

void DetHubbard::setupPropTmat() {
	MatNum tmat = -mu * arma::eye(N, N);

	for (unsigned site = 0; site < N; ++site) {
		//hopping between nearest neighbors
		for (auto p = nearestNeigbors.begin_col(site);
				 p != nearestNeigbors.end_col(site); ++p) {
			tmat(*p, site) -= t;
		}
	}

	proptmat = computePropagator(dtau, tmat);
}


void DetHubbard::setupUdVStorage() {
	eye_UdV.U = arma::eye(N,N);
	eye_UdV.d = arma::ones(N);
	eye_UdV.V = arma::eye(N,N);

	auto setup = [this](std::vector<UdV>& storage, Spin spinz) {
		storage = std::vector<UdV>(m + 1);

		storage[0] = eye_UdV;
		storage[1] = svd(computeBmatNaive(1, 0, spinz));

		for (unsigned k = 1; k <= m - 1; ++k) {
			const MatNum& U_k = storage[k].U;
			const VecNum& d_k = storage[k].d;
			const MatNum& V_k = storage[k].V;
			MatNum B_kp1 = computeBmatNaive(k + 1, k, spinz);
			UdV UdV_temp = svd((B_kp1 * U_k) * arma::diagmat(d_k));
			storage[k+1].U = UdV_temp.U;
			storage[k+1].d = UdV_temp.d;
			storage[k+1].V = UdV_temp.V * V_k;
		}
	};

	setup(UdVStorageUp, Spin::Up);
	setup(UdVStorageDn, Spin::Down);

	lastSweepDir = SweepDirection::Up;
}


MatNum DetHubbard::computePropagator(num scalar, const MatNum& matrix) const {
	using namespace arma;

	VecNum eigval;
	MatNum eigvec;
	eig_sym(eigval, eigvec, matrix);

	return eigvec * diagmat(exp(-scalar * eigval)) * trans(eigvec);
}


inline MatNum DetHubbard::computeBmatNaive(unsigned n2, unsigned n1, Spin spinz,
		const MatInt& arbitraryAuxfield) const {
	using namespace arma;

	if (n2 == n1) {
		return eye(N, N);
	}

	assert(n2 > n1);
	assert(n2 <= m);
	//assert(n1 >= 0);

	num sign = num(int(spinz));

	//Propagator using the HS-field potential for the given timeslice
	auto singleTimeslicePropagator = [this, sign, arbitraryAuxfield](unsigned timeslice) -> MatNum {
		//the cast with conv_to is necessary here, else everything would result in integers!
		//-- an Armadillo bug IMHO
		return diagmat(exp(sign * alpha *
				conv_to<VecNum>::from(arbitraryAuxfield.col(timeslice-1)))) * proptmat;
	};

	MatNum B = singleTimeslicePropagator(n2);

	for (unsigned n = n2 - 1; n >= n1 + 1; --n) {
		B *= singleTimeslicePropagator(n);
	}

	return B;
}

inline MatNum DetHubbard::computeBmatNaive(unsigned n2, unsigned n1, Spin spinz) const {
	return computeBmatNaive(n2, n1, spinz, auxfield);
}

inline MatNum DetHubbard::computeGreenFunctionNaive(
		const MatNum& bTau0, const MatNum& bBetaTau) const {
	return arma::inv(arma::eye(N,N) + bTau0 * bBetaTau);
}

inline MatNum DetHubbard::computeGreenFunctionNaive(unsigned timeslice,
		Spin spinz) const {
	//TODO: should use stored B-matrices, for the timeslices that have not changed
	return computeGreenFunctionNaive(computeBmatNaive(timeslice, 0, spinz),
			                    computeBmatNaive(m, timeslice, spinz));
}


num DetHubbard::weightRatioGenericNaive(const MatInt& auxfieldBefore,
		const MatInt& auxfieldAfter) const {
	using namespace arma;

	num weightAfterUp  = det(eye(N,N) + computeBmatNaive(m, 0, Spin::Up, auxfieldAfter));
	num weightBeforeUp = det(eye(N,N) + computeBmatNaive(m, 0, Spin::Up, auxfieldBefore));
	num ratioUp = weightAfterUp / weightBeforeUp;

	num weightAfterDown  = det(eye(N,N) + computeBmatNaive(m, 0, Spin::Down, auxfieldAfter));
	num weightBeforeDown = det(eye(N,N) + computeBmatNaive(m, 0, Spin::Down, auxfieldBefore));
	num ratioDown = weightAfterDown / weightBeforeDown;

//	std::cout << weightafterup << " " << weightbeforeup << " " << weightafterdown << " " << weightbeforedown << "\n";

	return ratioUp * ratioDown;
}

inline num DetHubbard::weightRatioSingleFlip(unsigned site, unsigned timeslice) const {
	using std::exp;
	//TODO: possibly precompute the exponential factors (auxfield is either +/- 1), would require an if though.

	//exponential factors
	//results again do not seem to for the location of the -sign
	num expUp   = exp(-2.0 * alpha * num(auxfield(site, timeslice-1)));
	num expDown = exp( 2.0 * alpha * num(auxfield(site, timeslice-1)));

	num ratioUp   = 1.0 + (expUp   - 1.0) * (1.0 - gUp(site,site, timeslice-1));
	num ratioDown = 1.0 + (expDown - 1.0) * (1.0 - gDn(site,site, timeslice-1));

//	std::cout << ratioUp << " " << ratioDown << std::endl;

	return ratioUp * ratioDown;
}

inline void DetHubbard::updateGreenFunctionWithFlip(unsigned site, unsigned timeslice) {
	auto update = [this, site](MatNum& green, num deltaSite) {
		const MatNum& greenOld = green;		//reference
		MatNum greenNew = green;			//copy
		const MatNum oneMinusGreenOld = arma::eye(N,N) - greenOld;
		num divisor = 1.0 + deltaSite * oneMinusGreenOld(site, site);
		num greenFactor = deltaSite / divisor;
		for (unsigned y = 0; y < N; ++y) {
			for (unsigned x = 0; x < N; ++x) {
				greenNew(x, y) -=  greenOld(x, site) * greenFactor *
						oneMinusGreenOld(site, y);

				//experimental index swap below -- this seemed to be the choice in Santos 2003
//				greenNew(x, y) -=  greenOld(site, y) * greenFactor *
//						oneMinusGreenOld(x, site);
			}
		}
		green = greenNew;
	};

	using std::exp;
	update(gUp.slice(timeslice-1), exp(-2.0 * alpha * num(auxfield(site, timeslice-1))) - 1.0);
	update(gDn.slice(timeslice-1), exp(+2.0 * alpha * num(auxfield(site, timeslice-1))) - 1.0);
}



