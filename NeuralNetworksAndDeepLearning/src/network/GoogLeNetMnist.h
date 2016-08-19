/**
 * @file GoogLeNetMnist.h
 * @date 2016/6/1
 * @author jhkim
 * @brief
 * @details
 */


#ifndef NETWORK_GOOGLENETMNIST_H_
#define NETWORK_GOOGLENETMNIST_H_

#include "../activation/ReLU.h"
#include "../layer/ConvLayer.h"
#include "../layer/InceptionLayer.h"
#include "../layer/InputLayer.h"
#include "../layer/LayerConfig.h"
#include "../layer/LRNLayer.h"
#include "../layer/PoolingLayer.h"
#include "../layer/SoftmaxLayer.h"
#include "../pooling/AvgPooling.h"
#include "../pooling/MaxPooling.h"
#include "Network.h"

#ifndef GPU_MODE
/**
 * @brief GoogLeNet을 Mnist 데이터셋에 대해 구현한 Network 클래스
 * @details 네트워크를 특정 입력 구조에 의존하지 않도록 구분해서 일반 GoogLeNet 클래스와 차이가 없어졌다.
 *          사용하지 않음.
 */
class GoogLeNetMnist : public Network {
public:
	GoogLeNetMnist(NetworkListener *networkListener=0, double lr_mult=0.1, double decay_mult=5.0) : Network(networkListener) {
		update_param weight_update_param(lr_mult, decay_mult);
		update_param bias_update_param(lr_mult, decay_mult);

		InputLayer *inputLayer = new InputLayer(
				"input"
				//io_dim(28, 28, 1, batchSize)
				);

		ConvLayer *conv1_7x7_s2 = new ConvLayer(
				"conv1_7x7_s2",
				//io_dim(28, 28, 1, batchSize),
				//io_dim(28, 28, 20, batchSize),
				filter_dim(5, 5, 1, 20, 1),
				//update_param(weight_lr_mult, weight_decay_mult),
				//update_param(bias_lr_mult, bias_decay_mult),
				weight_update_param,
				bias_update_param,
				param_filler(ParamFillerType::Xavier, 0.1),
				param_filler(ParamFillerType::Constant, 0.2),
				Activation::ReLU
				);

		PoolingLayer *pool1_3x3_s2 = new PoolingLayer(
				"pool1_3x3_s2",
				//io_dim(28, 28, 20, batchSize),
				//io_dim(14, 14, 20, batchSize),
				pool_dim(3, 3, 2),
				Pooling::Max
				);

		LRNLayer *pool1_norm1 = new LRNLayer(
				"lrn1",
				//io_dim(14, 14, 20, batchSize),
				lrn_dim(5, 0.0001, 0.75)
				);

		ConvLayer *conv2_3x3_reduce = new ConvLayer(
				"conv2_3x3_reduce",
				//io_dim(14, 14, 20, batchSize),
				//io_dim(14, 14, 10, batchSize),
				filter_dim(1, 1, 20, 10, 1),
				//update_param(weight_lr_mult, weight_decay_mult),
				//update_param(bias_lr_mult, bias_decay_mult),
				weight_update_param,
				bias_update_param,
				param_filler(ParamFillerType::Xavier, 0.1),
				param_filler(ParamFillerType::Constant, 0.2),
				Activation::ReLU
				);

		ConvLayer *conv2_3x3 = new ConvLayer(
				"conv2_3x3",
				//io_dim(14, 14, 10, batchSize),
				//io_dim(14, 14, 16, batchSize),
				filter_dim(3, 3, 10, 16, 1),
				//update_param(weight_lr_mult, weight_decay_mult),
				//update_param(bias_lr_mult, bias_decay_mult),
				weight_update_param,
				bias_update_param,
				param_filler(ParamFillerType::Xavier, 0.03),
				param_filler(ParamFillerType::Constant, 0.2),
				Activation::ReLU
				);

		LRNLayer *conv2_norm2 = new LRNLayer(
				"lrn1",
				//io_dim(14, 14, 16, batchSize),
				lrn_dim(5, 0.0001, 0.75)
				);

		PoolingLayer *pool2_3x3_s2 = new PoolingLayer(
				"pool2_3x3_s2",
				//io_dim(14, 14, 16, batchSize),
				//io_dim(14, 14, 16, batchSize),
				pool_dim(3, 3, 1),
				Pooling::Max
				);

		InceptionLayer *inception_3a = new InceptionLayer(
				"inception_3a",
				//io_dim(14, 14, 16, batchSize),
				//io_dim(14, 14, 16, batchSize),
				16,
				4, 2, 4, 2, 4, 4,
				weight_update_param,
				bias_update_param
				);

		InceptionLayer *inception_3b = new InceptionLayer(
				"inception_3b",
				//io_dim(14, 14, 16, batchSize),
				//io_dim(14, 14, 20, batchSize),
				16,
				5, 3, 5, 3, 5, 5,
				weight_update_param,
				bias_update_param
				);

		PoolingLayer *pool3_3x3_s2 = new PoolingLayer(
				"pool3_3x3_s2",
				//io_dim(14, 14, 20, batchSize),
				//io_dim(7, 7, 20, batchSize),
				pool_dim(3, 3, 2),
				Pooling::Max
				);

		InceptionLayer *inception_4a = new InceptionLayer(
				"inception_4a",
				//io_dim(7, 7, 20, batchSize),
				//io_dim(7, 7, 24, batchSize),
				20,
				6, 3, 6, 3, 6, 6,
				weight_update_param,
				bias_update_param
				);

		InceptionLayer *inception_4b = new InceptionLayer(
				"inception_4b",
				//io_dim(7, 7, 24, batchSize),
				//io_dim(7, 7, 28, batchSize),
				24,
				7, 4, 7, 4, 7, 7,
				weight_update_param,
				bias_update_param
				);

		InceptionLayer *inception_4c = new InceptionLayer(
				"inception_4c",
				//io_dim(7, 7, 28, batchSize),
				//io_dim(7, 7, 32, batchSize),
				28,
				8, 4, 8, 4, 8, 8,
				weight_update_param,
				bias_update_param
				);

		InceptionLayer *inception_4d = new InceptionLayer(
				"inception_4d",
				//io_dim(7, 7, 32, batchSize),
				//io_dim(7, 7, 36, batchSize),
				32,
				9, 5, 9, 5, 9, 9,
				weight_update_param,
				bias_update_param
				);

		InceptionLayer *inception_4e = new InceptionLayer(
				"inception_4e",
				//io_dim(7, 7, 36, batchSize),
				//io_dim(7, 7, 40, batchSize),
				36,
				10, 5, 10, 5, 10, 10,
				weight_update_param,
				bias_update_param
				);

		PoolingLayer *pool4_3x3_s2 = new PoolingLayer(
				"pool4_3x3_s2",
				//io_dim(7, 7, 40, batchSize),
				//io_dim(7, 7, 40, batchSize),
				pool_dim(3, 3, 1),
				Pooling::Max
				);

		InceptionLayer *inception_5a = new InceptionLayer(
				"inception_5a",
				//io_dim(7, 7, 40, batchSize),
				//io_dim(7, 7, 44, batchSize),
				40,
				11, 6, 11, 6, 11, 11,
				weight_update_param,
				bias_update_param
				);

		InceptionLayer *inception_5b = new InceptionLayer(
				"inception_5b",
				//io_dim(7, 7, 44, batchSize),
				//io_dim(7, 7, 48, batchSize),
				44,
				12, 6, 12, 6, 12, 12,
				weight_update_param,
				bias_update_param
				);

		PoolingLayer *pool5_7x7_s1 = new PoolingLayer(
				"pool5_7x7_s1",
				//io_dim(7, 7, 48, batchSize),
				//io_dim(1, 1, 48, batchSize),
				pool_dim(7, 7, 7),
				Pooling::Max
				);

		//FullyConnectedLayer *fc1 = new FullyConnectedLayer("fc1", 48, 48, 0.4, Activation::ReLU);

		SoftmaxLayer *outputLayer = new SoftmaxLayer(
				"output",
				//io_dim(1*1*48, 1, 1, batchSize),
				//io_dim(10, 1, 1, batchSize),
				10,
				0.0,
				//update_param(weight_lr_mult, weight_decay_mult),
				//update_param(bias_lr_mult, bias_decay_mult),
				weight_update_param,
				bias_update_param,
				param_filler(ParamFillerType::Constant, 0.0),
				param_filler(ParamFillerType::Constant, 0.0)
				);

		Network::addLayerRelation(inputLayer, conv1_7x7_s2);
		Network::addLayerRelation(conv1_7x7_s2, pool1_3x3_s2);
		Network::addLayerRelation(pool1_3x3_s2, pool1_norm1);
		Network::addLayerRelation(pool1_norm1, conv2_3x3_reduce);
		Network::addLayerRelation(conv2_3x3_reduce, conv2_3x3);
		Network::addLayerRelation(conv2_3x3, conv2_norm2);
		Network::addLayerRelation(conv2_norm2, pool2_3x3_s2);
		Network::addLayerRelation(pool2_3x3_s2, inception_3a);
		Network::addLayerRelation(inception_3a, /*inception_3b);
		Network::addLayerRelation(inception_3b, pool3_3x3_s2);
		Network::addLayerRelation(pool3_3x3_s2, inception_4a);
		Network::addLayerRelation(inception_4a, inception_4b);
		Network::addLayerRelation(inception_4b, inception_4c);
		Network::addLayerRelation(inception_4c, inception_4d);
		Network::addLayerRelation(inception_4d, inception_4e);
		Network::addLayerRelation(inception_4e, pool4_3x3_s2);
		Network::addLayerRelation(pool4_3x3_s2, inception_5a);
		Network::addLayerRelation(inception_5a, inception_5b);
		Network::addLayerRelation(inception_5b, */pool5_7x7_s1);
		Network::addLayerRelation(pool5_7x7_s1, outputLayer);

		//Network::addLayerRelation(pool1_norm1, outputLayer);

		this->inputLayer = inputLayer;
		addOutputLayer(outputLayer);
	}
	virtual ~GoogLeNetMnist() {}
};

#endif



#endif /* NETWORK_GOOGLENETMNIST_H_ */
