/*
 * ProposalTargetLayer.h
 *
 *  Created on: Nov 30, 2016
 *      Author: jkim
 */

#ifndef PROPOSALTARGETLAYER_H_
#define PROPOSALTARGETLAYER_H_

#include <vector>

#include "common.h"
#include "Layer.h"


/**
 * Assign object detection proposals to ground-truth targets. Produces proposal
 * classification labels and bounding-box regression targets.
 */
template <typename Dtype>
class ProposalTargetLayer : public Layer<Dtype> {
public:
	class Builder : public Layer<Dtype>::Builder {
	public:
		uint32_t _numClasses;

		Builder() {
			this->type = Layer<Dtype>::ProposalTarget;
			this->_numClasses = 0;
		}
		virtual Builder* name(const std::string name) {
			Layer<Dtype>::Builder::name(name);
			return this;
		}
		virtual Builder* id(uint32_t id) {
			Layer<Dtype>::Builder::id(id);
			return this;
		}
		virtual Builder* inputs(const std::vector<std::string>& inputs) {
			Layer<Dtype>::Builder::inputs(inputs);
			return this;
		}
		virtual Builder* outputs(const std::vector<std::string>& outputs) {
			Layer<Dtype>::Builder::outputs(outputs);
			return this;
		}
		virtual Builder* propDown(const std::vector<bool>& propDown) {
			Layer<Dtype>::Builder::propDown(propDown);
			return this;
		}
		Builder* numClasses(const uint32_t numClasses) {
			this->_numClasses = numClasses;
			return this;
		}
		Layer<Dtype>* build() {
			return new ProposalTargetLayer(this);
		}
	};

	ProposalTargetLayer(Builder* builder);
	virtual ~ProposalTargetLayer();

	virtual void reshape();
	virtual void feedforward();
	virtual void backpropagation();

private:
	void initialize();
	void _sampleRois(
			const std::vector<std::vector<float>>& allRois,
			const std::vector<std::vector<float>>& gtBoxes,
			const uint32_t fgRoisPerImage,
			const uint32_t roisPerImage,
			std::vector<uint32_t>& labels,
			std::vector<std::vector<float>>& rois,
			std::vector<std::vector<float>>& bboxTargets,
			std::vector<std::vector<float>>& bboxInsideWeights);
	void _computeTargets(
			const std::vector<std::vector<float>>& exRois,
			const uint32_t exRoisOffset,
			const std::vector<std::vector<float>>& gtRois,
			const uint32_t gtRoisOffset,
			const std::vector<uint32_t>& labels,
			std::vector<std::vector<float>>& targets,
			const uint32_t targetOffset);
	void _getBboxRegressionLabels(
			const std::vector<std::vector<float>>& bboxTargetData,
			std::vector<std::vector<float>>& bboxTargets,
			std::vector<std::vector<float>>& bboxInsideWeights);

private:
	uint32_t numClasses;

};

#endif /* PROPOSALTARGETLAYER_H_ */
