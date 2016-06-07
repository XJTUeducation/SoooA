/*
 * PoolingLayer.h
 *
 *  Created on: 2016. 5. 23.
 *      Author: jhkim
 */

#ifndef LAYER_POOLINGLAYER_H_
#define LAYER_POOLINGLAYER_H_

#include "HiddenLayer.h"
#include "../pooling/Pooling.h"
#include "../exception/Exception.h"


class PoolingLayer : public HiddenLayer {
public:
	PoolingLayer(string name, io_dim in_dim, pool_dim pool_d, Pooling *pooling_fn);
	virtual ~PoolingLayer();

	rcube &getDeltaInput() { return this->delta_input; }


	void feedforward(int idx, const rcube &input);
	void backpropagation(int idx, HiddenLayer *next_layer);

	// update할 weight, bias가 없기 때문에 아래의 method에서는 do nothing
	void reset_nabla(int idx) {
		if(!isLastPrevLayerRequest(idx)) throw Exception();
		Layer::reset_nabla(idx);
	}
	void update(int idx, double eta, double lambda, int n, int miniBatchSize) {
		if(!isLastPrevLayerRequest(idx)) throw Exception();
		Layer::update(idx, eta, lambda, n, miniBatchSize);
	}

private:
	ucube pool_map;
	rcube delta;
	rcube delta_input;

	pool_dim pool_d;
	Pooling *pooling_fn;
};

#endif /* LAYER_POOLINGLAYER_H_ */