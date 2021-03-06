#include <fstream>

#include "ssd_common.h"


using namespace std;


template <typename Dtype>
LabelMap<Dtype>::LabelMap(const string& labelMapPath)
: LabelMap() {
	this->labelMapPath = labelMapPath;
}

template <typename Dtype>
LabelMap<Dtype>::LabelMap() {
	// background
	this->colorList.push_back(cv::Scalar(0, 0, 0));

	this->colorList.push_back(cv::Scalar(10, 163, 240));
	this->colorList.push_back(cv::Scalar(44, 90, 130));
	this->colorList.push_back(cv::Scalar(239, 80, 0));
	this->colorList.push_back(cv::Scalar(37, 0, 162));
	this->colorList.push_back(cv::Scalar(226, 161, 27));

	this->colorList.push_back(cv::Scalar(115, 0, 216));
	this->colorList.push_back(cv::Scalar(0, 196, 164));
	this->colorList.push_back(cv::Scalar(255, 0, 106));
	this->colorList.push_back(cv::Scalar(23, 169, 96));
	this->colorList.push_back(cv::Scalar(0, 138, 0));

	this->colorList.push_back(cv::Scalar(138, 96, 118));
	this->colorList.push_back(cv::Scalar(100, 135, 109));
	this->colorList.push_back(cv::Scalar(0, 104, 250));
	this->colorList.push_back(cv::Scalar(208, 114, 244));
	this->colorList.push_back(cv::Scalar(0, 20, 229));

	this->colorList.push_back(cv::Scalar(63, 59, 122));
	this->colorList.push_back(cv::Scalar(135, 118, 100));
	this->colorList.push_back(cv::Scalar(169, 171, 0));
	this->colorList.push_back(cv::Scalar(255, 0, 170));
	this->colorList.push_back(cv::Scalar(0, 193, 216));
}



template <typename Dtype>
void LabelMap<Dtype>::setLabelMapPath(const string& labelMapPath) {
	this->labelMapPath = labelMapPath;
}

template <typename Dtype>
void LabelMap<Dtype>::build() {
	ifstream ifs(this->labelMapPath.c_str(), ios::in);
	if (!ifs.is_open()) {
		cout << "no such file: " << this->labelMapPath << endl;
		exit(1);
	}

	string part1;
	string part2;
	LabelItem labelItem;
	bool started = false;
	int line = 1;
	while (ifs >> part1 >> part2) {
		//cout << "part1: '" << part1 << "', part2: '" << part2 << "'" << endl;
		if (part1 == "item" && part2 == "{") {
			started = true;
		}
		else if (started && part1 == "name:") {
			part2 = part2.substr(1, part2.length()-2);
			labelItem.name = part2;
		}
		else if (started && part1 == "label:") {
			labelItem.label = atoi(part2.c_str());
		}
		else if (started && part1 == "display_name:") {
			part2 = part2.substr(1, part2.length()-2);
			labelItem.displayName = part2;

			ifs >> part1;
			if (part1 == "}") {
				this->labelItemList.push_back(labelItem);
				started = false;
			}
		}
		else {
			cout << "invalid label map format at line: " << line << endl;
			exit(1);
		}
		line++;
	}

	for (int i = 0; i < this->labelItemList.size(); i++) {
		LabelItem& labelItem = this->labelItemList[i];
		this->labelToIndMap[labelItem.displayName] = labelItem.label;
		this->indToLabelMap[labelItem.label] = labelItem.displayName;
	}
}

template <typename Dtype>
int LabelMap<Dtype>::convertLabelToInd(const string& label) {
	if (this->labelToIndMap.find(label) == this->labelToIndMap.end()) {
		cout << "invalid label: " << label << endl;
		exit(1);
	}
	return this->labelToIndMap[label];
}

template <typename Dtype>
string LabelMap<Dtype>::convertIndToLabel(int ind) {
	if (this->indToLabelMap.find(ind) == this->indToLabelMap.end()) {
		cout << "invalid ind: " << ind << endl;
		exit(1);
	}
	return this->indToLabelMap[ind];
}

template <typename Dtype>
void LabelMap<Dtype>::mapLabelToName(map<int, string>& labelToName) {
	labelToName = this->indToLabelMap;
}

template <typename Dtype>
int LabelMap<Dtype>::getCount() {
	return this->labelItemList.size();
}

template <typename Dtype>
void LabelMap<Dtype>::printLabelItemList() {
	for (int i = 0; i < this->labelItemList.size(); i++) {
		cout << "label item #" << i << endl;
		this->labelItemList[i].print();
	}
}

template <typename Dtype>
void LabelMap<Dtype>::LabelItem::print() {
	cout << "LabelItem: " 		<< this->name 			<< endl;
	cout << "\tlabel: " 		<< this->label 			<< endl;
	cout << "\tdisplay_name: " 	<< this->displayName 	<< endl;
}

template <typename Dtype>
void BoundingBox<Dtype>::print() {
	cout << "BoundingBox: " << this->name	<< endl;
	cout << "\tlabel: " 	<< this->label	<< endl;
	cout << "\txmin: " 		<< this->xmin	<< endl;
	cout << "\tymin: " 		<< this->ymin	<< endl;
	cout << "\txmax: " 		<< this->xmax	<< endl;
	cout << "\tymax: " 		<< this->ymax	<< endl;
	cout << "\tdiff: " 		<< this->diff	<< endl;
}


template <typename Dtype>
void ODRawData<Dtype>::print() {
	cout << "ODRawData" 						<< endl;
	cout << "\timPath: " 	<< this->imPath 	<< endl;
	cout << "\tannoPath: "	<< this->annoPath	<< endl;
	cout << "\twidth: " 	<< this->width 		<< endl;
	cout << "\theight: " 	<< this->height 	<< endl;
	cout << "\tdepth: " 	<< this->depth 		<< endl;
	cout << "\tboundingBoxes: " 				<< endl;

	for (int i = 0; i < this->boundingBoxes.size(); i++) {
		this->boundingBoxes[i].print();
	}
}

template <typename Dtype>
void ODRawData<Dtype>::displayBoundingBoxes(const string& baseDataPath,
		vector<cv::Scalar>& colorList) {
	cv::Mat im = cv::imread(baseDataPath + this->imPath);

	for (int i = 0; i < this->boundingBoxes.size(); i++) {
		BoundingBox<Dtype>& bb = this->boundingBoxes[i];

		cv::rectangle(im, cv::Point(bb.xmin, bb.ymin), cv::Point(bb.xmax, bb.ymax),
				colorList[bb.label], 2);
		cv::putText(im, bb.name , cv::Point(bb.xmin, bb.ymin+15.0f), 2, 0.5f,
				colorList[bb.label]);
	}

	const string windowName = this->imPath;
	cv::namedWindow(windowName, CV_WINDOW_AUTOSIZE);
	cv::imshow(windowName, im);

	cv::waitKey(0);
	cv::destroyAllWindows();
}



template class LabelMap<float>;


