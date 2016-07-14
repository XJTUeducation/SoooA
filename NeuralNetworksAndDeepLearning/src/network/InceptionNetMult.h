/*
 * InceptionNetSingle.h
 *
 *  Created on: 2016. 6. 2.
 *      Author: jhkim
 */

#ifndef NETWORK_INCEPTIONNETMULT_H_
#define NETWORK_INCEPTIONNETMULT_H_

#include "../layer/InceptionLayer.h"
#include "../layer/InputLayer.h"
#include "../layer/LayerConfig.h"
#include "../layer/SoftmaxLayer.h"
#include "Network.h"




class InceptionNetMult : public Network {
public:
	InceptionNetMult(NetworkListener *networkListener=0, double lr_mult=0.01, double decay_mult=5.0)
		: Network(networkListener) {
		//double lr_mult = 0.01;
		//double decay_mult = 5.0;

		InputLayer *inputLayer = new InputLayer(
				"input"
				//io_dim(28, 28, 1, batchSize)
				);

		HiddenLayer *pool1Layer = new PoolingLayer(
				"pool1",
				//io_dim(28, 28, 1, batchSize),
				//io_dim(14, 14, 1, batchSize),
				pool_dim(3, 3, 2),
				PoolingType::Max
				);

		HiddenLayer *incept1Layer = new InceptionLayer(
				"incept1",
				//io_dim(14, 14, 1, batchSize),
				//io_dim(14, 14, 12, batchSize),
				1,
				3, 2, 3, 2, 3, 3
				);

		HiddenLayer *incept2Layer = new InceptionLayer(
				"incept2",
				//io_dim(14, 14, 12, batchSize),
				//io_dim(14, 14, 24, batchSize),
				12,
				6, 4, 6, 4, 6, 6
				);

		HiddenLayer *incept3Layer = new InceptionLayer(
				"incept2",
				//io_dim(14, 14, 24, batchSize),
				//io_dim(14, 14, 36, batchSize),
				24,
				9, 6, 9, 6, 9, 9
				);

		/*
		HiddenLayer *incept2Layer = new InceptiornLayer(
				"incept2",
				io_dim(28, 28, 24, batchSize),
				io_dim(28, 28, 36, batchSize),
				9, 6, 9, 6, 9, 9
				);
				*/

		OutputLayer *softmaxLayer = new SoftmaxLayer(
				"softmax",
				//io_dim(14*14*36, 1, 1, batchSize),
				//io_dim(10, 1, 1, batchSize),
				10,
				0.5,
				update_param(lr_mult, decay_mult),
				update_param(lr_mult, decay_mult),
				param_filler(ParamFillerType::Xavier),
				param_filler(ParamFillerType::Constant, 0.1)
				);

		Network::addLayerRelation(inputLayer, pool1Layer);
		Network::addLayerRelation(pool1Layer, incept1Layer);
		Network::addLayerRelation(incept1Layer, incept2Layer);
		Network::addLayerRelation(incept2Layer, incept3Layer);
		Network::addLayerRelation(incept3Layer, softmaxLayer);

		this->inputLayer = inputLayer;
		addOutputLayer(softmaxLayer);
	}
	virtual ~InceptionNetMult() {}
};


#endif /* NETWORK_INCEPTIONNETMULT_H_ */