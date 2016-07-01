/*
 * PoolingFactory.h
 *
 *  Created on: 2016. 6. 7.
 *      Author: jhkim
 */

#ifndef POOLING_POOLINGFACTORY_H_
#define POOLING_POOLINGFACTORY_H_

#include "AvgPooling.h"
#include "MaxPooling.h"


#if CPU_MODE


class PoolingFactory {
public:
	PoolingFactory() {}
	virtual ~PoolingFactory() {}

	static Pooling *create(PoolingType poolingType) {
		switch(poolingType) {
		case PoolingType::Max: return new MaxPooling();
		case PoolingType::Avg: return new AvgPooling();
		case PoolingType::None:
		default: return 0;
		}
	}

	static void destroy(Pooling *&pooling_fn) {
		if(pooling_fn) {
			delete pooling_fn;
			pooling_fn = NULL;
		}
	}
};


#else


class PoolingFactory {
public:
	PoolingFactory() {}
	virtual ~PoolingFactory() {}

	static Pooling *create(PoolingType poolingType) {
		switch(poolingType) {
		case PoolingType::Max: return new MaxPooling();
		case PoolingType::Avg: return new AvgPooling();
		case PoolingType::None:
		default: return 0;
		}
	}

	static void destroy(Pooling *&pooling_fn) {
		if(pooling_fn) {
			delete pooling_fn;
			pooling_fn = NULL;
		}
	}
};

#endif

#endif /* POOLING_POOLINGFACTORY_H_ */