/**
 * @file BatchNormLayer_device.cu
 * @date 2017-01-25
 * @author moonhoen lee
 * @brief 
 * @details
 */

#include "cuda_runtime.h"

#include "BatchNormLayer.h"
#include "Exception.h"
#include "NetworkConfig.h"
#include "SysLog.h"
#include "StdOutLog.h"
#include "ColdLog.h"

#define BATCHCONDLAYER_LOG  0

using namespace std;

#ifdef GPU_MODE

///////////////////////////////////////////////////////////////////////////////////////////
// GPU Kernels

template <typename Dtype>
__global__ void FillValues(Dtype *vec, int size, Dtype value)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= size)
		return;
	vec[idx] = value;
}

template <typename Dtype>
__global__ void CalcMean(const Dtype *input, int depth, int batchCount, Dtype *mean)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    for (int i = 0 ; i < batchCount; i++) {
        int index = i * depth + idx;
        mean[idx] += input[index];
    }


    mean[idx] = mean[idx] / (Dtype)batchCount;
}

template <typename Dtype>
__global__ void CalcVariance(const Dtype *input, const Dtype* mean, int depth, int batchCount,
    Dtype *variance)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    for (int i = 0 ; i < batchCount; i++) {
        int index = i * depth + idx;
        variance[idx] += (input[index] - mean[idx]) * (input[index] - mean[idx]);
    }

    variance[idx] = variance[idx] / (Dtype)batchCount;
}

template <typename Dtype>
__global__ void Normalize(const Dtype *input, const Dtype* mean, const Dtype* variance,
    const Dtype* gamma, const Dtype* beta, int depth, int batchCount, Dtype epsilon,
    Dtype* normInput, Dtype* output)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;
    
    Dtype denominator = sqrt(variance[idx] + epsilon);

    for (int i = 0 ; i < batchCount; i++) {
        int index = i * depth + idx;
        normInput[index] = (input[index] - mean[idx]) / denominator;
        output[index] = normInput[index] * gamma[idx] + beta[idx];
    }
}

template <typename Dtype>
__global__ void Add(const Dtype *input, int depth, Dtype* output)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    output[idx] += input[idx];
}

template <typename Dtype>
__global__ void Inference(const Dtype *input, const Dtype *meanSum, const Dtype *varianceSum,
    const Dtype *gamma, const Dtype *beta, int depth, int batchCount, int batchSetCount, 
    Dtype epsilon, Dtype* output)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    Dtype avgMean = meanSum[idx] / (Dtype)batchSetCount;
    Dtype avgVariance = varianceSum[idx] / (Dtype)(batchSetCount - 1);
    Dtype sqrtVariance = sqrt(avgVariance + epsilon);

    for (int i = 0 ; i < batchCount; i++) {
        int index = i * depth + idx;

        output[idx] = input[index] * gamma[idx] / sqrtVariance + beta[idx] - 
            gamma[idx] * avgMean / sqrtVariance;
    }
}

template <typename Dtype>
__global__ void FirstInference(const Dtype *input, const Dtype *meanSum,
    const Dtype *varianceSum, const Dtype *gamma, const Dtype *beta, int depth,
    int batchCount, int batchSetCount, Dtype epsilon, Dtype* output)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth)
		return;

    Dtype avgMean = meanSum[idx];
    Dtype avgVariance = varianceSum[idx];
    Dtype sqrtVariance = sqrt(avgVariance + epsilon);

    for (int i = 0 ; i < batchCount; i++) {
        int index = i * depth + idx;

        output[idx] = input[index] * gamma[idx] / sqrtVariance + beta[idx] - 
            gamma[idx] * avgMean / sqrtVariance;
    }
}

template <typename Dtype>
__global__ void ComputeNormInputGrad(const Dtype *outputGrads, const Dtype *gammas, int depth,
    int batchCount, Dtype* normInputGrads)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    for (int i = 0; i < batchCount; i++) {
        int index = i * depth + idx;
        normInputGrads[index] = outputGrads[index] * gammas[idx];
    }
}

template <typename Dtype>
__global__ void ComputeVarianceGrad(const Dtype* normInputGrad, const Dtype *inputData, 
    const Dtype *mean, const Dtype *variance, Dtype epsilon, int depth, int batchCount,
    Dtype* varianceGrad)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    varianceGrad[idx] = 0;
    for (int i = 0; i < batchCount; i++) {
        int index = i * depth + idx;
        varianceGrad[idx] += normInputGrad[index] * (inputData[index] - mean[idx]) * (-0.5) *
            pow((variance[idx] + epsilon), -1.5);
    }
}

template <typename Dtype>
__global__ void ComputeMeanGrad(const Dtype *normInputGrads, const Dtype *vars,
    const Dtype *varGrads, const Dtype* inputData, const Dtype* means, int depth,
    int batchCount, Dtype epsilon, Dtype* meanGrads)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    meanGrads[idx] = 0;
    for (int i = 0; i < batchCount; i++) {
        int index = i * depth + idx;
        meanGrads[i] += normInputGrads[index] * (-1) / sqrt(vars[i] + epsilon) +
            varGrads[i] * (-2) * (inputData[index] - means[i]) / batchCount;
    }
}

template <typename Dtype>
__global__ void ComputeInputGrad(const Dtype *normInputGrads, const Dtype *vars,
    const Dtype *varGrads, const Dtype* inputData, const Dtype* means, const Dtype* meanGrads,
    int depth, int batchCount, Dtype epsilon, Dtype* inputGrads)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    for (int i = 0; i < batchCount; i++) {
        int index = i * depth + idx;
        inputGrads[index] = normInputGrads[index] / sqrt(vars[idx] + epsilon) +
            varGrads[idx] * 2 * (inputData[index] - means[idx]) / batchCount +
            meanGrads[idx] / batchCount;
    }
}

template <typename Dtype>
__global__ void ComputeScaleGrad(const Dtype *normInputs, const Dtype *outputGrads,
    int depth, int batchCount, Dtype* gammaGrads)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    gammaGrads[idx] = 0;
    for (int i = 0; i < batchCount; i++) {
        int index = i * depth + idx;
        gammaGrads[idx] += outputGrads[index] * normInputs[index];
    }
}

template <typename Dtype>
__global__ void ComputeShiftGrad(const Dtype *outputGrads, int depth, int batchCount,
    Dtype* betaGrads)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= depth) 
		return;

    betaGrads[idx] = 0;
    for (int i = 0; i < batchCount; i++) {
        int index = i * depth + idx;
        betaGrads[idx] += outputGrads[index];
    }
}

template<typename Dtype>
BatchNormLayer<Dtype>::~BatchNormLayer() {
    if (this->depth == 0)
        return;

    SASSERT0(this->gammaSet != NULL);
    free(this->gammaSet);
    SASSERT0(this->betaSet != NULL);
    free(this->betaSet);
    SASSERT0(this->meanSet != NULL);
    free(this->meanSet);
    SASSERT0(this->varSet != NULL);
    free(this->varSet);
    SASSERT0(this->normInputSet != NULL);
    free(this->normInputSet);
}

template <typename Dtype>
void BatchNormLayer<Dtype>::update() {
    // FIXME: momentum, decay 등의 factor들을 고려하지 않았다.
    //        이런 부분을 고려하면 더 좋은 결과가 나올지도 모른다.
    float learningRate = this->networkConfig->getLearningRate();

    Dtype* gammaData = this->gammaSet->mutable_host_data();
    Dtype* gammaGrad = this->gammaSet->mutable_host_grad();
    Dtype* betaData = this->betaSet->mutable_host_data();
    Dtype* betaGrad = this->betaSet->mutable_host_grad();

    for (int i = 0; i < this->depth; i++) {
        gammaData[i] -= learningRate * gammaGrad[i];
        betaData[i]  -= learningRate * betaGrad[i];
    }
}

template <typename Dtype>
void BatchNormLayer<Dtype>::feedforward() {
    // FIXME: 현재 CPU 코드로 구현이 되어 있다. GPU 코드로 변경하자.
    // (1) mini-batch mean 값을 구한다.
	const vector<uint32_t>& inputShape = this->_inputData[0]->getShape();
	int batchCount = inputShape[0];

    const Dtype* inputData = this->_inputData[0]->device_data();
    Dtype* outputData = this->_outputData[0]->mutable_device_data();

	if (this->networkConfig->_status == NetworkStatus::Train) {
        Dtype* means = this->meanSet->mutable_device_data();
        Dtype* vars = this->varSet->mutable_device_data();

        // (1) mini-batch에 사용하는 mean, variance를 초기화 한다.
        FillValues<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            means, this->depth, 0.0f);
        FillValues<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            vars, this->depth, 0.0f);

        // (2) mini-batch mean 값을 구한다.
        CalcMean<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            inputData, this->depth, batchCount, means);

        // (3) mini-batch variance 값을 구한다.
        CalcVariance<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            inputData, means, this->depth, batchCount, vars);

        // (4) normalize 
        Dtype* normInputs = this->normInputSet->mutable_device_data();
        const Dtype* gammas = this->gammaSet->device_data();
        const Dtype* betas = this->betaSet->device_data();
        Normalize<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            inputData, means, vars, gammas, betas, this->depth, batchCount,
            (Dtype)this->epsilon, normInputs, outputData);

        // (5) global meanSets과 varianceSets를 갱신한다.
        this->batchSetCount += 1;

        Dtype* meanSums = this->meanSumSet->mutable_device_mem();
        Dtype* varSums = this->varSumSet->mutable_device_mem();
        Add<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            means, this->depth, meanSums);
        Add<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            vars, this->depth, varSums);
	} else if (this->networkConfig->_status == NetworkStatus::Test) {
        SASSERT((this->batchSetCount > 0), "need train before inference");

        const Dtype* meanSums = this->meanSumSet->device_mem();
        const Dtype* varSums = this->varSumSet->device_mem();
        const Dtype* gammas = this->gammaSet->device_data();
        const Dtype* betas = this->betaSet->device_data();

        if (this->batchSetCount == 1) {
            FirstInference<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
                inputData, meanSums, varSums, gammas, betas, this->depth, batchCount,
                this->batchSetCount, (Dtype)this->epsilon, outputData);
        } else {
            Inference<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
                inputData, meanSums, varSums, gammas, betas, this->depth, batchCount,
                this->batchSetCount, (Dtype)this->epsilon, outputData);
        }
    } else {
        SASSERT(false, "Invalid network status =%d", this->networkConfig->_status);
    }
}

template <typename Dtype>
void BatchNormLayer<Dtype>::reshape() {
	if (!Layer<Dtype>::_adjustInputShape()) {
		const uint32_t count = Util::vecCountByAxis(this->_inputShape[0], 1);
		const uint32_t inputDataCount = this->_inputData[0]->getCountByAxis(1);
		assert(count == inputDataCount);
	}

	if (!Layer<Dtype>::_isInputShapeChanged(0))
		return;

	const vector<uint32_t>& inputShape = this->_inputData[0]->getShape();

    // XXX: 현재 FC에 대해서만 생각하였음
    // TODO: Conv Layer에 대한 구현 필요
	uint32_t batches = inputShape[0];
	uint32_t channels = inputShape[1];
	uint32_t rows = inputShape[2];
	uint32_t cols = inputShape[3];
    uint32_t depth = this->_inputData[0]->getCountByAxis(1);

	this->_inputShape[0] = {batches, channels, rows, cols};
	this->_outputData[0]->reshape({batches, channels, rows, cols});

	STDOUT_COND_LOG(BATCHCONDLAYER_LOG, 
        "<%s> layer' input-0 has reshaped as: %dx%dx%dx%d\n",
        this->name.c_str(), batches, channels, rows, cols);
	STDOUT_COND_LOG(BATCHCONDLAYER_LOG,
	    "<%s> layer' output-0 has reshaped as: %dx%dx%dx%d\n", 
        this->name.c_str(), batches, channels, rows, cols);

    // Batch Normalization 과정에 필요한 구조체들의 메모리를 할당한다.
    if (this->depth == 0) {
        this->depth = depth;

        this->gammaSet->reshape({1, channels, rows, cols});
        this->betaSet->reshape({1, channels, rows, cols});
        this->meanSet->reshape({1, channels, rows, cols});
        this->varSet->reshape({1, channels, rows, cols});

        int allocSize = sizeof(Dtype) * this->depth;
        this->meanSumSet->reshape((size_t)allocSize);
        this->varSumSet->reshape((size_t)allocSize);

        this->normInputSet->reshape({batches, channels, rows, cols});

        // FIXME: 더 좋은 초기화 방법이 있을지도 모른다..
        Dtype* gammas = this->gammaSet->mutable_device_data();
        Dtype* betas = this->betaSet->mutable_device_data();
        Dtype* meanSums = this->meanSumSet->mutable_device_mem();
        Dtype* varSums = this->varSumSet->mutable_device_mem();

        FillValues<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            gammas, this->depth, 1.0f);
        FillValues<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            betas, this->depth, 0.0f);
        FillValues<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            meanSums, this->depth, 0.0f);
        FillValues<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
            varSums, this->depth, 0.0f);
    } else {
        SASSERT0(this->depth == depth);
    }
}

template <typename Dtype>
void BatchNormLayer<Dtype>::computeNormInputGrad() {
    const vector<uint32_t>& inputShape = this->_inputData[0]->getShape();
    int batchCount = inputShape[0];

    const Dtype* outputGrads = this->_outputData[0]->device_grad();
    Dtype* normInputGrads = this->normInputSet->mutable_device_grad();
    const Dtype* gammas = this->gammaSet->device_data();

    ComputeNormInputGrad<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
        outputGrads, gammas, this->depth, batchCount, normInputGrads);
}

template <typename Dtype>
void BatchNormLayer<Dtype>::computeVarianceGrad() {
	const vector<uint32_t>& inputShape = this->_inputData[0]->getShape();
	int batchCount = inputShape[0];
    const Dtype* inputData = this->_inputData[0]->device_data();
    Dtype* varGrads = this->varSet->mutable_device_grad();
    const Dtype* normInputGrads = this->normInputSet->device_grad();
    const Dtype* means = this->meanSet->device_data();
    const Dtype* vars = this->varSet->device_data();

    ComputeVarianceGrad<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
        normInputGrads, inputData, means, vars, (Dtype)this->epsilon, depth, batchCount,
        varGrads);
}

template <typename Dtype>
void BatchNormLayer<Dtype>::computeMeanGrad() {
	const vector<uint32_t>& inputShape = this->_inputData[0]->getShape();
	int batchCount = inputShape[0];
    const Dtype* inputData = this->_inputData[0]->device_data();
    Dtype* meanGrads = this->meanSet->mutable_device_grad();
    const Dtype* normInputGrads = this->normInputSet->device_grad();
    const Dtype* vars = this->varSet->device_data();
    const Dtype* varGrads = this->varSet->device_grad();
    const Dtype* means = this->meanSet->device_data();

    ComputeMeanGrad<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
        normInputGrads, vars, varGrads, inputData, means, depth, batchCount,
        (Dtype)this->epsilon, meanGrads);
}

template <typename Dtype>
void BatchNormLayer<Dtype>::computeInputGrad() {
	const vector<uint32_t>& inputShape = this->_inputData[0]->getShape();
	int batchCount = inputShape[0];
    const Dtype* inputData = this->_inputData[0]->device_data();
	Dtype* inputGrads = this->_inputData[0]->mutable_device_grad();
    const Dtype* normInputGrads = this->normInputSet->device_grad();
    const Dtype* vars = this->varSet->device_data();
    const Dtype* varGrads = this->varSet->device_grad();
    const Dtype* means = this->meanSet->device_data();
    const Dtype* meanGrads = this->meanSet->device_grad();

    ComputeInputGrad<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
        normInputGrads, vars, varGrads, inputData, means, meanGrads, depth, batchCount,
        (Dtype)this->epsilon, inputGrads);
}

template <typename Dtype>
void BatchNormLayer<Dtype>::computeScaleGrad() {
	const vector<uint32_t>& inputShape = this->_inputData[0]->getShape();
	int batchCount = inputShape[0];
    const Dtype* outputGrads = this->_outputData[0]->device_grad();;
    Dtype* gammaGrads = this->gammaSet->mutable_device_grad();
    const Dtype* normInputs = this->normInputSet->device_data();

    ComputeScaleGrad<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
        normInputs, outputGrads, depth, batchCount, gammaGrads);
   
}

template <typename Dtype>
void BatchNormLayer<Dtype>::computeShiftGrad() {
	const vector<uint32_t>& inputShape = this->_inputData[0]->getShape();
	int batchCount = inputShape[0];
    const Dtype* outputGrads = this->_outputData[0]->device_grad();
    Dtype* betaGrads = this->betaSet->mutable_device_grad();

    ComputeShiftGrad<<<SOOOA_GET_BLOCKS(this->depth), SOOOA_CUDA_NUM_THREADS>>>(
        outputGrads, depth, batchCount, betaGrads);
}

template <typename Dtype>
void BatchNormLayer<Dtype>::backpropagation() {
    /*
     * 아래와 같은 simple한 network layer가 있다고 가정하자.
     *
     *               <<<< ith layer >>>>                        <<<< i+1th layer >>>>
     *   .....    Xi  Norm    ^Xi   γi * ^Xi + βi      Yi (=Xi+1)  ........
     *   .....    O ---------  O  ---------------------  O         ........
     *                                                     dL/dYi is already computed
     *
     *  (※  Xi = i번째 layer의 input 값, Norm = normaliztion
     *      ^Xi = i번째 layer의 중간 값, γi = scale factor, βi = shift factor
     *      Yi = i번째 layer의 ouput 값, i+1 번째 layer의 input 값이기도 함
     *      L = loss, dL/dYi = i+1번째 layer에서 계산되었던 gradient 값)
     *
     *  BatchNormLayer에서는 γi, βi를 학습해야 하는데 그것을 위해서 dL/dγi, dL/dβi를 계산해야
     *  한다. 또한, 하위 layer에 전달할 dL/dXi이 필요하다.
     *
     *  논문(https://arxiv.org/abs/1502.03167)에서 각각의 계산식이 있기 때문에 그것을 이용하여
     *  연산을 하도록 하자.)
     */

    // (1) dL/d^Xi = dL/dYi * γi
    computeNormInputGrad();

    // (2) dL/dSquaredSigma
    computeVarianceGrad();

    // (3) dL/dMean
    computeMeanGrad();

    // (4) dL/dXi
    computeInputGrad();

    // (5) dL/dγi
    computeScaleGrad();

    // (6) dL/dβi
    computeShiftGrad();
}

template BatchNormLayer<float>::~BatchNormLayer();
template void BatchNormLayer<float>::reshape();
template void BatchNormLayer<float>::update();
template void BatchNormLayer<float>::feedforward();
template void BatchNormLayer<float>::backpropagation();

#endif
