/*
 * PermuteLayer.cpp
 *
 *  Created on: Apr 22, 2017
 *      Author: jkim
 */

#include "PermuteLayer.h"
#include "SysLog.h"

using namespace std;

template <typename Dtype>
__global__ void PermuteKernel(const int nthreads, Dtype* const bottom_data,
		const bool forward, const int* permute_order, const int* old_steps,
		const int* new_steps, const int num_axes, Dtype* const top_data) {
	CUDA_KERNEL_LOOP(index, nthreads) {
		int temp_idx = index;
		int old_idx = 0;
		for (int i = 0; i < num_axes; ++i) {
			int order = permute_order[i];
			old_idx += (temp_idx / new_steps[i]) * old_steps[order];
			temp_idx %= new_steps[i];
		}
		if (forward) {
			top_data[index] = bottom_data[old_idx];
		} else {
			bottom_data[old_idx] = top_data[index];
		}
	}
}



template <typename Dtype>
PermuteLayer<Dtype>::PermuteLayer(Builder* builder)
: Layer<Dtype>(builder),
  orders(builder->_orders) {

	initialize();
}

template <typename Dtype>
PermuteLayer<Dtype>::~PermuteLayer() {

}

template <typename Dtype>
void PermuteLayer<Dtype>::reshape() {
	SASSERT0(this->_inputs.size() == 1 && this->_outputs.size() == 1);

	Layer<Dtype>::_adjustInputShape();
	if (!Layer<Dtype>::_isInputShapeChanged(0))
		return;

	vector<uint32_t> outputShape;
	for (int i = 0; i < this->numAxes; i++) {
		if (i == this->numAxes - 1) {
			this->oldSteps_.mutable_host_data()[i] = 1;
		} else {
			this->oldSteps_.mutable_host_data()[i] =
					this->_inputData[0]->getCountByAxis(i + 1);
		}
		outputShape.push_back(this->_inputData[0]->getShape(
				this->permuteOrder_.host_data()[i]));
	}
	this->_outputData[0]->reshape(outputShape);

	for (int i = 0; i < this->numAxes; i++) {
		if (i == this->numAxes - 1) {
			this->newSteps_.mutable_host_data()[i] = 1;
		} else {
			this->newSteps_.mutable_host_data()[i] =
					this->_outputData[0]->getCountByAxis(i + 1);
		}
	}
}

template <typename Dtype>
void PermuteLayer<Dtype>::feedforward() {
	reshape();

	if (this->needPermute) {
		Dtype* inputData = this->_inputData[0]->mutable_device_data();
		Dtype* outputData = this->_outputData[0]->mutable_device_data();
		int count = this->_outputData[0]->getCount();
		const int* permuteOrder = this->permuteOrder_.device_data();
		const int* newSteps = this->newSteps_.device_data();
		const int* oldSteps = this->oldSteps_.device_data();

		bool forward = true;
		PermuteKernel<Dtype><<<SOOOA_GET_BLOCKS(count), SOOOA_CUDA_NUM_THREADS>>>(
				count, inputData, forward, permuteOrder, oldSteps, newSteps, this->numAxes,
				outputData);
		CUDA_POST_KERNEL_CHECK;
	} else {
		// if there is no need to permute, we share data to save memory
		this->_outputData[0]->share_data(this->_inputData[0]);
	}
}

template <typename Dtype>
void PermuteLayer<Dtype>::backpropagation() {
	if (this->needPermute) {
		Dtype* outputGrad = this->_outputData[0]->mutable_device_grad();
		Dtype* inputGrad = this->_inputData[0]->mutable_device_grad();
		const int count = this->_inputData[0]->getCount();
		const int* permuteOrder = this->permuteOrder_.device_data();
		const int* newSteps = this->newSteps_.device_data();
		const int* oldSteps = this->oldSteps_.device_data();

		bool forward = false;
		PermuteKernel<Dtype><<<SOOOA_GET_BLOCKS(count), SOOOA_CUDA_NUM_THREADS>>>(
				count, inputGrad, forward, permuteOrder, oldSteps, newSteps, this->numAxes,
				outputGrad);
		CUDA_POST_KERNEL_CHECK;
	} else {
		// if there is no need to permute, we share grad to save memory
		//this->_inputData[0]->_grad = this->_outputData[0]->_grad;
		this->_inputData[0]->share_grad(this->_outputData[0]);
	}
}

template <typename Dtype>
void PermuteLayer<Dtype>::initialize() {
	SASSERT0(this->_inputs.size() == 1 && this->_outputs.size() == 1);
	this->numAxes = 4;

	vector<uint32_t> orders;
	// push the specified new orders.
	for (int i = 0; i < this->orders.size(); i++) {
		int order = this->orders[i];
		SASSERT(order < this->numAxes, "order should be less than the input dimension.");
		if (std::find(orders.begin(), orders.end(), order) != orders.end()) {
			SASSERT(false, "there are duplicate orders");
		}
		orders.push_back(order);
	}

	// push the rest orders. And save original step sizes for each axis
	for (int i = 0; i < this->numAxes; i++) {
		if (std::find(orders.begin(), orders.end(), i) == orders.end()) {
			orders.push_back(i);
		}
	}
	SASSERT0(this->numAxes == orders.size());

	// check if we need to reorder the data or keep it
	this->needPermute = false;
	for (int i = 0; i < this->numAxes; i++) {
		if (orders[i] != i) {
			// as long as there is one order which is different from the natural order
			// of the data, we need to permute. Otherwise, we share the data and grad
			this->needPermute = true;
			break;
		}
	}

	this->permuteOrder_.reshape({this->numAxes, 1, 1, 1});
	this->oldSteps_.reshape({this->numAxes, 1, 1, 1});
	this->newSteps_.reshape({this->numAxes, 1, 1, 1});

	for (int i = 0; i < this->numAxes; i++) {
		this->permuteOrder_.mutable_host_data()[i] = orders[i];
	}

	this->orders = orders;
}


template class PermuteLayer<float>;
