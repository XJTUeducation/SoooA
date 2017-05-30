/*
 * DetectionEvaluateLayer.cpp
 *
 *  Created on: May 15, 2017
 *      Author: jkim
 */

#include "DetectionEvaluateLayer.h"
#include "MathFunctions.h"
#include "BBoxUtil.h"
#include "SysLog.h"

#define DETECTIONEVALUATIONLAYER_LOG 1

using namespace std;

template <typename Dtype>
DetectionEvaluateLayer<Dtype>::DetectionEvaluateLayer(Builder* builder)
: Layer<Dtype>(builder) {
	initialize(builder);
}

template <typename Dtype>
DetectionEvaluateLayer<Dtype>::~DetectionEvaluateLayer() {
}


template <typename Dtype>
void DetectionEvaluateLayer<Dtype>::reshape() {
	Layer<Dtype>::_adjustInputShape();

	bool inputShapeChanged = false;
	for (int i = 0; i < this->_inputData.size(); i++) {
		if (Layer<Dtype>::_isInputShapeChanged(0)) {
			inputShapeChanged = true;
			break;
		}
	}

	if (!inputShapeChanged)
		return;

	SASSERT0(this->count <= this->sizes.size());
	SASSERT0(this->_inputData[0]->batches() == 1);
	SASSERT0(this->_inputData[0]->channels() == 1);
	SASSERT0(this->_inputData[0]->width() == 7);
	SASSERT0(this->_inputData[1]->batches() == 1);
	SASSERT0(this->_inputData[1]->channels() == 1);
	SASSERT0(this->_inputData[1]->width() == 8);

	// batches() and channels() are 1.
	vector<uint32_t> outputShape(2, 1);
	int numPosClasses = this->backgroundLabelId == -1 ?
			this->numClasses : this->numClasses - 1;
	int numValidDet = 0;
	const Dtype* detData = this->_inputData[0]->host_data();
	for (int i = 0; i < this->_inputData[0]->height(); i++) {
		if (detData[1] != -1) {
			++numValidDet;
		}
		detData += 7;
	}
	outputShape.push_back(numPosClasses + numValidDet);
	// Each row is a 5 dimension vector, which stores
	// [image_id, label, confidence, truePos, falsePos]
	outputShape.push_back(5);
	this->_outputData[0]->reshape(outputShape);
}

template <typename Dtype>
void DetectionEvaluateLayer<Dtype>::feedforward() {
	reshape();

	const Dtype* detData = this->_inputData[0]->host_data();
	const Dtype* gtData = this->_inputData[1]->host_data();

	// Retrieve all detection results.
	map<int, LabelBBox> allDetections;
	GetDetectionResults(detData, this->_inputData[0]->height(), this->backgroundLabelId,
			&allDetections);

#if DETECTIONEVALUATIONLAYER_LOG && false
	for (map<int, LabelBBox>::iterator it = allDetections.begin();
			it != allDetections.end(); it++) {
		cout << "key: " << it->first << endl;
		LabelBBox& labelBBox = it->second;

		for (LabelBBox::iterator iit = labelBBox.begin(); iit != labelBBox.end(); iit++) {
			cout << iit->first << endl;
			for (int j = 0; j < iit->second.size(); j++) {
				iit->second[j].print();
				cout << "-------" << endl;
			}
		}
	}
#endif

	// Retrieve all ground truth (including difficult ones).
	map<int, LabelBBox> allGtBBoxes;
	GetGroundTruth(gtData, this->_inputData[1]->height(), this->backgroundLabelId, true,
			&allGtBBoxes);

#if DETECTIONEVALUATIONLAYER_LOG && false
	for (map<int, LabelBBox>::iterator it = allGtBBoxes.begin();
			it != allGtBBoxes.end(); it++) {
		cout << "key: " << it->first << endl;
		LabelBBox& labelBBox = it->second;

		for (LabelBBox::iterator iit = labelBBox.begin(); iit != labelBBox.end(); iit++) {
			cout << iit->first << endl;
			for (int j = 0; j < iit->second.size(); j++) {
				iit->second[j].print();
				cout << "-------" << endl;
			}
		}
	}
#endif

	Dtype* outputData = this->_outputData[0]->mutable_host_data();
	soooa_set(this->_outputData[0]->getCount(), Dtype(0.), outputData);
	int numDet = 0;

	// Insert number of ground truth for each label.
	map<int, int> numPos;
	for (map<int, LabelBBox>::iterator it = allGtBBoxes.begin();
			it != allGtBBoxes.end(); it++) {
		for (LabelBBox::iterator iit = it->second.begin(); iit != it->second.end(); iit++) {
			int count = 0;
			if (this->evaluateDifficultGt) {
				count = iit->second.size();
			} else {
				// Get number of non difficult ground truth.
				for (int i = 0; i < iit->second.size(); i++) {
					if (!iit->second[i].difficult) {
						count++;
					}
				}
			}
			if (numPos.find(iit->first) == numPos.end()) {
				numPos[iit->first] = count;
			} else {
				numPos[iit->first] += count;
			}
		}
	}

#if DETECTIONEVALUATIONLAYER_LOG && false
	for (map<int, int>::iterator it = numPos.begin(); it != numPos.end(); it++) {
		cout << "key: " << it->first << ", value: " << it->second << endl;
	}
#endif


	for (int c = 0; c < this->numClasses; c++) {
		if (c == this->backgroundLabelId) {
			continue;
		}
		outputData[numDet * 5] = -1;
		outputData[numDet * 5 + 1] = c;
		if (numPos.find(c) == numPos.end()) {
			outputData[numDet * 5 + 2] = 0;
		} else {
			outputData[numDet * 5 + 2] = numPos.find(c)->second;
		}
		outputData[numDet * 5 + 3] = -1;
		outputData[numDet * 5 + 4] = -1;
		++numDet;
	}

#if DETECTIONEVALUATIONLAYER_LOG && false
	this->_printOn();
	this->_outputData[0]->print_data({}, false, -1);
	this->_printOff();
#endif

	// Insert detection evaluate status.
	for (map<int, LabelBBox>::iterator it = allDetections.begin();
			it != allDetections.end(); it++) {

		int imageId = it->first;
		LabelBBox& detections = it->second;
		if (allGtBBoxes.find(imageId) == allGtBBoxes.end()) {
			// No ground truth for current image. All detections become falsePos.
			for (LabelBBox::iterator iit = detections.begin();
					iit != detections.end(); iit++) {

				int label = iit->first;
				if (label == -1) {
					continue;
				}
				const vector<NormalizedBBox>& bboxes = iit->second;
				for (int i = 0; i < bboxes.size(); i++) {
					outputData[numDet * 5] = imageId;
					outputData[numDet * 5 + 1] = label;
					outputData[numDet * 5 + 2] = bboxes[i].score;
					outputData[numDet * 5 + 3] = 0;
					outputData[numDet * 5 + 4] = 1;
					numDet++;
				}
			}
		} else {
			LabelBBox& labelBBoxes = allGtBBoxes.find(imageId)->second;
			for (LabelBBox::iterator iit = detections.begin();
					iit != detections.end(); iit++) {
				int label = iit->first;
				if (label == -1) {
					continue;
				}
				vector<NormalizedBBox>& bboxes = iit->second;
				if (labelBBoxes.find(label) == labelBBoxes.end()) {
					// No ground truth for current label. All detections become falsePos.
					for (int i = 0; i < bboxes.size(); i++) {
						outputData[numDet * 5] = imageId;
						outputData[numDet * 5 + 1] = label;
						outputData[numDet * 5 + 2] = bboxes[i].score;
						outputData[numDet * 5 + 3] = 0;
						outputData[numDet * 5 + 4] = 1;
						numDet++;
					}
				} else {
					vector<NormalizedBBox>& gtBBoxes = labelBBoxes.find(label)->second;
					// Scale ground truth if needed.
					if (!this->useNormalizedBBox) {
						SASSERT0(this->count < this->sizes.size());
						for (int i = 0; i < gtBBoxes.size(); i++) {
							OutputBBox(gtBBoxes[i], this->sizes[this->count], false,
									&(gtBBoxes[i]));
						}
					}
					vector<bool> visited(gtBBoxes.size(), false);
					// Sort detections in descend order based on scores.
					std::sort(bboxes.begin(), bboxes.end(), SortBBoxDescend);
					for (int i = 0; i < bboxes.size(); i++) {
						outputData[numDet * 5] = imageId;
						outputData[numDet * 5 + 1] = label;
						outputData[numDet * 5 + 2] = bboxes[i].score;
						if (!this->useNormalizedBBox) {
							OutputBBox(bboxes[i], this->sizes[this->count], false,
									&(bboxes[i]));
						}
						// Compare with each ground truth bbox.
						float overlapMax = -1;
						int jmax = -1;
						for (int j = 0; j < gtBBoxes.size(); j++) {
							float overlap = JaccardOverlap(bboxes[i], gtBBoxes[j],
									this->useNormalizedBBox);
							if (overlap > overlapMax) {
								overlapMax = overlap;
								jmax = j;
							}
						}
						if (overlapMax >= this->overlapThreshold) {
							if (this->evaluateDifficultGt ||
									(!this->evaluateDifficultGt && !gtBBoxes[jmax].difficult)) {
								if (!visited[jmax]) {
									// true positive.
									outputData[numDet * 5 + 3] = 1;
									outputData[numDet * 5 + 4] = 0;
									visited[jmax] = true;
								} else {
									// false positive (multiple detection).
									outputData[numDet * 5 + 3] = 0;
									outputData[numDet * 5 + 4] = 1;
								}
							}
						} else {
							// false positive.
							outputData[numDet * 5 + 3] = 0;
							outputData[numDet * 5 + 4] = 1;
						}
						numDet++;
					}
				}
			}
		}
		if (this->sizes.size() > 0) {
			this->count++;
			if (this->count == this->sizes.size()) {
				// reset count after a full iterations through the DB.
				this->count = 0;
			}
		}
	}

#if DETECTIONEVALUATIONLAYER_LOG && false
	this->_printOn();
	this->_outputData[0]->print_data({}, false, -1);
	this->_printOff();
#endif
}

template <typename Dtype>
void DetectionEvaluateLayer<Dtype>::backpropagation() {

}

template <typename Dtype>
void DetectionEvaluateLayer<Dtype>::initialize(Builder* builder) {
	SASSERT(builder->_numClasses >= 0, "Must provide numClasses.");
	this->numClasses = builder->_numClasses;
	this->backgroundLabelId = builder->_backgroundLabelId;
	this->overlapThreshold = builder->_overlapThreshold;
	SASSERT(this->overlapThreshold > 0.f,
			"overlapThreshold must be non negative: %lf", this->overlapThreshold);
	this->evaluateDifficultGt = builder->_evaluateDifficultGt;
	if (!builder->_nameSizeFile.empty()) {
		string nameSizeFile = builder->_nameSizeFile;
		std::ifstream infile(nameSizeFile.c_str());
		SASSERT(infile.good(), "Failed to open name size file: %s", nameSizeFile.c_str());
		// The file is in the following format:
		//		name height width
		//		...
		string name;
		int height;
		int width;
		while (infile >> name >> height >> width) {
			this->sizes.push_back(std::make_pair(height, width));
		}
		infile.close();
	}
	this->count = 0;
	// If there is no nameSizeFile provided, use normalized bbox to evaluate.
	this->useNormalizedBBox = this->sizes.size() == 0;

	// Retrieve resize parameter if there is any provided.
}

template class DetectionEvaluateLayer<float>;
