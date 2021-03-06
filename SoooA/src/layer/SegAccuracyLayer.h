/*
 * SegAccuracyLayer.h
 *
 *  Created on: Aug 7, 2017
 *      Author: jkim
 */

#ifndef SEGACCURACYLAYER_H_
#define SEGACCURACYLAYER_H_

#include <set>

#include "common.h"
#include "BaseLayer.h"
#include "ConfusionMatrix.h"
#include "MeasureLayer.h"

template <typename Dtype>
class SegAccuracyLayer : public MeasureLayer<Dtype> {
public:
	SegAccuracyLayer();
	virtual ~SegAccuracyLayer();

	Dtype getAccuracy();
    Dtype measure();

	virtual void reshape();
	virtual void feedforward();
	virtual void backpropagation();

private:
	ConfusionMatrix confusionMatrix;
	std::set<int> ignoreLabel;

public:
    /****************************************************************************
     * layer callback functions
     ****************************************************************************/
    static void* initLayer();
    static void destroyLayer(void* instancePtr);
    static void setInOutTensor(void* instancePtr, void* tensorPtr, bool isInput, int index);
    static bool allocLayerTensors(void* instancePtr);
    static void forwardTensor(void* instancePtr, int miniBatchIndex);
    static void backwardTensor(void* instancePtr);
    static void learnTensor(void* instancePtr);
};

#endif /* SEGACCURACYLAYER_H_ */
