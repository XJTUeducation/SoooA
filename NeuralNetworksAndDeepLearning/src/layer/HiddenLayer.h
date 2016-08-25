/**
 * @file	HiddenLayer.h
 * @date	2016/5/11
 * @author	jhkim
 * @brief
 * @details
 */

#ifndef LAYER_HIDDENLAYER_H_
#define LAYER_HIDDENLAYER_H_

#include "Layer.h"


/**
 * @brief 히든 레이어 기본 추상 클래스
 * @details 기본 레이어의 클래스에 backpropagation, parameter update와 같은
 *          파라미터 학습 관련 기능을 추가한다.
 */
class HiddenLayer : public Layer {
public:
	class Builder : public Layer::Builder {
	public:
		vector<uint32_t> _prevLayerIndices;

		Builder() {}
		virtual Builder* name(const string name) {
			Layer::Builder::name(name);
			return this;
		}
		virtual Builder* id(uint32_t id) {
			Layer::Builder::id(id);
			return this;
		}
		virtual Builder* nextLayerIndices(const vector<uint32_t>& nextLayerIndices) {
			Layer::Builder::nextLayerIndices(nextLayerIndices);
			return this;
		}
		virtual Builder* prevLayerIndices(const vector<uint32_t>& prevLayerIndices) {
			this->_prevLayerIndices = prevLayerIndices;
			return this;
		}
		Layer* build() = 0;
	};


	HiddenLayer() {}
	HiddenLayer(Builder* builder) : Layer(builder) {
		for(uint32_t i = 0; i < builder->_prevLayerIndices.size(); i++) {
			this->prevLayers.push_back((Layer*)((size_t)builder->_prevLayerIndices[i]));
		}
	}
	HiddenLayer(const string name) : Layer(name) {}
	virtual ~HiddenLayer() {}


	/**
	 * @details 네트워크 cost의 다음 레이어의 입력에 관한 gradient값을 전달 받아
	 *          현재 레이어의 parameter(parameter가 있는 경우), input에 관한 gradient를 계산하고
	 *          이전 레이어에 현재 레이어의 input에 관한 gradient값을 전달한다.
	 * @param idx 현재 레이어에 연결된 다음 레이어의 순번 index
	 * @param next_delta_input 네트워크 cost의 다음 레이어의 입력에 관한 gradient 장치 메모리 포인터
	 */
	virtual void backpropagation(UINT idx, Data* next_input, uint32_t offset) {
		_deconcat(idx, next_input, offset);
		if (!w_isLastNextLayerRequest(idx, "HiddenLayer::backpropagation()")) return;

		//_scaleGradient();
		_backpropagation();
		propBackpropagation();
	}


protected:
	virtual void _shape(bool recursive=true) {
		if(recursive) {
			Layer::_shape();
		}
	}
	virtual void _clearShape() {
		Layer::_clearShape();
	}
	/**
	 * @details 네트워크 cost의 다음 레이어의 입력에 관한 gradient값을 전달 받아
	 *          현재 레이어의 parameter(parameter가 있는 경우), input에 관한 gradient를 계산한다.
	 */
	virtual void _backpropagation() {
		_input->set_device_grad(_output);
	}
	/**
	 * @details 복수의 '다음' 레이어로부터의 gradient를 조합한다.
	 *          기본 조합은 gradient의 합으로 한다.
	 * @param idx 현재 레이어에 연결된 다음 레이어의 순번 index
	 * @param next_delta_input 네트워크 cost의 다음 레이어의 입력에 관한 gradient 장치 메모리 포인터
	 */
	virtual void _deconcat(UINT idx, Data* next_delta_input, uint32_t offset) {
		//Util::printDeviceData(next_delta_input, out_dim.rows, out_dim.cols, out_dim.channels, out_dim.batches, "next_delta_input:");
		//Util::printDeviceData(d_delta_output, out_dim.rows, out_dim.cols, out_dim.channels, out_dim.batches, "d_delta_output:");
		next_delta_input->print_grad("next_delta_input:");
		_output->print_grad("d_delta_output");
		// 첫번째 branch로부터의 backpropagation, 그대로 copy
		if(isFirstNextLayerRequest(idx)) {
			//checkCudaErrors(cudaMemcpyAsync(d_delta_output, next_delta_input, sizeof(DATATYPE)*out_dim.batchsize(), cudaMemcpyDeviceToDevice));
			_output->set_device_grad(next_delta_input, offset);
		}
		// 첫번째 이후의 branch로부터의 backpropagation, accumulate gradient
		else {
			//checkCudaErrors(cublasSaxpy(Cuda::cublasHandle, static_cast<int>(out_dim.batchsize()), &Cuda::alpha, next_delta_input, 1, d_delta_output, 1));
			_output->add_device_grad(next_delta_input, offset);
		}
		//Util::printDeviceData(d_delta_output, out_dim.rows, out_dim.cols, out_dim.channels, out_dim.batches, "d_delta_output:");
		_output->print_grad("d_delta_output:");
	}



	/**
	 * @details 복수의 '다음' 레이어로부터의 gradient들에 대해 branch의 수 기준으로 스케일링한다.
	 *          _deconcat()이 gradient합산이 아닌 방식으로 구현된 경우 _scaleGradient() 역시 적절히 재정의해야 한다.
	 */
	virtual void _scaleGradient() {
		if(nextLayers.size() > 1) {
			float branchFactor = 1.0f / nextLayers.size();
			//cout << this->name << "'s backpropagation branch factor is " << branchFactor << endl;
			//checkCudaErrors(cublasSscal(Cuda::cublasHandle, static_cast<int>(out_dim.batchsize()), &branchFactor, d_delta_output, 1));
			_output->scale_device_grad(branchFactor);
		}
	}

	/**
	 * @details 이전 레이어들에 대해 backpropagation() 메쏘드를 호출한다.
	 */
	virtual void propBackpropagation() {
		HiddenLayer *hiddenLayer;
		for(UINT i = 0; i < prevLayers.size(); i++) {
			hiddenLayer = dynamic_cast<HiddenLayer *>(prevLayers[i]);

			// !!! 대부분의 경우 _backpropagation에서 사용한 d_delta_input을 그대로 사용하므로 문제가 없지만
			// DepthConcatLayer와 같이 d_delta_input을 분배해야 하는 케이스가 있으므로 d_delta_input을 그대로 사용하지 말고
			// getter를 사용하여 이전 레이어에 d_delta_input을 전달해야 한다.
			if(hiddenLayer) {
				//_distGradToPrev(i, hiddenLayer);
				hiddenLayer->backpropagation(id, getInput(), 0);
			}
		}
	}

};


#endif /* LAYER_HIDDENLAYER_H_ */



















