/**
 * @file DQNWork.cpp
 * @date 2016-12-23
 * @author moonhoen lee
 * @brief 
 * @details
 */

#include "common.h"
#include "DQNWork.h"
#include "SysLog.h"
#include "Network.h"
#include "LegacyWork.h"
#include "Broker.h"
#include "Debug.h"

using namespace std;

template<typename Dtype>
void DQNWork<Dtype>::createDQNImageLearner(Job* job) {
    // (1) Learner을 생성한다.
    int rowCount = job->getIntValue(0);
    int colCount = job->getIntValue(1);
    int channelCount = job->getIntValue(2);
    int actionCount = job->getIntValue(3);

    DQNImageLearner<Dtype>* learner =
        new DQNImageLearner<Dtype>(rowCount, colCount, channelCount, actionCount);
   
    // (2) Q Network와 Q head Network를 생성한다.
    int netQID = LegacyWork<Dtype>::createNetwork();
    int netQHeadID = LegacyWork<Dtype>::createNetwork();

    // (3) pubJob을 reqPubJobMap으로부터 얻는다.
    SASSUME0(job->hasPubJob());
    unique_lock<mutex> reqPubJobMapLock(Job::reqPubJobMapMutex); 
    Job *pubJob = Job::reqPubJobMap[job->getJobID()];
    SASSUME0(pubJob != NULL);
    Job::reqPubJobMap.erase(job->getJobID());
    reqPubJobMapLock.unlock();
    SASSUME0(pubJob->getType() == job->getPubJobType());

    // (4) pubJob에 elem을 채운다.
    int learnerID = learner->getID();
    pubJob->addJobElem(Job::IntType, 1, (void*)&learnerID);
    pubJob->addJobElem(Job::IntType, 1, (void*)&netQID);
    pubJob->addJobElem(Job::IntType, 1, (void*)&netQHeadID);

    // (5) pubJob을 publish한다.
    Broker::publish(pubJob->getJobID(), pubJob);
}

template<typename Dtype>
void DQNWork<Dtype>::cleanupDQNImageLearner(Job* job) {
    int learnerID = job->getIntValue(0);
    DQNImageLearner<Dtype>* learner = DQNImageLearner<Dtype>::getLearnerFromID(learnerID);
    SASSERT0(learner != NULL);
    delete learner;
    learner = NULL;
}

template<typename Dtype>
void DQNWork<Dtype>::buildDQNNetwork(DQNImageLearner<Dtype>* learner,
    Network<Dtype>* network) {
#if 0
    // (1) layer config를 만든다. 이 과정중에 layer들의 초기화가 진행된다.
	LayersConfig<float>* layersConfig = createDQNLayersConfig<float>();

    // (2) network config 정보를 layer들에게 전달한다.
    for(uint32_t i = 0; i < layersConfig->_layers.size(); i++) {
        layersConfig->_layers[i]->setNetworkConfig(network->config);
    }

    // (3) shape 과정을 수행한다. 
    ALEInputLayer<Dtype>* inputLayer = (ALEInputLayer<Dtype>*)layersConfig->_layers[0];
    inputLayer->setInputCount(learner->rowCnt, learner->colCnt, learner->chCnt,
        learner->actionCnt);
    for(uint32_t i = 0; i < layersConfig->_layers.size(); i++) {
    	layersConfig->_layers[i]->reshape();
    }

    // (4) network에 layersConfig 정보를 등록한다.
    network->setLayersConfig(layersConfig);
#endif
}

template<typename Dtype>
void DQNWork<Dtype>::buildDQNNetworks(Job* job) {
    // (1) get learner
    int learnerID = job->getIntValue(0);
    DQNImageLearner<Dtype>* learner = DQNImageLearner<Dtype>::getLearnerFromID(learnerID);
    SASSERT0(learner != NULL);

    // (2) build Q Network
    int netQID = job->getIntValue(1);
    Network<Dtype>* networkQ = Network<Dtype>::getNetworkFromID(netQID);
    buildDQNNetwork(learner, networkQ);

    // (3) build Q head Network
    int netQHeadID = job->getIntValue(2);
    Network<Dtype>* networkQHead = Network<Dtype>::getNetworkFromID(netQHeadID);
    buildDQNNetwork(learner, networkQHead);
}

template<typename Dtype>
void DQNWork<Dtype>::stepDQNImageLearner(Job* job) {
    // (1) get learner
    int learnerID = job->getIntValue(0);
    DQNImageLearner<Dtype>* learner = DQNImageLearner<Dtype>::getLearnerFromID(learnerID);

    // (2) get Q & Q head Network
    int netQID = job->getIntValue(1);
    Network<Dtype>* networkQ = Network<Dtype>::getNetworkFromID(netQID);
    int netQHeadID = job->getIntValue(2);
    Network<Dtype>* networkQHead = Network<Dtype>::getNetworkFromID(netQHeadID);

    // (3) fill replay memory
    Dtype lastReward = job->getFloatValue(3);
    int lastAction = job->getIntValue(4);
    int lastTerm = job->getIntValue(5);
    float* state = job->getFloatArray(6);
    learner->fillRM(lastReward, lastAction, lastTerm, state);

    // (4) choose action
    int action = learner->chooseAction(networkQ);

    // (5) train network Q
    if (learner->isReady()) {
        learner->syncNetworks(networkQ, networkQHead);
        learner->prepareActiveRMSlots();
        learner->forwardMiniBatch(networkQHead, false);
        learner->updateQLabelValues();
        learner->forwardMiniBatch(networkQ, true);
        learner->backwardMiniBatch(networkQ);
    }

    // (5) pubJob을 reqPubJobMap으로부터 얻는다.
    SASSUME0(job->hasPubJob());
    unique_lock<mutex> reqPubJobMapLock(Job::reqPubJobMapMutex); 
    Job *pubJob = Job::reqPubJobMap[job->getJobID()];
    SASSUME0(pubJob != NULL);
    Job::reqPubJobMap.erase(job->getJobID());
    reqPubJobMapLock.unlock();
    SASSUME0(pubJob->getType() == job->getPubJobType());

    // (6) pubJob에 elem을 채운다.
    pubJob->addJobElem(Job::IntType, 1, (void*)&action);

    // (7) pubJob을 publish한다.
    Broker::publish(pubJob->getJobID(), pubJob);
}

template class DQNWork<float>;
