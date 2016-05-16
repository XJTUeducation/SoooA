/*
 * CostAbstract.h
 *
 *  Created on: 2016. 4. 25.
 *      Author: jhkim
 */

#ifndef COST_COST_H_
#define COST_COST_H_

#include <armadillo>

using namespace arma;


class Cost {
public:
	Cost() {}
	virtual ~Cost() {}

	virtual double fn(const vec *pA, const vec *pY) = 0;
	virtual void d_cost(const cube &z, const cube &activation, const vec &target, cube &delta) = 0;
};

#endif /* COST_COST_H_ */
