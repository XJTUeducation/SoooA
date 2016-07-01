/*
 * ActivationFactory.h
 *
 *  Created on: 2016. 6. 7.
 *      Author: jhkim
 */

#ifndef ACTIVATION_ACTIVATIONFACTORY_H_
#define ACTIVATION_ACTIVATIONFACTORY_H_

#include "../layer/LayerConfig.h"
#include "Activation.h"
#include "Sigmoid.h"
#include "Softmax.h"
#include "ReLU.h"



class ActivationFactory {
public:
	ActivationFactory() {}
	virtual ~ActivationFactory() {}

#if CPU_MODE
public:
	static Activation *create(ActivationType activationType) {
		switch(activationType) {
		case ActivationType::Sigmoid: return new Sigmoid();
		case ActivationType::Softmax: return new Softmax();
		case ActivationType::ReLU: return new ReLU();
		case ActivationType::None:
		default: return 0;
		}
	}

	static void destory(Activation *&activation_fn) {
		if(activation_fn) {
			delete activation_fn;
			activation_fn = NULL;
		}
	}
#else
public:
	static Activation *create(ActivationType activationType) {
		switch(activationType) {
		case ActivationType::Sigmoid: return new Sigmoid();
		case ActivationType::Softmax: return new Softmax();
		case ActivationType::ReLU: return new ReLU();
		case ActivationType::None:
		default: return 0;
		}
	}

	static void destory(Activation *&activation_fn) {
		if(activation_fn) {
			delete activation_fn;
			activation_fn = NULL;
		}
	}
#endif
};

#endif /* ACTIVATION_ACTIVATIONFACTORY_H_ */