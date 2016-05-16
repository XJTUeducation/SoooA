/*
 * SoftmaxLayer.h
 *
 *  Created on: 2016. 5. 12.
 *      Author: jhkim
 */

#ifndef LAYER_SOFTMAXLAYER_H_
#define LAYER_SOFTMAXLAYER_H_

#include "OutputLayer.h"
#include "../cost/LogLikelihoodCost.h"
#include "../activation/Softmax.h"
#include <armadillo>

using namespace arma;


class SoftmaxLayer : public OutputLayer {
public:
	SoftmaxLayer(int n_in, int n_out)
		: OutputLayer(n_in, n_out) {
		initialize();
	}
	SoftmaxLayer(io_dim in_dim, io_dim out_dim)
		: OutputLayer(in_dim, out_dim) {
		initialize();
	}
	virtual ~SoftmaxLayer() {
		if(cost_fn) delete cost_fn;
		if(activation_fn) delete activation_fn;
	}

	void cost(const vec &target, const cube &input) {
		cost_fn->d_cost(z, output, target, delta);

		nabla_b += delta.slice(0);
		nabla_w += delta.slice(0)*input.slice(0).t();
	}

private:
	void initialize() {
		this->cost_fn = new LogLikelihoodCost();
		this->activation_fn = new Softmax();
		this->activation_fn->initialize_weight(in_dim.rows, weight);
	}
};

#endif /* LAYER_SOFTMAXLAYER_H_ */
