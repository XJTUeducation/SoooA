/**
 * @file NoiseInputLayer.cpp
 * @date 2017-02-16
 * @author moonhoen lee
 * @brief 
 * @details
 */

#include <time.h>

#include <boost/random.hpp>
#include <boost/math/special_functions/next.hpp>

#include <cblas.h>

#include "common.h"
#include "NoiseInputLayer.h"
#include "InputLayer.h"
#include "NetworkConfig.h"

using namespace std;

typedef boost::mt19937 RNGType;

template<typename Dtype>
NoiseInputLayer<Dtype>::NoiseInputLayer() {
    initialize(0, 0.0, 0.0, false, 0, 0, 0, 0.0, 0.0);
}

template<typename Dtype>
NoiseInputLayer<Dtype>::NoiseInputLayer(const string name, int noiseDepth, double noiseMean,
    double noiseVariance, bool useLinearTrans, int tranChannels, int tranRows, int tranCols,
    double tranMean, double tranVariance) :
    InputLayer<Dtype>(name) {
    initialize(noiseDepth, noiseMean, noiseVariance, useLinearTrans, tranChannels, tranRows,
        tranCols, tranMean, tranVariance);
}

template<typename Dtype>
NoiseInputLayer<Dtype>::NoiseInputLayer(Builder* builder) : InputLayer<Dtype>(builder) {
	initialize(builder->_noiseDepth, builder->_noiseMean, builder->_noiseVariance,
        builder->_useLinearTrans, builder->_tranChannels, builder->_tranRows,
        builder->_tranCols, builder->_tranMean, builder->_tranVariance);
}

template<typename Dtype>
NoiseInputLayer<Dtype>::~NoiseInputLayer() {
    if (this->uniformArray != NULL) {
        free(this->uniformArray);
    }
}

template <typename Dtype>
void NoiseInputLayer<Dtype>::prepareUniformArray() {
	RNGType rng;
    rng.seed(static_cast<unsigned int>(time(NULL)+getpid()));

    SASSERT0(this->uniformArray == NULL);
    int allocSize = sizeof(Dtype) * this->noiseDepth;
    this->uniformArray = (Dtype*)malloc(allocSize);
    SASSERT0(this->uniformArray != NULL);

    boost::normal_distribution<float> random_distribution(0, 1);
    boost::variate_generator<RNGType, boost::normal_distribution<float> >
    variate_generator(rng, random_distribution);

    for (int i = 0; i < this->noiseDepth; ++i) {
        this->uniformArray[i] = (Dtype)variate_generator();
    }
}

template <typename Dtype>
void NoiseInputLayer<Dtype>::prepareLinearTranMatrix() {
	RNGType rng;
    rng.seed(static_cast<unsigned int>(time(NULL)+getpid()));
    boost::normal_distribution<float> random_distribution(0, 1);
    boost::variate_generator<RNGType, boost::normal_distribution<float> >
    variate_generator(rng, random_distribution);

    Dtype* tempMatrix;
    int tempMatrixCount = this->noiseDepth * this->tranChannels * this->tranRows *
        this->tranCols;

    int tempMatrixAllocSize = sizeof(Dtype) * tempMatrixCount;
    tempMatrix = (Dtype*)malloc(tempMatrixAllocSize);
    SASSERT0(tempMatrix != NULL);

    for (int i = 0; i < tempMatrixCount; i++) {
        tempMatrix[i] = (Dtype)variate_generator();
    }

    int linearTransMatrixAllocSize = sizeof(Dtype) * this->tranChannels * this->tranRows *
        this->tranCols;
    SASSERT0(this->linearTransMatrix == NULL);
    this->linearTransMatrix = (Dtype*)malloc(linearTransMatrixAllocSize);
    SASSERT0(this->linearTransMatrix != NULL);

    int m = 1;
    int n = this->tranChannels * this->tranRows * this->tranCols;
    int k = this->noiseDepth;

    if (sizeof(Dtype) == sizeof(float)) {
        cblas_sgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, m, n, k, 1,
            this->uniformArray, m, tempMatrix, n, 0, this->linearTransMatrix, m);
    } else {
        cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, m, n, k, 1,
            (const double*)this->uniformArray, m, (const double*)tempMatrix, n, 0,
            (double*)this->linearTransMatrix, m);
    }

    free(tempMatrix);
}

template <typename Dtype>
void NoiseInputLayer<Dtype>::reshape() {
    uint32_t batchSize = this->networkConfig->_batchSize;

    if (batchSize <= this->batchSize)
        return;

    if (this->uniformArray == NULL) {
        prepareUniformArray();

        if (this->useLinearTrans)
            prepareLinearTranMatrix();
    }

    this->batchSize = batchSize;
    if (!this->useLinearTrans) {
        this->_inputData[0]->reshape(
            {(unsigned int)batchSize, 1, (unsigned int)this->noiseDepth, 1});
    } else {
        this->_inputData[0]->reshape({(unsigned int)batchSize, 
                                      (unsigned int)this->tranChannels,
                                      (unsigned int)this->tranRows,
                                      (unsigned int)this->tranCols});
    }

    for (int i = 0 ; i < batchSize; i++) {
        int copySize;

        if (this->useLinearTrans) {
            copySize = sizeof(Dtype) * this->tranChannels * this->tranRows * this->tranCols;
            this->_inputData[0]->set_device_with_host_data(this->linearTransMatrix,
                copySize * i, copySize); 
        } else {
            copySize = sizeof(Dtype) * this->noiseDepth;
            this->_inputData[0]->set_device_with_host_data(this->uniformArray, copySize * i,
                copySize); 
        }
    }

}

template<typename Dtype>
void NoiseInputLayer<Dtype>::feedforward() {
	//Layer<Dtype>::feedforward();
	cout << "unsupported ... " << endl;
	exit(1);
}

template<typename Dtype>
void NoiseInputLayer<Dtype>::feedforward(const uint32_t baseIndex, const char* end) {
    reshape();

}

template<typename Dtype>
void NoiseInputLayer<Dtype>::initialize(int noiseDepth, double noiseMean,
    double noiseVariance, bool useLinearTrans, int tranChannels, int tranRows,
    int tranCols, double tranMean, double tranVariance) {

    this->type = Layer<Dtype>::NoiseInput;
    this->batchSize = 0;
    this->uniformArray = NULL;
    this->linearTransMatrix = NULL;

    this->noiseDepth = noiseDepth;
    this->noiseMean = noiseMean;
    this->noiseVariance = noiseVariance;
    this->useLinearTrans = useLinearTrans;
    this->tranChannels = tranChannels;
    this->tranRows = tranRows;
    this->tranCols = tranCols;
    this->tranMean = tranMean;
    this->tranVariance = tranVariance;
}

template class NoiseInputLayer<float>;
