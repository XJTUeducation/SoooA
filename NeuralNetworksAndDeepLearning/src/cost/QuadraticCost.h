/*
 * QuadraticCost.h
 *
 *  Created on: 2016. 4. 25.
 *      Author: jhkim
 */

#ifndef COST_QUADRATICCOST_H_
#define COST_QUADRATICCOST_H_



#include "Cost.h"




class QuadraticCost : public Cost {
public:
	QuadraticCost() {
		this->type = CostType::Quadratic;
	}
	virtual ~QuadraticCost() {}

#if CPU_MODE
public:
	double fn(const rvec *pA, const rvec *pY) {
		return 0.5*sum(square(*pA - *pY));
	}
	void d_cost(const rcube &z, const rcube &activation, const rvec &target, rcube &delta) {
		delta.slice(0) = activation.slice(0) - target;
	}
#else
	double fn(const rvec *pA, const rvec *pY) {
		return 0.5*sum(square(*pA - *pY));
	}
	void d_cost(const DATATYPE *z, DATATYPE *activation, const UINT *target, DATATYPE *delta, UINT numLabels, UINT size) {
		Cuda::refresh();
		checkCudaErrors(cudaMemcpyAsync(delta, activation, sizeof(DATATYPE)*size, cudaMemcpyDeviceToDevice));
		UINT i;
		for(i = 0; i < size/numLabels; i++) {
			delta[i*numLabels+target[i]]-=1;
		}
	}
#endif

};

#endif /* COST_QUADRATICCOST_H_ */
