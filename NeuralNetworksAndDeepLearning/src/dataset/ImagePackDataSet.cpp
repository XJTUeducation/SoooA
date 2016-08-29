/*
 * ImagePackDataSet.cpp
 *
 *  Created on: 2016. 7. 13.
 *      Author: jhkim
 */

#include "ImagePackDataSet.h"

#include <stddef.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "../exception/Exception.h"
#include "../util/UByteImage.h"
#include "../Util.h"


template <typename Dtype>
ImagePackDataSet<Dtype>::ImagePackDataSet(
		string trainImage,
		string trainLabel,
		uint32_t numTrainFile,
		string testImage,
		string testLabel,
		uint32_t numTestFile)
	: trainImage(trainImage),
	  trainLabel(trainLabel),
	  numTrainFile(numTrainFile),
	  testImage(testImage),
	  testLabel(testLabel),
	  numTestFile(numTestFile) {

	this->trainFileIndex = 0;
	this->testFileIndex = 0;
	this->bufDataSet = NULL;
}

template <typename Dtype>
ImagePackDataSet<Dtype>::~ImagePackDataSet() {}


template <typename Dtype>
const Dtype* ImagePackDataSet<Dtype>::getTrainDataAt(int index) {
	if(index >= this->numTrainData || index < 0) throw Exception();
	int reqPage = index / numImagesInTrainFile;
	if(reqPage != trainFileIndex) {
		load(DataSet<Dtype>::Train, reqPage);
		trainFileIndex = reqPage;
	}
	return &(*this->trainDataSet)[this->dataSize*(*this->trainSetIndices)[(index-reqPage*numImagesInTrainFile)]];
}

template <typename Dtype>
const uint32_t* ImagePackDataSet<Dtype>::getTrainLabelAt(int index) {
	if(index >= this->numTrainData || index < 0) throw Exception();
	int reqPage = index / numImagesInTrainFile;
	if(reqPage != trainFileIndex) {
		load(DataSet<Dtype>::Train, reqPage);
		trainFileIndex = reqPage;
	}
	return &(*this->trainLabelSet)[(*this->trainSetIndices)[index-reqPage*numImagesInTrainFile]];
}

template <typename Dtype>
const Dtype* ImagePackDataSet<Dtype>::getValidationDataAt(int index) {
	if(index >= this->numValidationData || index < 0) throw Exception();
	return &(*this->validationDataSet)[this->dataSize*index];
}

template <typename Dtype>
const uint32_t* ImagePackDataSet<Dtype>::getValidationLabelAt(int index) {
	if(index >= this->numValidationData || index < 0) throw Exception();
	return &(*this->validationLabelSet)[index];
}

template <typename Dtype>
const Dtype* ImagePackDataSet<Dtype>::getTestDataAt(int index) {
	if(index >= this->numTestData || index < 0) throw Exception();
	int reqPage = index / numImagesInTestFile;
	if(reqPage != testFileIndex) {
		load(DataSet<Dtype>::Test, reqPage);
		testFileIndex = reqPage;
	}
	return &(*this->testDataSet)[this->dataSize*(*this->testSetIndices)[(index-reqPage*numImagesInTestFile)]];
}

template <typename Dtype>
const uint32_t* ImagePackDataSet<Dtype>::getTestLabelAt(int index) {
	if(index >= this->numTestData || index < 0) throw Exception();
	int reqPage = index / numImagesInTestFile;
	if(reqPage != testFileIndex) {
		load(DataSet<Dtype>::Test, reqPage);
		testFileIndex = reqPage;
	}
	return &(*this->testLabelSet)[(*this->testSetIndices)[index-reqPage*numImagesInTestFile]];
}






template <typename Dtype>
void ImagePackDataSet<Dtype>::load() {

#ifndef GPU_MODE
	numTrainData = loadDataSetFromResource(filenames[0], trainDataSet, 0, 10000);
	numTestData = loadDataSetFromResource(filenames[1], testDataSet, 0, 0);
#else
	int numTrainDataInFile = load(DataSet<Dtype>::Train, 0);
	int numTestDataInFile = load(DataSet<Dtype>::Test, 0);

	if(numTrainDataInFile <= 0 || numTestDataInFile <= 0) {
		cout << "could not load resources ... " << endl;
		exit(1);
	}

	this->numTrainData = numTrainDataInFile*numTrainFile;
	this->numTestData = numTestDataInFile*numTestFile;
	this->numImagesInTrainFile = numTrainDataInFile;
	this->numImagesInTestFile = numTestDataInFile;
#endif
}

template <typename Dtype>
int ImagePackDataSet<Dtype>::load(typename DataSet<Dtype>::Type type, int page) {
	string pageSuffix = to_string(page);
	int numData = 0;
	// train
	switch(type) {
	case DataSet<Dtype>::Train:
		numData = loadDataSetFromResource(
				trainImage+pageSuffix,
				trainLabel+pageSuffix,
				this->trainDataSet,
				this->trainLabelSet,
				this->trainSetIndices);
		break;
	case DataSet<Dtype>::Test:
		numData = loadDataSetFromResource(
				testImage+pageSuffix,
				testLabel+pageSuffix,
				this->testDataSet,
				this->testLabelSet,
				this->testSetIndices);
		break;
	}
	return numData;
}

#ifndef GPU_MODE
template <typename Dtype>
int ImagePackDataSet<Dtype>::loadDataSetFromResource(string resources[2], DataSample* &dataSet, int offset, int size) {
	// LOAD IMAGE DATA
	ImageInfo dataInfo(resources[0]);
	dataInfo.load();

	// READ IMAGE DATA META DATA
	unsigned char* dataPtr = dataInfo.getBufferPtrAt(0);
	int dataMagicNumber = Util::pack4BytesToInt(dataPtr);
	if(dataMagicNumber != 0x00000803) return -1;
	dataPtr += 4;
	int dataSize = Util::pack4BytesToInt(dataPtr);
	dataPtr += 4;
	int dataNumRows = Util::pack4BytesToInt(dataPtr);
	dataPtr += 4;
	int dataNumCols = Util::pack4BytesToInt(dataPtr);
	dataPtr += 4;

	// LOAD LABEL DATA
	ImageInfo targetInfo(resources[1]);
	targetInfo.load();

	// READ LABEL DATA META DATA
	unsigned char* targetPtr = targetInfo.getBufferPtrAt(0);
	int targetMagicNumber = Util::pack4BytesToInt(targetPtr);
	if(targetMagicNumber != 0x00000801) return -1;
	targetPtr += 4;
	//int labelSize = Util::pack4BytesToInt(targetPtr);
	targetPtr += 4;

	if(offset >= dataSize) return 0;

	dataPtr += offset;
	targetPtr += offset;

	int stop = dataSize;
	if(size > 0) stop = min(dataSize, offset+size);

	//int dataArea = dataNumRows*  dataNumCols;
	if(dataSet) delete dataSet;
	dataSet = new DataSample[stop-offset];

	for(int i = offset; i < stop; i++) {
		//const DataSample* dataSample = new DataSample(dataPtr, dataArea, targetPtr, 10);
		//dataSet.push_back(dataSample);
		//dataSet[i-offset].readData(dataPtr, dataNumRows, dataNumCols, 1, targetPtr, 10);
		dataSet[i-offset].readData(dataPtr, dataNumRows, dataNumCols, 1, targetPtr, 10);
	}
	return stop-offset;
}

#else
template <typename Dtype>
int ImagePackDataSet<Dtype>::loadDataSetFromResource(
		string data_path,
		string label_path,
		vector<Dtype>*& dataSet,
		vector<uint32_t>*& labelSet,
		vector<uint32_t>*& setIndices) {

	FILE* imfp = fopen(data_path.c_str(), "rb");
	if(!imfp) {
		cout << "ERROR: Cannot open image dataset " << data_path << endl;
		return 0;
	}
	FILE* lbfp = fopen(label_path.c_str(), "rb");
	if(!lbfp) {
		fclose(imfp);
		cout << "ERROR: Cannot open label dataset " << label_path << endl;
		return 0;
	}

	UByteImageDataset image_header;
	UByteLabelDataset label_header;

	// Read and verify file headers
	if(fread(&image_header, sizeof(UByteImageDataset), 1, imfp) != 1) {
		cout << "ERROR: Invalid dataset file (image file header)" << endl;
		fclose(imfp);
		fclose(lbfp);
		return 0;
	}
	if(fread(&label_header, sizeof(UByteLabelDataset), 1, lbfp) != 1) {
		cout << "ERROR: Invalid dataset file (label file header)" << endl;
		fclose(imfp);
		fclose(lbfp);
		return 0;
	}

	// Byte-swap data structure values (change endianness)
	image_header.Swap();
	label_header.Swap();

	// Verify datasets
	if(image_header.magic != UBYTE_IMAGE_MAGIC) {
		printf("ERROR: Invalid dataset file (image file magic number)\n");
		fclose(imfp);
		fclose(lbfp);
		return 0;
	}
	if (label_header.magic != UBYTE_LABEL_MAGIC) 	{
		printf("ERROR: Invalid dataset file (label file magic number)\n");
		fclose(imfp);
		fclose(lbfp);
		return 0;
	}
	if (image_header.length != label_header.length) {
		printf("ERROR: Dataset file mismatch (number of images does not match the number of labels)\n");
		fclose(imfp);
		fclose(lbfp);
		return 0;
	}

	// Output dimensions
	size_t width = image_header.width;
	size_t height = image_header.height;
	size_t channel = image_header.channel;
	this->cols = width;
	this->rows = height;
	this->channels = channel;
	this->dataSize = this->rows*this->cols*this->channels;

	// Read images and labels (if requested)
	size_t dataSetSize = ((size_t)image_header.length)*this->dataSize;
	if(!dataSet) dataSet = new vector<Dtype>(dataSetSize);
	if(!bufDataSet) bufDataSet = new vector<uint8_t>(dataSetSize);
	if(!labelSet) labelSet = new vector<uint32_t>(label_header.length);
	if(!setIndices) setIndices = new vector<uint32_t>(label_header.length);
	std::iota(setIndices->begin(), setIndices->end(), 0);

	if(fread(&(*bufDataSet)[0], sizeof(uint8_t), dataSetSize, imfp) != dataSetSize) {
		printf("ERROR: Invalid dataset file (partial image dataset)\n");
		fclose(imfp);
		fclose(lbfp);
		return 0;
	}

	for(size_t i = 0; i < dataSetSize; i++) {
		(*dataSet)[i] = (*bufDataSet)[i]/255.0f;
	}

	if (fread(&(*labelSet)[0], sizeof(uint32_t), label_header.length, lbfp) != label_header.length) {
		printf("ERROR: Invalid dataset file (partial label dataset)\n");
		fclose(imfp);
		fclose(lbfp);
		return 0;
	}

	fclose(imfp);
	fclose(lbfp);

	return image_header.length;
}
#endif


template class ImagePackDataSet<float>;
