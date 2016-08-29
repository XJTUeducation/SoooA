/*
 * PoolingLayer.cpp
 *
 *  Created on: 2016. 5. 23.
 *      Author: jhkim
 */

#include "PoolingLayer.h"


template <typename Dtype>
PoolingLayer<Dtype>::PoolingLayer() {
	this->type = Layer<Dtype>::Pool;
}

template <typename Dtype>
PoolingLayer<Dtype>::PoolingLayer(Builder* builder)
	: HiddenLayer<Dtype>(builder) {
	initialize(builder->_poolDim, builder->_poolingType);
}

template <typename Dtype>
PoolingLayer<Dtype>::PoolingLayer(const string name, pool_dim pool_d, typename Pooling<Dtype>::Type poolingType)
	: HiddenLayer<Dtype>(name) {
	initialize(pool_d, poolingType);
}

template <typename Dtype>
PoolingLayer<Dtype>::~PoolingLayer() {
	PoolingFactory<Dtype>::destroy(pooling_fn);
}

template <typename Dtype>
void PoolingLayer<Dtype>::initialize(pool_dim pool_d, typename Pooling<Dtype>::Type poolingType) {
	this->type = Layer<Dtype>::Pool;
	this->pool_d = pool_d;
	this->pooling_fn = PoolingFactory<Dtype>::create(poolingType, pool_d);
}

template <typename Dtype>
void PoolingLayer<Dtype>::_save(ofstream &ofs) {
	HiddenLayer<Dtype>::_save(ofs);

	int poolingType = (int)pooling_fn->getType();

	ofs.write((char *)&pool_d, sizeof(pool_dim));
	ofs.write((char *)&poolingType, sizeof(int));
}

template <typename Dtype>
void PoolingLayer<Dtype>::_load(ifstream &ifs, map<Layer<Dtype>*, Layer<Dtype>*>& layerMap) {
	HiddenLayer<Dtype>::_load(ifs, layerMap);

	pool_dim pool_d;
	typename Pooling<Dtype>::Type poolingType;

	ifs.read((char *)&pool_d, sizeof(pool_dim));
	ifs.read((char *)&poolingType, sizeof(typename Pooling<Dtype>::Type));

	initialize(pool_d, poolingType);

	PoolingLayer<Dtype>::_shape(false);
}

template <typename Dtype>
void PoolingLayer<Dtype>::_clearShape() {
	//checkCudaErrors(cudaFree(d_delta));
	//checkCudaErrors(cudaFree(d_delta_input));

	//d_delta = NULL;
	//d_delta_input = 0;

	HiddenLayer<Dtype>::_clearShape();
}






#ifndef GPU_MODE
template <typename Dtype>
void PoolingLayer<Dtype>::initialize(pool_dim pool_d, typename Pooling<Dtype>::Type poolingType) {
	this->type = Layer<Dtype>::Pool;

	this->out_dim.rows = in_dim.rows / pool_d.rows;
	this->out_dim.cols = in_dim.rows / pool_d.cols;
	this->out_dim.channels = in_dim.channels;

	//this->output.set_size(out_dim.rows, out_dim.cols, out_dim.channels);

	this->pool_d = pool_d;

	this->pooling_fn = PoolingFactory::create(poolingType);

	this->pool_map.set_size(in_dim.rows/pool_d.stride, in_dim.cols/pool_d.stride, in_dim.channels);
	this->output.set_size(size(pool_map));
	this->delta_input.set_size(size(input));
	this->delta_input.zeros();
}

template <typename Dtype>
void PoolingLayer<Dtype>::_feedforward(uint32_t idx, const rcube &input, const char *end=0) {
	if(!isLastPrevLayerRequest(idx)) throw Exception();

	Util::convertCube(input, this->input);
	pooling_fn->pool(pool_d, this->input, pool_map, output);

	propFeedforward(this->output, end);
}

template <typename Dtype>
void PoolingLayer<Dtype>::backpropagation(uint32_t idx, HiddenLayer *next_layer) {
	// TODO w_next_delta를 모두 합하여 한 번에 d_pool하는 것이 연산적으로 유리, 수정 필요
	rcube w_next_delta(size(output));

	Util::convertCube(next_layer->getDeltaInput(), w_next_delta);
	Util::printCube(next_layer->getDeltaInput(), "delta input:");
	Util::printCube(w_next_delta, "w_next_delta:");

	rcube temp(size(delta_input));
	pooling_fn->d_pool(pool_d, w_next_delta, pool_map, temp);
	delta_input += temp;
	Util::printCube(delta_input, "delta_input:");


	// dx가 모두 aggregate된 후 이전 레이어로 back propagate한다.
	if(!isLastNextLayerRequest(idx)) return;

	propBackpropagation();
	delta_input.zeros();
}
#endif





template class PoolingLayer<float>;










