/**
 * @file	LogLikelihoodCost.h
 * @date	2016/5/12
 * @author	jhkim
 * @brief
 * @details
 */


#ifndef COST_LOGLIKELIHOODCOST_H_
#define COST_LOGLIKELIHOODCOST_H_

#include "Cost.h"
#include "../cuda/Cuda.h"


class LogLikelihoodCost : public Cost {
public:
	LogLikelihoodCost();
	virtual ~LogLikelihoodCost();

#if CPU_MODE
	double fn(const rvec *pA, const rvec *pY);
	void d_cost(const rcube &z, const rcube &activation, const rvec &target, rcube &delta);
#else
	double fn(const DATATYPE *pA, const DATATYPE *pY);
	void d_cost(const DATATYPE *z, DATATYPE *activation, const UINT *target, DATATYPE *delta, UINT numLabels, UINT batchsize);
#endif

};


#endif /* COST_LOGLIKELIHOODCOST_H_ */


