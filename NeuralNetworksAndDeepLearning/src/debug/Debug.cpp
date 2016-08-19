
#include "Debug.h"

#include <vector>

#include "../activation/Activation.h"
#include "../dataset/ImagePackDataSet.h"
#include "../layer/ConvLayer.h"
#include "../layer/DepthConcatLayer.h"
#include "../layer/FullyConnectedLayer.h"
#include "../layer/HiddenLayer.h"
#include "../layer/InceptionLayer.h"
#include "../layer/InputLayer.h"
#include "../layer/LayerConfig.h"
#include "../layer/LRNLayer.h"
#include "../layer/PoolingLayer.h"
#include "../layer/SoftmaxLayer.h"
#include "../network/NetworkConfig.h"
#include "../pooling/Pooling.h"




DataSet* createMnistDataSet() {
	DataSet* dataSet = new ImagePackDataSet(
			"/home/jhkim/data/learning/mnist/train_data",
			"/home/jhkim/data/learning/mnist/train_label",
			1,
			"/home/jhkim/data/learning/mnist/test_data",
			"/home/jhkim/data/learning/mnist/test_label",
			1);
	dataSet->setMean({0.13066047740});
	return dataSet;

}

DataSet* createImageNet10CatDataSet() {
	DataSet* dataSet = new ImagePackDataSet(
			"/home/jhkim/image/ILSVRC2012/save/10cat_100train_100test_100_100/train_data",
			"/home/jhkim/image/ILSVRC2012/save/10cat_100train_100test_100_100/train_label",
			1,
			"/home/jhkim/image/ILSVRC2012/save/10cat_100train_100test_100_100/train_data",
			"/home/jhkim/image/ILSVRC2012/save/10cat_100train_100test_100_100/train_label",
			//"/home/jhkim/image/ILSVRC2012/save/100cat_10000train_1000test_10000_1000/test_data",
			//"/home/jhkim/image/ILSVRC2012/save/100cat_10000train_1000test_10000_1000/test_label",
			1);
	dataSet->setMean({0.47684615850, 0.45469805598, 0.41394191980});
	return dataSet;
}

DataSet* createImageNet100CatDataSet() {
	DataSet* dataSet = new ImagePackDataSet(
			"/home/jhkim/image/ILSVRC2012/save/100cat_10000train_1000test_10000_1000/train_data",
			"/home/jhkim/image/ILSVRC2012/save/100cat_10000train_1000test_10000_1000/train_label",
			1,
			"/home/jhkim/image/ILSVRC2012/save/100cat_10000train_1000test_10000_1000/train_data",
			"/home/jhkim/image/ILSVRC2012/save/100cat_10000train_1000test_10000_1000/train_label",
			//"/home/jhkim/image/ILSVRC2012/save/100cat_10000train_1000test_10000_1000/test_data",
			//"/home/jhkim/image/ILSVRC2012/save/100cat_10000train_1000test_10000_1000/test_label",
			1);
	dataSet->setMean({0.47684615850, 0.45469805598, 0.41394191980});
	return dataSet;
}




LayersConfig* createCNNDoubleLayersConfig() {

	LayersConfig* layersConfig =
			(new LayersConfig::Builder())
			->layer((new InputLayer::Builder())
					->id(0)
					->name("inputLayer")
					->nextLayerIndices({1}))
			->layer((new ConvLayer::Builder())
					->id(1)
					->name("convLayer1")
					->filterDim(5, 5, 3, 10, 2)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({2}))
			->layer((new PoolingLayer::Builder())
					->id(2)
					->name("poolingLayer1")
					->poolDim(3, 3, 2)
					->poolingType(Pooling::Max)
					->prevLayerIndices({1})
					->nextLayerIndices({3}))
			->layer((new ConvLayer::Builder())
					->id(3)
					->name("convLayer2")
					->filterDim(5, 5, 10, 20, 2)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({2})
					->nextLayerIndices({4}))
			->layer((new PoolingLayer::Builder())
					->id(4)
					->name("poolingLayer2")
					->poolDim(3, 3, 2)
					->poolingType(Pooling::Max)
					->prevLayerIndices({3})
					->nextLayerIndices({5}))
			->layer((new FullyConnectedLayer::Builder())
					->id(5)
					->name("fullyConnectedLayer1")
					->nOut(5000)
					->pDropout(0.4)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::Type::ReLU)
					->prevLayerIndices({4})
					->nextLayerIndices({6}))
			->layer((new SoftmaxLayer::Builder())
					->id(6)
					->name("softmaxLayer")
					->nOut(100)
					->pDropout(0.4)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::Type::ReLU)
					->prevLayerIndices({5}))
			->build();

	return layersConfig;
}





LayersConfig* createGoogLeNetLayersConfig() {
	const float bias_const = 0.1;


	cout << endl;
	LayersConfig* layersConfig =
			(new LayersConfig::Builder())
			->layer((new InputLayer::Builder())
					->id(0)
					->name("input")
					->nextLayerIndices({1}))
			->layer((new ConvLayer::Builder())
					->id(1)
					->name("conv1_7x7_s2")
					->filterDim(7, 7, 3, 64, 2)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({2}))
			->layer((new PoolingLayer::Builder())
					->id(2)
					->name("pool1_3x3_s2")
					->poolDim(3, 3, 2)
					->poolingType(Pooling::Max)
					->prevLayerIndices({1})
					->nextLayerIndices({3}))
			->layer((new LRNLayer::Builder())
					->id(3)
					->name("lrn1")
					->lrnDim(5, 0.0001, 0.75, 2.0)
					->prevLayerIndices({2})
					->nextLayerIndices({4}))
			->layer((new ConvLayer::Builder())
					->id(4)
					->name("conv2_3x3_reduce")
					->filterDim(1, 1, 64, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({3})
					->nextLayerIndices({5}))
			->layer((new ConvLayer::Builder())
					->id(5)
					->name("conv2_3x3")
					->filterDim(3, 3, 64, 192, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({4})
					->nextLayerIndices({6}))
			->layer((new LRNLayer::Builder())
					->id(6)
					->name("lrn2")
					->lrnDim(5, 0.0001, 0.75, 2.0)
					->prevLayerIndices({5})
					->nextLayerIndices({7}))
			->layer((new PoolingLayer::Builder())
					->id(7)
					->name("pool2_3x3_s2")
					->poolDim(3, 3, 2)
					->poolingType(Pooling::Max)
					->prevLayerIndices({6})
					->nextLayerIndices({8, 9, 11, 13}))

			//INCEPTION 3A
			->layer((new ConvLayer::Builder())
					->id(8)
					->name("inception_3a/conv1x1")
					->filterDim(1, 1, 192, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.03)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({7})
					->nextLayerIndices({15}))
			->layer((new ConvLayer::Builder())
					->id(9)
					->name("inception_3a/conv3x3reduce")
					->filterDim(1, 1, 192, 96, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.09)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({7})
					->nextLayerIndices({10}))
			->layer((new ConvLayer::Builder())
					->id(10)
					->name("inception_3a/conv3x3")
					->filterDim(3, 3, 96, 128, 1)
					//->filterDim(3, 3, 96, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.03)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({9})
					->nextLayerIndices({15}))
			->layer((new ConvLayer::Builder())
					->id(11)
					->name("inception_3a/conv5x5reduce")
					->filterDim(3, 3, 192, 16, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({7})
					->nextLayerIndices({12}))
			->layer((new ConvLayer::Builder())
					->id(12)
					->name("inception_3a/conv5x5")
					->filterDim(3, 3, 16, 32, 1)
					//->filterDim(3, 3, 16, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({11})
					->nextLayerIndices({15}))
			->layer((new PoolingLayer::Builder())
					->id(13)
					->name("inception_3a/pool3x3")
					->poolDim(3, 3, 1)
					->poolingType(Pooling::Max)
					->prevLayerIndices({7})
					->nextLayerIndices({14}))
			->layer((new ConvLayer::Builder())
					->id(14)
					->name("inception_3a/convProjection")
					->filterDim(3, 3, 192, 32, 1)
					//->filterDim(3, 3, 192, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({13})
					->nextLayerIndices({15}))
			->layer((new DepthConcatLayer::Builder())
					->id(15)
					->name("inception_3a/depthConcat")
					->prevLayerIndices({8, 10, 12, 14})
					->nextLayerIndices({16, 17, 19, 21}))

			//INCEPTION 3B
			->layer((new ConvLayer::Builder())
					->id(16)
					->name("inception_3b/conv1x1")
					->filterDim(1, 1, 256, 128, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.03)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({15})
					->nextLayerIndices({23}))
			->layer((new ConvLayer::Builder())
					->id(17)
					->name("inception_3b/conv3x3reduce")
					->filterDim(1, 1, 256, 128, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.09)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({15})
					->nextLayerIndices({18}))
			->layer((new ConvLayer::Builder())
					->id(18)
					->name("inception_3b/conv3x3")
					->filterDim(3, 3, 128, 192, 1)
					//->filterDim(3, 3, 96, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.03)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({17})
					->nextLayerIndices({23}))
			->layer((new ConvLayer::Builder())
					->id(19)
					->name("inception_3b/conv5x5reduce")
					->filterDim(3, 3, 256, 32, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({15})
					->nextLayerIndices({20}))
			->layer((new ConvLayer::Builder())
					->id(20)
					->name("inception_3b/conv5x5")
					->filterDim(3, 3, 32, 96, 1)
					//->filterDim(3, 3, 16, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({19})
					->nextLayerIndices({23}))
			->layer((new PoolingLayer::Builder())
					->id(21)
					->name("inception_3b/pool3x3")
					->poolDim(3, 3, 1)
					->poolingType(Pooling::Max)
					->prevLayerIndices({15})
					->nextLayerIndices({22}))
			->layer((new ConvLayer::Builder())
					->id(22)
					->name("inception_3b/convProjection")
					->filterDim(3, 3, 256, 64, 1)
					//->filterDim(3, 3, 192, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({21})
					->nextLayerIndices({23}))
			->layer((new DepthConcatLayer::Builder())
					->id(23)
					->name("inception_3b/depthConcat")
					->prevLayerIndices({16, 18, 20, 22})
					->nextLayerIndices({24}))

			->layer((new PoolingLayer::Builder())
					->id(24)
					->name("pool3_3x3_s2")
					->poolDim(3, 3, 2)
					->poolingType(Pooling::Max)
					->prevLayerIndices({23})
					->nextLayerIndices({25, 26, 28, 30}))

			//INCEPTION 4A
			->layer((new ConvLayer::Builder())
					->id(25)
					->name("inception_4a/conv1x1")
					->filterDim(1, 1, 480, 192, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.03)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({24})
					->nextLayerIndices({32}))
			->layer((new ConvLayer::Builder())
					->id(26)
					->name("inception_4a/conv3x3reduce")
					->filterDim(1, 1, 480, 96, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.09)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({24})
					->nextLayerIndices({27}))
			->layer((new ConvLayer::Builder())
					->id(27)
					->name("inception_4a/conv3x3")
					->filterDim(3, 3, 96, 208, 1)
					//->filterDim(3, 3, 96, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.03)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({26})
					->nextLayerIndices({32}))
			->layer((new ConvLayer::Builder())
					->id(28)
					->name("inception_4a/conv5x5reduce")
					->filterDim(3, 3, 480, 16, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({24})
					->nextLayerIndices({29}))
			->layer((new ConvLayer::Builder())
					->id(29)
					->name("inception_4a/conv5x5")
					->filterDim(3, 3, 16, 48, 1)
					//->filterDim(3, 3, 16, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({28})
					->nextLayerIndices({32}))
			->layer((new PoolingLayer::Builder())
					->id(30)
					->name("inception_4a/pool3x3")
					->poolDim(3, 3, 1)
					->poolingType(Pooling::Max)
					->prevLayerIndices({24})
					->nextLayerIndices({31}))
			->layer((new ConvLayer::Builder())
					->id(31)
					->name("inception_4a/convProjection")
					->filterDim(3, 3, 480, 64, 1)
					//->filterDim(3, 3, 192, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::ReLU)
					->prevLayerIndices({30})
					->nextLayerIndices({32}))
			->layer((new DepthConcatLayer::Builder())
					->id(32)
					->name("inception_4a/depthConcat")
					->prevLayerIndices({25, 27, 29, 31})
					->nextLayerIndices({33}))

			->layer((new PoolingLayer::Builder())
					->id(33)
					->name("pool5_7x7_s1")
					->poolDim(7, 7, 4)
					->poolingType(Pooling::Avg)
					->prevLayerIndices({32})
					//->prevLayerIndices({8, 10, 12, 14})
					->nextLayerIndices({34}))
			->layer((new SoftmaxLayer::Builder())
					->id(34)
					->name("softmaxLayer")
					->nOut(100)
					->pDropout(0.4)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, bias_const)
					->activationType(Activation::Type::ReLU)
					->prevLayerIndices({33}))
			->build();

	return layersConfig;



}




LayersConfig* createInceptionLayersConfig() {
	uint32_t layerId = 0;
	const uint32_t ic = 1;
	const uint32_t oc_cv1x1 = 1;
	const uint32_t oc_cv3x3reduce = 1;
	const uint32_t oc_cv3x3 = 2;
	const uint32_t oc_cv5x5reduce = 1;
	const uint32_t oc_cv5x5 = 3;
	const uint32_t oc_cp = 2;


	LayersConfig* layersConfig =
			(new LayersConfig::Builder())
			->layer((new InputLayer::Builder())
					->id(0)
					->name("input")
					->nextLayerIndices({1, 2, 4, 6}))

			->layer((new ConvLayer::Builder())
					->id(1)
					->name("inception_3a/conv1x1")
					->filterDim(1, 1, ic, oc_cv1x1, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({8}))
			->layer((new ConvLayer::Builder())
					->id(2)
					->name("inception_3a/conv3x3reduce")
					->filterDim(1, 1, ic, oc_cv3x3reduce, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({3}))
			->layer((new ConvLayer::Builder())
					->id(3)
					->name("inception_3a/conv3x3")
					->filterDim(3, 3, oc_cv3x3reduce, oc_cv3x3, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({2})
					->nextLayerIndices({8}))
			->layer((new ConvLayer::Builder())
					->id(4)
					->name("inception_3a/conv5x5reduce")
					->filterDim(3, 3, ic, oc_cv5x5reduce, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({5}))
			->layer((new ConvLayer::Builder())
					->id(5)
					->name("inception_3a/conv5x5")
					->filterDim(3, 3, oc_cv5x5reduce, oc_cv5x5, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({4})
					->nextLayerIndices({8}))
			->layer((new PoolingLayer::Builder())
					->id(6)
					->name("inception_3a/pool3x3")
					->poolDim(3, 3, 1)
					->poolingType(Pooling::Max)
					->prevLayerIndices({0})
					->nextLayerIndices({7}))
			->layer((new ConvLayer::Builder())
					->id(7)
					->name("inception_3a/convProjection")
					->filterDim(3, 3, ic, oc_cp, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({6})
					->nextLayerIndices({8}))
			->layer((new DepthConcatLayer::Builder())
					->id(8)
					->name("inception_3a/depthConcat")
					->prevLayerIndices({1, 3, 5, 7})
					->nextLayerIndices({9}))
			->layer((new SoftmaxLayer::Builder())
					->id(9)
					->name("softmaxLayer")
					->nOut(10)
					->pDropout(0.4)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::Type::ReLU)
					->prevLayerIndices({8}))
			->build();



	/*
	LayersConfig* layersConfig =
			(new LayersConfig::Builder())
			->layer((new InputLayer::Builder())
					->id(0)
					->name("input")
					->nextLayerIndices({1, 2, 4, 6}))

			->layer((new ConvLayer::Builder())
					->id(1)
					->name("inception_3a/conv1x1")
					->filterDim(1, 1, 192, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({8}))

			->layer((new ConvLayer::Builder())
					->id(1)
					->name("inception_3a/conv1x1")
					->filterDim(1, 1, 192, 64, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({8}))
			->layer((new ConvLayer::Builder())
					->id(2)
					->name("inception_3a/conv3x3reduce")
					->filterDim(1, 1, 192, 96, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({3}))
			->layer((new ConvLayer::Builder())
					->id(3)
					->name("inception_3a/conv3x3")
					->filterDim(3, 3, 96, 128, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({2})
					->nextLayerIndices({8}))
			->layer((new ConvLayer::Builder())
					->id(4)
					->name("inception_3a/conv5x5reduce")
					->filterDim(3, 3, 192, 16, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({0})
					->nextLayerIndices({5}))
			->layer((new ConvLayer::Builder())
					->id(5)
					->name("inception_3a/conv5x5")
					->filterDim(3, 3, 16, 32, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({4})
					->nextLayerIndices({8}))
			->layer((new PoolingLayer::Builder())
					->id(6)
					->name("inception_3a/pool3x3")
					->poolDim(3, 3, 1)
					->poolingType(Pooling::Max)
					->prevLayerIndices({0})
					->nextLayerIndices({7}))
			->layer((new ConvLayer::Builder())
					->id(7)
					->name("inception_3a/convProjection")
					->filterDim(3, 3, 96, 32, 1)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::ReLU)
					->prevLayerIndices({6})
					->nextLayerIndices({8}))
			->layer((new DepthConcatLayer::Builder())
					->id(8)
					->name("inception_3a/depthConcat")
					->prevLayerIndices({1, 3, 5, 7})
					->nextLayerIndices({9}))
			->layer((new SoftmaxLayer::Builder())
					->id(9)
					->name("softmaxLayer")
					->nOut(100)
					->pDropout(0.4)
					->weightUpdateParam(1, 1)
					->biasUpdateParam(2, 0)
					->weightFiller(ParamFillerType::Xavier, 0.1)
					->biasFiller(ParamFillerType::Constant, 0.2)
					->activationType(Activation::Type::ReLU)
					->prevLayerIndices({8}))
			->build();
			*/

	return layersConfig;



}



























