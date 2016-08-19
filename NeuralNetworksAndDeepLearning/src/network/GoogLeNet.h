/**
 * @file GoogLeNet.h
 * @date 2016/5/31
 * @author jhkim
 * @brief
 * @details
 */


#ifndef NETWORK_GOOGLENET_H_
#define NETWORK_GOOGLENET_H_

#include "../activation/ReLU.h"
#include "../layer/ConvLayer.h"
#include "../layer/InceptionLayer.h"
#include "../layer/InputLayer.h"
#include "../layer/LayerConfig.h"
#include "../layer/LRNLayer.h"
#include "../layer/DepthConcatLayer.h"
#include "../layer/PoolingLayer.h"
#include "../layer/SoftmaxLayer.h"
#include "../pooling/AvgPooling.h"
#include "../pooling/MaxPooling.h"

#include "Network.h"

#ifndef GPU_MODE

/**
 * @brief GoogLeNet을 구현한 Network 클래스
 */
class GoogLeNet : public Network {
public:
	GoogLeNet(NetworkListener *networkListener=0, double w_lr_mult=0.1, double w_decay_mult=1.0,
			double b_lr_mult=2.0, double b_decay_mult=0.0) : Network(networkListener) {

		update_param weight_update_param(w_lr_mult, w_decay_mult);
		update_param bias_update_param(b_lr_mult, b_decay_mult);

		double bias_const = 0.1;

		// 224x224x3
		InputLayer *inputLayer = new InputLayer(
				"input"
				);

		// 112x112x64
		ConvLayer *conv1_7x7_s2 = new ConvLayer(
				"conv1_7x7_s2",
				filter_dim(7, 7, 3, 64, 2),
				//update_param(weight_lr_mult, weight_decay_mult),
				//update_param(bias_lr_mult, bias_decay_mult),
				weight_update_param,
				bias_update_param,
				param_filler(ParamFillerType::Xavier),
				param_filler(ParamFillerType::Constant, bias_const),
				Activation::ReLU
				);

		// 56x56x64
		PoolingLayer *pool1_3x3_s2 = new PoolingLayer(
				"pool1_3x3_s2",
				pool_dim(3, 3, 2),
				Pooling::Max
				);

		LRNLayer *pool1_norm1 = new LRNLayer(
				"lrn1",
				lrn_dim(5, 0.0001, 0.75)
				);

		// 56x56x64
		ConvLayer *conv2_3x3_reduce = new ConvLayer(
				"conv2_3x3_reduce",
				filter_dim(1, 1, 64, 64, 1),
				//update_param(weight_lr_mult, weight_decay_mult),
				//update_param(bias_lr_mult, bias_decay_mult),
				weight_update_param,
				bias_update_param,
				param_filler(ParamFillerType::Xavier),
				param_filler(ParamFillerType::Constant, bias_const),
				Activation::ReLU
				);

		// 56x56x192
		ConvLayer *conv2_3x3 = new ConvLayer(
				"conv2_3x3",
				filter_dim(3, 3, 64, 192, 1),
				//update_param(weight_lr_mult, weight_decay_mult),
				//update_param(bias_lr_mult, bias_decay_mult),
				weight_update_param,
				bias_update_param,
				param_filler(ParamFillerType::Xavier),
				param_filler(ParamFillerType::Constant, bias_const),
				Activation::ReLU
				);

		LRNLayer *conv2_norm2 = new LRNLayer(
				"lrn2",
				lrn_dim(5, 0.0001, 0.75)
				);

		// 28x28x192
		PoolingLayer *pool2_3x3_s2 = new PoolingLayer(
				"pool2_3x3_s2",
				pool_dim(3, 3, 2),
				Pooling::Max
				);


		// 28x28x256
		InceptionLayer *inception_3a = new InceptionLayer(
				"inception_3a",
				192,
				64, 96, 128, 16, 32, 32,
				weight_update_param,
				bias_update_param
				);

		// 28x28x480
		InceptionLayer *inception_3b = new InceptionLayer(
				"inception_3b",
				256,
				128, 128, 192, 32, 96, 64,
				weight_update_param,
				bias_update_param
				);

		// 14x14x480
		PoolingLayer *pool3_3x3_s2 = new PoolingLayer(
				"pool3_3x3_s2",
				pool_dim(3, 3, 2),
				Pooling::Max
				);

		// 14x14x512
		InceptionLayer *inception_4a = new InceptionLayer(
				"inception_4a",
				480,
				192, 96, 208, 16, 48, 64,
				weight_update_param,
				bias_update_param
				);

		// 14x14x512
		InceptionLayer *inception_4b = new InceptionLayer(
				"inception_4b",
				512,
				160, 112, 224, 24, 64, 64,
				weight_update_param,
				bias_update_param
				);

		// 14x14x512
		InceptionLayer *inception_4c = new InceptionLayer(
				"inception_4c",
				512,
				128, 128, 256, 24, 64, 64,
				weight_update_param,
				bias_update_param
				);

		// 14x14x528
		InceptionLayer *inception_4d = new InceptionLayer(
				"inception_4d",
				512,
				112, 144, 288, 32, 64, 64,
				weight_update_param,
				bias_update_param
				);

		// 14x14x832
		InceptionLayer *inception_4e = new InceptionLayer(
				"inception_4e",
				528,
				256, 160, 320, 32, 128, 128,
				weight_update_param,
				bias_update_param
				);

		// 7x7x832
		PoolingLayer *pool4_3x3_s2 = new PoolingLayer(
				"pool4_3x3_s2",
				pool_dim(3, 3, 2),
				Pooling::Max
				);

		// 7x7x832
		InceptionLayer *inception_5a = new InceptionLayer(
				"inception_5a",
				832,
				256, 160, 320, 32, 128, 128,
				weight_update_param,
				bias_update_param
				);

		// 7x7x1024
		InceptionLayer *inception_5b = new InceptionLayer(
				"inception_5b",
				//832,
				480,
				384, 192, 384, 48, 128, 128,
				weight_update_param,
				bias_update_param
				);

		// 1x1x1024
		PoolingLayer *pool5_7x7_s1 = new PoolingLayer(
				"pool5_7x7_s1",
				pool_dim(7, 7, 4),
				Pooling::Avg
				);

		// 1000
		SoftmaxLayer *outputLayer = new SoftmaxLayer(
				"output",
				100,
				0.4,
				//update_param(weight_lr_mult, weight_decay_mult),
				//update_param(bias_lr_mult, bias_decay_mult),
				weight_update_param,
				bias_update_param,
				param_filler(ParamFillerType::Xavier),
				//param_filler(ParamFillerType::Constant, 0.0),
				param_filler(ParamFillerType::Constant, bias_const)
				);

		Network::addLayerRelation(inputLayer, conv1_7x7_s2);
		Network::addLayerRelation(conv1_7x7_s2, pool1_3x3_s2);
		Network::addLayerRelation(pool1_3x3_s2, pool1_norm1);
		Network::addLayerRelation(pool1_norm1, conv2_3x3_reduce);
		Network::addLayerRelation(conv2_3x3_reduce, conv2_3x3);
		Network::addLayerRelation(conv2_3x3, conv2_norm2);
		Network::addLayerRelation(conv2_norm2, pool2_3x3_s2);
		Network::addLayerRelation(pool2_3x3_s2, inception_3a);
		Network::addLayerRelation(inception_3a, inception_3b);
		Network::addLayerRelation(inception_3b, pool3_3x3_s2);
		Network::addLayerRelation(pool3_3x3_s2, /*inception_4a);
		Network::addLayerRelation(inception_4a, inception_4b);
		Network::addLayerRelation(inception_4b, inception_4c);
		Network::addLayerRelation(inception_4c, inception_4d);
		Network::addLayerRelation(inception_4d, inception_4e);
		Network::addLayerRelation(inception_4e, */pool4_3x3_s2);
		Network::addLayerRelation(pool4_3x3_s2, /*inception_5a);
		Network::addLayerRelation(inception_5a, */inception_5b);
		Network::addLayerRelation(inception_5b, pool5_7x7_s1);
		Network::addLayerRelation(pool5_7x7_s1, outputLayer);

		this->inputLayer = inputLayer;
		addOutputLayer(outputLayer);
	}
	virtual ~GoogLeNet() {}
};

#endif











#endif /* NETWORK_GOOGLENET_H_ */
