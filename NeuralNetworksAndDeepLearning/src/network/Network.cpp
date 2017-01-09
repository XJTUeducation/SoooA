/*
 * Network.cpp
 *
 *  Created on: 2016. 4. 20.
 *      Author: jhkim
 */

#include <vector>
#include <map>
#include <cfloat>

#include "DataSet.h"
#include "LayerFactory.h"
#include "HiddenLayer.h"
#include "SoftmaxWithLossLayer.h"
#include "LossLayer.h"
#include "Util.h"
#include "Worker.h"
#include "Perf.h"
#include "StdOutLog.h"
#include "Network.h"

using namespace std;


template <typename Dtype>
Network<Dtype>::Network(NetworkConfig<Dtype>* config)
	: config(config) {
	//DataSet<Dtype>* dataSet = config->_dataSet;
	//this->in_dim.rows = dataSet->getRows();
	//this->in_dim.cols = dataSet->getCols();
	//this->in_dim.channels = dataSet->getChannels();
	//this->in_dim.batches = config->_batchSize;
}

template <typename Dtype>
InputLayer<Dtype>* Network<Dtype>::getInputLayer() {
    LayersConfig<Dtype>* layersConfig = config->layersConfigs[Worker<Dtype>::consumerIdx];
    return dynamic_cast<InputLayer<Dtype>*>(layersConfig->_firstLayers[0]);
}

template <typename Dtype>
LayersConfig<Dtype>* Network<Dtype>::getLayersConfig() {
    return config->layersConfigs[Worker<Dtype>::consumerIdx];
}

template <typename Dtype>
void Network<Dtype>::setLayersConfig(LayersConfig<Dtype>* layersConfig) {
    config->layersConfigs[Worker<Dtype>::consumerIdx] = layersConfig;
}


template <typename Dtype>
Network<Dtype>::~Network() {
    /*
    typename vector<LayersConfig<Dtype>*>::iterator iter;
    for (iter = config->layersConfigs.begin(); iter != config->layersConfigs.end(); iter++) {
        if ((*iter)->_inputLayer) {
            delete (*iter)->_inputLayer;
            (*iter)->_inputLayer = NULL;
        }
    }
    */
}

template <typename Dtype>
void Network<Dtype>::sgd_with_timer(int epochs) {
    struct timespec startTime;
    SPERF_START(NETWORK_TRAINING_TESTTIME, &startTime);
    // XXX: 임시
    epochs = 1000000;
    //epochs = 1;
	sgd(epochs);

    SPERF_END(NETWORK_TRAINING_TESTTIME, startTime, epochs);
    STDOUT_BLOCK(cout << "Total Training Time : " << SPERF_TIME(NETWORK_TRAINING_TESTTIME)
                    << endl;);
}




#define SAVE_PROPOSAL_TARGET_LAYER 0


template <typename Dtype>
void Network<Dtype>::sgd(int epochs) {
	DataSet<Dtype>* dataSet = getLayersConfig()->_inputLayer->_dataSet;
	//vector<vector<Evaluation<Dtype>*>>& evaluations = config->_evaluations;
	vector<NetworkListener*>& networkListeners = config->_networkListeners;

	const uint32_t trainDataSize = dataSet->getNumTrainData();
	const uint32_t numBatches = trainDataSize / config->_batchSize / Worker<Dtype>::consumerCount;

	Timer timer1;
	Timer timer2;


	//iterations = 0;
	for (uint32_t epochIndex = 0; epochIndex < epochs; epochIndex++) {
		config->_status = NetworkStatus::Train;


		cout << "epochIndex: " << epochIndex << ", epochs: " << epochs << endl;

		dataSet->shuffleTrainDataSet();
		timer1.start();
		timer2.start();

        // GPU가 여러대 있는 경우에 한대의 GPU가 하나의 batch에 해당하는
        // 데이터를 트레이닝한다.
        // XXX: GPU 대수는 numBatches의 최소공약수라 가정한다. 
        //      (나중에 고쳐야 한다.)


		LayersConfig<Dtype>* layersConfig = getLayersConfig();
		vector<double> costList(config->_lossLayers.size());
		typename map<string, Layer<Dtype>*>::iterator it;


#if SAVE_PROPOSAL_TARGET_LAYER
		ofstream ofs(config->_savePathPrefix + "/proposal_target_layer.ptl", ios::out | ios::binary);
		const uint32_t numData = numBatches*5;
		ofs.write((char*)&numData, sizeof(uint32_t));
#endif
		double cost;
		for (uint32_t batchTotalIndex = 0; batchTotalIndex < numBatches; batchTotalIndex++) {
            uint32_t batchIndex = batchTotalIndex * Worker<Dtype>::consumerCount +
                Worker<Dtype>::consumerIdx;
			config->_iterations++;

			/*
			if((batchIndex+1)%100 == 0) {
				cout << "Minibatch " << batchIndex+1 <<
						" started: " << timer2.stop(false) << endl;
				timer2.start();
			}
			*/
#ifndef GPU_MODE
			inputLayer->reset_nabla(0);
#endif
			trainBatch(batchIndex);



#if SAVE_PROPOSAL_TARGET_LAYER
			saveProposalTargets(ofs);
#endif



			// UPDATE
			if (config->_phase == NetworkPhase::TrainPhase) {
				for (uint32_t i = 0; i < config->_lossLayers.size(); i++) {
					it = layersConfig->_nameLayerMap.find(config->_lossLayers[i]);
					assert(it != layersConfig->_nameLayerMap.end());
					LossLayer<Dtype>* lossLayer = dynamic_cast<LossLayer<Dtype>*>(it->second);
					assert(lossLayer != 0);
					costList[i] += lossLayer->cost();
				}
				applyUpdate();
			}

            // 모든 worker에서 GPU 트레이닝이 끝나길 기다린다.
            // XXX: 예쁘게.. 
            if (Worker<Dtype>::waitPeer()) {
                // 마지막 쓰레드가 메모리를 갱신한다.
                if (config->_phase == NetworkPhase::TrainPhase && config->doTest()) {
                    config->_status = NetworkStatus::Test;


					for (uint32_t i = 0; i < config->_lossLayers.size(); i++) {
						float cost = costList[i]/config->_testInterval;
						networkListeners[i]->onCostComputed(0, config->_lossLayers[i], cost);
						costList[i] = 0.0;
						cout << config->_lossLayers[i] << " cost:" << cost << ",";
					}
					cout << endl;




                    //const uint32_t numTestData = dataSet->getNumTestData();
                    //if(numTestData > 0) {
                    //    double cost = evaluateTestSet();
                    //    cost /= numTestData;

                        /*
                        cout << "epoch: " << epochIndex+1 <<
                        		", iteration: " << epochIndex*numBatches+batchTotalIndex+1 << " " <<
                        		accurateCnt << " / " << numTestData <<
                        		", accuracy: " << accuracy <<
                        		", cost: " << cost <<
                        		" :" << timer1.stop(false) << endl;
                        		*/
                        /*
                        //const float cost = evaluations[0]->getCost() / numTestData;
                        const uint32_t accurateCnt = evaluations[0]->getAccurateCount();
                        const float accuracy = (float)accurateCnt/numTestData;
                        
                        cout << "epoch: " << epochIndex+1 << ", iteration: " 
                            << epochIndex*numBatches+batchTotalIndex+1 << " " << accurateCnt << " / " 
                            << numTestData << ", accuracy: " << accuracy << ", cost: " << cost 
                            << " :" << timer1.stop(false) << endl;
                        
                        for(uint32_t nl = 0; nl < networkListeners.size(); nl++) {
                            networkListeners[nl]->onAccuracyComputed(0, "top1_accuracy",
                                (double)evaluations[0]->getAccurateCount()/numTestData*100);
                            networkListeners[nl]->onAccuracyComputed(1, "top5_accuracy",
                                (double)evaluations[1]->getAccurateCount()/numTestData*100);
                            //networkListeners[nl]->onCostComputed(0, "cost", evaluations[0]->getCost()/numTestData);
                            networkListeners[nl]->onCostComputed(0, "cost", cost);
                        }
                        */
                    //}
                    config->_status = NetworkStatus::Train;
                }

                if(config->_phase == NetworkPhase::TrainPhase && config->doSave()) {
                    save();
                }

                Worker<Dtype>::wakeupPeer();
            }
		}

#if SAVE_PROPOSAL_TARGET_LAYER
		ofs.close();
#endif


	}
}


template <typename Dtype>
double Network<Dtype>::evaluateTestSet() {
	DataSet<Dtype>* dataSet = getLayersConfig()->_inputLayer->_dataSet;
	//vector<vector<Evaluation<Dtype>*>>& evaluations = config->_evaluations;
	double cost = 0.0;

	//for (uint32_t i = 0; i < evaluations.size(); i++) {
	//	for (uint32_t j = 0; j < evaluations[i].size(); j++)
	//		evaluations[i][j]->reset();
	//}

	vector<double> costList;
	const uint32_t numBatches = dataSet->getNumTestData()/config->_batchSize;
	for (uint32_t batchIndex = 0; batchIndex < numBatches; batchIndex++) {
		//cost += evaluateTestData(batchIndex);
		evaluateTestData(batchIndex, costList);
	}

	for (uint32_t i = 0; i < config->_lossLayers.size(); i++) {
		cout << costList[i] / numBatches << ", ";
	}
	cout << endl;

	return cost;
}

template <typename Dtype>
double Network<Dtype>::evaluateTestData(uint32_t batchIndex, vector<double>& costList) {
	LayersConfig<Dtype>* layersConfig = getLayersConfig();

#ifndef OUTPUTLAYER
#else
	OutputLayer<Dtype>* lossLayer = layersConfig->_lossLayers[0];
#endif
	int baseIndex = batchIndex*config->_batchSize;

	_feedforward(batchIndex);

	costList.assign(config->_lossLayers.size(), 0.0);

	typename map<string, Layer<Dtype>*>::iterator it;
	for (uint32_t i = 0; i < config->_lossLayers.size(); i++) {
		it = layersConfig->_nameLayerMap.find(config->_lossLayers[i]);
		assert(it != layersConfig->_nameLayerMap.end());

		LossLayer<Dtype>* lossLayer = dynamic_cast<LossLayer<Dtype>*>(it->second);
		assert(lossLayer != 0);

		costList[i] += lossLayer->cost();
	}


	/*
	LossLayer<Dtype>* lossLayer = layersConfig->_lossLayers[0];
	LossLayer<Dtype>* softmaxWithLoss =
			dynamic_cast<SoftmaxWithLossLayer<Dtype>*>(lossLayer);
	assert(softmaxWithLoss);

	const uint32_t numLabels = softmaxWithLoss->prob->getShape(2);
	Data<Dtype>* networkOutput = softmaxWithLoss->prob;

	double cost = lossLayer->cost();

	networkOutput->print_data("networkOutput:");
	const Dtype* output = networkOutput->host_data();
	DataSet<Dtype>* dataSet = layersConfig->_inputLayer->_dataSet;
	for(int i = 0; i < config->_evaluations.size(); i++) {
		config->_evaluations[i]->evaluate(numLabels, config->_batchSize,
				output, dataSet, baseIndex);
	}
	*/

	//return cost;
	return 0.0;
}

template <typename Dtype>
void Network<Dtype>::test() {
	config->_status = NetworkStatus::Test;
	/*
	DataSet<Dtype>* dataSet = getLayersConfig()->_inputLayer->_dataSet;
	vector<vector<Evaluation<Dtype>*>>& evaluations = config->_evaluations;

	Timer timer;
	float numTestData = (float)dataSet->getNumTestData();
	double cost = evaluateTestSet();
	cost /= numTestData;
	int accurateCnt = evaluations[0]->getAccurateCount();
	float accuracy = accurateCnt / numTestData;

	if(dataSet->getNumTestData() > 0) {
		timer.start();
		cout << accurateCnt << " / " << numTestData << ", accuracy: " << accuracy <<
				", cost: " << cost << " :" << timer.stop(false) << endl;
	}
	*/
}


#ifndef GPU_MODE
template <typename Dtype>
void Network<Dtype>::feedforward(const rcube &input, const char *end) {
	//cout << "feedforward()" << endl;
	inputLayer->feedforward(0, input, end);

}

template <typename Dtype>
void Network<Dtype>::updateMiniBatch(int nthMiniBatch, int miniBatchSize) {

	int baseIndex = nthMiniBatch*miniBatchSize;
	for(int i = 0; i < miniBatchSize; i++) {
		backprop(dataSet->getTrainDataAt(baseIndex+i));
	}

	int n = dataSet->getTrainData();

	//cout << "update()" << endl;
	//inputLayer->update(0, n, miniBatchSize);
}

template <typename Dtype>
void Network<Dtype>::backprop(const DataSample &dataSample) {
	//Timer timer;
	//timer.start();
	// feedforward
	feedforward(dataSample.getData());

	//cout << "time for feed forward: ";
	//timer.stop();

	//timer.start();
	//cout << "backpropagation()" << endl;
	for(UINT i = 0; i < outputLayers.size(); i++) {
		//outputLayers[i]->cost(dataSample.getTarget());
	}
	//cout << "time for backward: ";
	//timer.stop();
}

/*
double Network<Dtype>::totalCost(const vector<const DataSample *> &dataSet, double lambda) {
	double cost = 0.0;
	int dataSize = dataSet.size();

	for(int i = 0; i < dataSize; i++) {
		vec activation = feedforward(dataSet[i]->getData());
		cost += this->cost->fn(&activation, dataSet[i]->getTarget());
	}
	cost /= dataSize;

	// add weight decay term of cost
	for(int i = 1; i < numLayers; i++) {
		cost += 0.5*(lambda/dataSize)*accu(square(*weights[i]));
	}
	return cost;
}

double Network<Dtype>::accuracy(const vector<const DataSample *> &dataSet) {
	int total = 0;
	int dataSize = dataSet.size();
	for(int i = 0; i < dataSize; i++) {
		const DataSample *dataSample = dataSet[i];
		Util::printVec(dataSample->getData(), "data");
		Util::printVec(dataSample->getTarget(), "target");
		total += testEvaluateResult(feedforward(dataSample->getData()), dataSample->getTarget());
	}
	return total/(double)dataSize;
}
*/

template <typename Dtype>
int Network<Dtype>::testEvaluateResult(const rvec &output, const rvec &y) {
	//Util::printVec(&evaluateResult, "result");
	//Util::printVec(y, "y");

	uword rrow, yrow;
	output.max(rrow);
	y.max(yrow);

	if(rrow == yrow) return 1;
	else return 0;
}
#else


template <typename Dtype>
void Network<Dtype>::trainBatch(uint32_t batchIndex) {
	_feedforward(batchIndex);

	if (config->_phase == NetworkPhase::TrainPhase)
		_backpropagation(batchIndex);
}

#endif

template <typename Dtype>
void Network<Dtype>::applyUpdate() {
	clipGradients();
	const uint32_t numLearnableLayers = getLayersConfig()->_learnableLayers.size();

    // device 메모리를 host 메모리로 동기화 시킨다.
    for (uint32_t i = 0; i < numLearnableLayers; i++) {
        getLayersConfig()->_learnableLayers[i]->syncMutableMem();
    }


    // 모든 worker에서 GPU 트레이닝이 끝나길 기다린다.
    // XXX: 예쁘게.. 
    if (Worker<Dtype>::waitPeer()) {
        typename vector<LayersConfig<Dtype>*>::iterator iter; 
        LayersConfig<Dtype>* firstLayersConfig;
        // 마지막 쓰레드가 learnable layer들의 data를 갱신한다.

        // (1) 변화된 부분을 첫번째 layer에 적용한다.
        for (iter = config->layersConfigs.begin(); iter != config->layersConfigs.end(); iter++) {
            
            if (iter == config->layersConfigs.begin()) {
                firstLayersConfig = (*iter);
                continue;
            }

            for (uint32_t i = 0; i < numLearnableLayers; i++) {
                LearnableLayer<Dtype>* firstLayer = firstLayersConfig->_learnableLayers[i];
                LearnableLayer<Dtype>* curLayer = (*iter)->_learnableLayers[i];

                curLayer->applyChanges(firstLayer);
            }
        }

        // (2) 첫번째 layer의 값을 다른 layer들에게 동기화 한다.
        for (iter = config->layersConfigs.begin(); iter != config->layersConfigs.end(); iter++) {
            if (iter == config->layersConfigs.begin()) {
                continue;
            }

            for (uint32_t i = 0; i < numLearnableLayers; i++) {
                LearnableLayer<Dtype>* firstLayer = firstLayersConfig->_learnableLayers[i];
                LearnableLayer<Dtype>* curLayer = (*iter)->_learnableLayers[i];

                curLayer->syncParams(firstLayer);
            }
        }

        Worker<Dtype>::wakeupPeer();
    }

    // 각 layer들을 갱신한다.
	for (uint32_t i = 0; i < numLearnableLayers; i++) {
        getLayersConfig()->_learnableLayers[i]->update();
	}
}

template <typename Dtype>
void Network<Dtype>::clipGradients() {
	const float clipGradientsLevel = config->_clipGradientsLevel;
	const double sumsqParamsGrad = computeSumSquareParamsGrad();
	const double sumsqParamsData = computeSumSquareParamsData();

	const double l2normParamsGrad = sqrt(sumsqParamsGrad);
	const double l2normParamsData = sqrt(sumsqParamsData);

	if (clipGradientsLevel < 0.0001) {
		//cout << "Gradient clipping: no scaling down gradients (L2 norm " << l2normParamsGrad <<
		//		", Weight: " << l2normParamsData << " <= " << clipGradientsLevel << ")" << endl;
	} else {
		if (l2normParamsGrad > clipGradientsLevel) {
			const float scale_factor = clipGradientsLevel / (l2normParamsGrad*1);

			cout << "Gradient clipping: scaling down gradients (L2 norm " << l2normParamsGrad <<
					", Weight: " << l2normParamsData << " > " << clipGradientsLevel <<
					") by scale factor " << scale_factor << endl;
			scaleParamsGrad(scale_factor);
		} else {
			cout << "Gradient clipping: no scaling down gradients (L2 norm "
                << l2normParamsGrad << ", Weight: " << l2normParamsData << " <= "
                << clipGradientsLevel << ")" << endl;
		}
	}
}

template <typename Dtype>
double Network<Dtype>::computeSumSquareParamsData() {
	uint32_t numLearnableLayers = getLayersConfig()->_learnableLayers.size();
	double sumsq = 0.0;
	for(uint32_t i = 0; i < numLearnableLayers; i++) {
		double temp = getLayersConfig()->_learnableLayers[i]->sumSquareParamsData();
		//if(i >= numLearnableLayers-10) { // && i < numLearnableLayers-1) {
		if(i < 0) {
			config->_networkListeners[0]->onDataSumsqComputed(
					//i-(numLearnableLayers-10),
					i,
					getLayersConfig()->_learnableLayers[i]->getName(),
					sqrt(temp));
		}
		sumsq += temp;
	}
	return sumsq;
}

template <typename Dtype>
double Network<Dtype>::computeSumSquareParamsGrad() {
	uint32_t numLearnableLayers = getLayersConfig()->_learnableLayers.size();
	double sumsq = 0.0;
	for(uint32_t i = 0; i < numLearnableLayers; i++) {
		double temp = getLayersConfig()->_learnableLayers[i]->sumSquareParamsGrad();
		//if(i < 10) {
		if(i < 0) {
			config->_networkListeners[0]->onGradSumsqComputed(
					i,
					getLayersConfig()->_learnableLayers[i]->getName(),
					sqrt(temp));
		}
		sumsq += temp;
		//cout << getLayersConfig()->_learnableLayers[i]->getName() << ", grad l2-norm: " << sqrt(temp) << endl;
	}
	return sumsq;
}

template <typename Dtype>
void Network<Dtype>::scaleParamsGrad(float scale) {
	uint32_t numLearnableLayers = getLayersConfig()->_learnableLayers.size();
	for(uint32_t i = 0; i < numLearnableLayers; i++) {
		getLayersConfig()->_learnableLayers[i]->scaleParamsGrad(scale);
	}
}





template <typename Dtype>
void Network<Dtype>::save() {
	//if(saveConfigured && cost < minCost) {
	//	minCost = cost;
	//	char savePath[256];
	//	sprintf(savePath, "%s%02d.network", savePrefix, i+1);
	//	save(savePath);


	/*
	InputLayer<Dtype>* inputLayer = getLayersConfig()->_inputLayer;
	vector<OutputLayer<Dtype>*>& outputLayers = getLayersConfig()->_outputLayers;

	Timer timer;
	timer.start();

	ofstream ofs(filename, ios::out | ios::binary);
	//int inputLayerSize = 1;
	int outputLayerSize = outputLayers.size();

	ofs.write((char *)&in_dim, sizeof(io_dim));
	//ofs.write((char *)&inputLayerSize, sizeof(int));		// input layer size
	//ofs.write((char *)&inputLayer, sizeof(Layer<Dtype>*));		// input layer address
	ofs.write((char *)&outputLayerSize, sizeof(UINT));		// output layer size
	for(UINT i = 0; i < outputLayers.size(); i++) {
		ofs.write((char *)&outputLayers[i], sizeof(Layer<Dtype>*));
	}
	inputLayer->save(0, ofs);
	ofs.close();

	cout << "time elapsed to save network: " << timer.stop(false) << endl;
	*/

	config->save();
}



template <typename Dtype>
void Network<Dtype>::loadPretrainedWeights() {
	if (config->_weightsArgs.size() < 1) return;

	const uint32_t numWeightsArgs = config->_weightsArgs.size();

	// load data list from model file
	map<std::string, Data<float>*> dataMap;

	for (uint32_t i = 0; i < numWeightsArgs; i++) {
		ifstream ifs(config->_weightsArgs[i].weightsPath, std::ios::in | std::ios::binary);
		map<string, string>& weightsMap = config->_weightsArgs[i].weightsMap;

		uint32_t numData;
		ifs.read((char*)&numData, sizeof(uint32_t));

		Data<float>::printConfig = true;
		cout << "Load Pretrained Weights ... ----------" << endl;
		for (uint32_t j = 0; j < numData; j++) {
			Data<float>* data = new Data<float>("", true);
			data->load(ifs);

			if (data)
				data->print();

			string dataName;
			if (weightsMap.size() < 1)
				dataName = data->_name;
			else {
				map<string, string>::iterator it;
				it = weightsMap.find(data->_name);
				if (it == weightsMap.end()) {
					dataName = data->_name;
				} else {
					dataName = it->second;
				}
			}

			map<string, Data<float>*>::iterator it;
			it = dataMap.find(dataName);
			if (it != dataMap.end()) {
				cout << dataName << " overwrites ... " << endl;
				delete it->second;
			}

			dataMap[dataName] = data;
			cout << data->_name << " is set to " << dataName << endl;

			/*
			// 별도의 weight name을 지정하지 않은 경우 전체 weights load
			if (weightsMap.size() < 1) {
				dataMap[data->_name] = data;

				cout << data->_name << " is set to " << data->_name << endl;
			}
			// 별도의 weight name을 지정한 경우 weights update
			else {
				map<string, string>::iterator it;
				it = weightsMap.find(data->_name);
				if (it == weightsMap.end()) {
					dataMap[data->_name] = data;
					cout << data->_name << " is set to " << data->_name << endl;
				} else {
					dataMap[it->second] = data;
					cout << data->_name << " is set to " << it->second << endl;
				}
			}
			*/

			/*
			// 별도의 weight name을 지정하지 않은 경우 전체 weights load
			if (config->_weightsArgs[i].weights.size() < 1) {
				dataMap[data->_name] = data;
			}
			// 별도의 weight name을 지정한 경우 weights update
			else {
				uint32_t k;
				for (k = 0; k < config->_weightsArgs[i].weights.size(); k++) {
					if (data->_name == config->_weightsArgs[i].weights[k]) {
						dataMap[data->_name] = data;
						break;
					}
				}

				if (k == config->_weightsArgs[i].weights.size()) {
					delete data;
					data = 0;
				}
			}
			*/


		}
		cout << "--------------------------------------" << endl;
		Data<float>::printConfig = false;
		ifs.close();
	}










	LayersConfig<Dtype>* layersConfig = getLayersConfig();
	vector<LearnableLayer<Dtype>*> learnableLayers = layersConfig->_learnableLayers;
	const uint32_t numLearnableLayers = learnableLayers.size();

	for (uint32_t i = 0; i < numLearnableLayers; i++) {
		learnableLayers[i]->loadParams(dataMap);
	}

	map<std::string, Data<float>*>::iterator it;
	for (it = dataMap.begin(); it != dataMap.end(); it++)
		delete it->second;
	dataMap.clear();

}



/*
template <typename Dtype>
void Network<Dtype>::loadPretrainedWeights() {
	if (config->_weightsPath == "") return;

	// load data list from model file
	map<std::string, Data<float>*> dataMap;
	ifstream ifs(config->_weightsPath, std::ios::in | std::ios::binary);

	uint32_t numData;
	ifs.read((char*)&numData, sizeof(uint32_t));

	Data<float>::printConfig = true;
	cout << "Load Pretrained Weights ... ----------" << endl;
	for (uint32_t i = 0; i < numData; i++) {
		Data<float>* data = new Data<float>("");
		data->load(ifs);
		dataMap[data->_name] = data;
		data->print();
	}
	cout << "--------------------------------------" << endl;
	Data<float>::printConfig = false;
	ifs.close();

	LayersConfig<Dtype>* layersConfig = getLayersConfig();
	vector<LearnableLayer<Dtype>*> learnableLayers = layersConfig->_learnableLayers;
	const uint32_t numLearnableLayers = learnableLayers.size();

	for (uint32_t i = 0; i < numLearnableLayers; i++) {
		learnableLayers[i]->loadParams(dataMap);
	}

	map<std::string, Data<float>*>::iterator it;
	for (it = dataMap.begin(); it != dataMap.end(); it++)
		delete it->second;
	dataMap.clear();
}
*/


template <typename Dtype>
Layer<Dtype>* Network<Dtype>::findLayer(const string name) {
	//return getLayersConfig()->_inputLayer->find(0, name);
	map<string, Layer<Dtype>*>& nameLayerMap = getLayersConfig()->_nameLayerMap;
	typename map<string, Layer<Dtype>*>::iterator it = nameLayerMap.find(name);
	if(it != nameLayerMap.end()) {
		return it->second;
	} else {
		return 0;
	}
}


template <typename Dtype>
void Network<Dtype>::_feedforward(uint32_t batchIndex) {
	LayersConfig<Dtype>* layersConfig = getLayersConfig();
	InputLayer<Dtype>* inputLayer = layersConfig->_inputLayer;
	int baseIndex = batchIndex*config->_batchSize;

	inputLayer->feedforward(baseIndex);
	for (uint32_t i = 1; i < layersConfig->_layers.size(); i++) {
		//cout << layersConfig->_layers[i]->name << ": feedforward ... " << endl;
		layersConfig->_layers[i]->feedforward();
	}
}


template <typename Dtype>
void Network<Dtype>::_backpropagation(uint32_t batchIndex) {
	LayersConfig<Dtype>* layersConfig = getLayersConfig();

	for (int i = layersConfig->_layers.size()-1; i >= 0; i--) {
		HiddenLayer<Dtype>* hiddenLayer =
				dynamic_cast<HiddenLayer<Dtype>*>(layersConfig->_layers[i]);
		if (hiddenLayer) {
			//cout << layersConfig->_layers[i]->name << ": backpropagation ... " << endl;
			hiddenLayer->backpropagation();
		}

		/*
		else {
			cout << layersConfig->_layers[i]->name <<
					" is not a hiddenLayer, so skip backpropagation ... " << endl;
		}
		*/
	}
}




template <typename Dtype>
void Network<Dtype>::saveProposalTargets(ofstream& ofs) {
	LayersConfig<Dtype>* layersConfig = getLayersConfig();

	typename map<string, Layer<Dtype>*>::iterator it = layersConfig->_nameLayerMap.find("roi-data");
	assert(it != layersConfig->_nameLayerMap.end());

	Layer<Dtype>* proposalTargetLayer = it->second;
	const uint32_t numOutputs = proposalTargetLayer->_outputs.size();

	for (uint32_t i = 0; i < numOutputs; i++) {
		proposalTargetLayer->_outputData[i]->save(ofs);
	}


	//Data<Dtype>::printConfig = true;
	//proposalTargetLayer->_outputData[0]->print_data({}, false);
	//Data<Dtype>::printConfig = false;
}


template class Network<float>;





































