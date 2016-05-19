#include <iostream>
#include <armadillo>

#include "Network.h"
#include "dataset/MnistDataSet.h"
#include "dataset/MockDataSet.h"
#include "activation.h"
#include "Util.h"
#include "pooling/Pooling.h"
#include "pooling/MaxPooling.h"
#include "cost/CrossEntropyCost.h"
#include "cost/LogLikelihoodCost.h"
#include "monitor/NetworkMonitor.h"

#include "layer/Layer.h"
#include "layer/InputLayer.h"
#include "layer/FullyConnectedLayer.h"
#include "layer/ConvPoolLayer.h"
#include "layer/SigmoidLayer.h"
#include "layer/SoftmaxLayer.h"
#include "activation/Activation.h"
#include "activation/Sigmoid.h"
#include "activation/ReLU.h"

using namespace std;
using namespace arma;

// Armadillo documentation is available at:
// http://arma.sourceforge.net/docs.html


void armadillo_test();
void network_test();
void network_test2();
void custom_armadillo_test();


int main(int argc, char** argv) {

	cout << "main" << endl;

	//armadillo_test();
	network_test2();

	//custom_armadillo_test();
	return 0;
}


void custom_armadillo_test() {
	vec *pY = new vec(2), *pA = new vec(2);
	(*pY) << 1 << 0;
	(*pA) << 0.0872 << 0.2008;


	vec temp1 = (-1 * (*pY));
	vec temp2 = log(*pA);
	vec temp3 = temp1 % temp2;
	Util::printVec(temp1, "temp1");
	Util::printVec(temp2, "temp2");
	Util::printVec(temp3, "temp3");


	vec left = (-1 * (*pY)) % log(*pA);
	vec right = (1 - (*pY)) % log(1 - (*pA));
	vec result = left - right;
	Util::printVec(left, "left");
	Util::printVec(right, "right");
	Util::printVec(result, "result");

}



void network_test() {

	/*
	bool debug = false;
	double validationSetRatio = 1.0/6.0;

	CrossEntropyCost cost;
	NetworkListener *networkListener = new NetworkMonitor();

	double lambda = 5.0;

	if(!debug) {
		Util::setPrint(false);
		MnistDataSet dataSet(validationSetRatio);
		dataSet.load();
		int sizes[] = {784, 30, 10};
		Network network(sizes, 3, &dataSet, &cost, networkListener);
		network.sgd(30, 10, 0.1, lambda);
	} else {
		Util::setPrint(true);
		MockDataSet dataSet;
		dataSet.load();
		int sizes[] = {9, 5, 10};
		Network network(sizes, 3, &dataSet, &cost, networkListener);
		network.sgd(5, 2, 3.0, lambda);
	}

	delete networkListener;
	*/
}


void network_test2() {
	bool debug = false;
	double validationSetRatio = 1.0/6.0;

	Activation *sigmoid = new Sigmoid();

	//Cost *crossEntropyCost = new CrossEntropyCost();
	Pooling *maxPooling = new MaxPooling();
	NetworkListener *networkListener = new NetworkMonitor();

	double lambda = 5.0;

	if(!debug) {
		Util::setPrint(false);

		//Activation *relu1 = new ReLU(io_dim(24, 24, 20));
		//Activation *relu2 = new ReLU(io_dim(100, 1, 1));

		// DataSet은 memory를 크게 차지할 수 있으므로 heap에 생성
		DataSet *mnistDataSet = new MnistDataSet(validationSetRatio);
		mnistDataSet->load();
		//int sizes[] = {784, 30, 10};

		Layer *layers[] = {
			new InputLayer(io_dim(28, 28, 1)),
			new ConvPoolLayer(io_dim(28, 28, 1), filter_dim(5, 5, 1, 20), pool_dim(2, 2), sigmoid, maxPooling),
			new ConvPoolLayer(io_dim(12, 12, 20), filter_dim(5, 5, 20, 40), pool_dim(2, 2), sigmoid, maxPooling),
			new FullyConnectedLayer(4*4*40, 100, 0.0, sigmoid),
			//new FullyConnectedLayer(12*12*20, 100, 0.0, sigmoid),
			new SoftmaxLayer(100, 10, 0.0)
			//new SigmoidLayer(30, 10, crossEntropyCost)
			//new FullyConnectedLayer(30, 10, sigmoid)
		};

		int numLayers = sizeof(layers)/sizeof(layers[0]);
		Network network(layers, numLayers, mnistDataSet, networkListener);
		//network.sgd(30, 10, 0.1, lambda);
		network.sgd(500, 10, 0.1, lambda);

	} else {
		Util::setPrint(true);

		//Activation *relu1 = new ReLU(io_dim(8, 8, 5));
		//Activation *relu2 = new ReLU(io_dim(10, 1, 1));

		MockDataSet *dataSet = new MockDataSet();
		dataSet->load();

		//int sizes[] = {9, 5, 10};
		Layer *layers[] = {
			/*
			new InputLayer(io_dim(10, 10, 1)),
			new ConvPoolLayer(io_dim(10, 10, 1), filter_dim(3, 3, 1, 1), pool_dim(2, 2), relu1, maxPooling),
			new FullyConnectedLayer(16, 10, 0.5, relu2),
			new SoftmaxLayer(10, 10, 0.5)
			*/
			new InputLayer(io_dim(12, 12, 3)),
			new ConvPoolLayer(io_dim(12, 12, 3), filter_dim(3, 3, 3, 2), pool_dim(2, 2), sigmoid, maxPooling),
			new ConvPoolLayer(io_dim(5, 5, 2), filter_dim(2, 2, 2, 3), pool_dim(2, 2), sigmoid, maxPooling),
			new FullyConnectedLayer(2*2*3, 10, 0.0, sigmoid),
			new SoftmaxLayer(10, 10, 0.0)
		};

		int numLayers = sizeof(layers)/sizeof(layers[0]);
		Network network(layers, numLayers, dataSet, networkListener);
		network.sgd(5, 2, 3.0, lambda);
	}
	delete networkListener;
}








void armadillo_test()
{
	cout << "Armadillo version: " << arma_version::as_string() << endl;

	mat A(2, 3); // directly specify the matrix size (elements are uninitialised)

	cout << "A.n_rows: " << A.n_rows << endl; // .n_rows and .n_cols are read only
	cout << "A.n_cols: " << A.n_cols << endl;

	A(1, 2) = 456.0;  // directly access an element (indexing starts at 0)
	A.print("A:");

	A = 5.0;         // scalars are treated as a 1x1 matrix
	A.print("A:");

	A.set_size(4, 5); // change the size (data is not preserved)

	A.fill(5.0);     // set all elements to a particular value
	A.print("A:");

	// endr indicates "end of row"
	A << 0.165300 << 0.454037 << 0.995795 << 0.124098 << 0.047084 << endr
			<< 0.688782 << 0.036549 << 0.552848 << 0.937664 << 0.866401 << endr
			<< 0.348740 << 0.479388 << 0.506228 << 0.145673 << 0.491547 << endr
			<< 0.148678 << 0.682258 << 0.571154 << 0.874724 << 0.444632 << endr
			<< 0.245726 << 0.595218 << 0.409327 << 0.367827 << 0.385736 << endr;

	A.print("A:");

	// determinant
	// cout << "det(A): " << det(A) << endl;

	// inverse
	// cout << "inv(A): " << endl << inv(A) << endl;

	// save matrix as a text file
	A.save("A.txt", raw_ascii);

	// load from file
	mat B;
	B.load("A.txt");

	// submatrices
	cout << "B( span(0,2), span(3,4) ):" << endl << B(span(0, 2), span(3, 4))
			<< endl;

	cout << "B( 0,3, size(3,2) ):" << endl << B(0, 3, size(3, 2)) << endl;

	cout << "B.row(0): " << endl << B.row(0) << endl;

	cout << "B.col(1): " << endl << B.col(1) << endl;

	// transpose
	cout << "B.t(): " << endl << B.t() << endl;

	// maximum from each column (traverse along rows)
	cout << "max(B): " << endl << max(B) << endl;

	// maximum from each row (traverse along columns)
	cout << "max(B,1): " << endl << max(B, 1) << endl;

	// maximum value in B
	cout << "max(max(B)) = " << max(max(B)) << endl;

	// sum of each column (traverse along rows)
	cout << "sum(B): " << endl << sum(B) << endl;

	// sum of each row (traverse along columns)
	cout << "sum(B,1) =" << endl << sum(B, 1) << endl;

	// sum of all elements
	cout << "accu(B): " << accu(B) << endl;

	// trace = sum along diagonal
	cout << "trace(B): " << trace(B) << endl;

	// generate the identity matrix
	mat C = eye < mat > (4, 4);

	// random matrix with values uniformly distributed in the [0,1] interval
	mat D = randu < mat > (4, 4);
	D.print("D:");

	// row vectors are treated like a matrix with one row
	rowvec r;
	r << 0.59119 << 0.77321 << 0.60275 << 0.35887 << 0.51683;
	r.print("r:");

	// column vectors are treated like a matrix with one column
	vec q;
	q << 0.14333 << 0.59478 << 0.14481 << 0.58558 << 0.60809;
	q.print("q:");

	// convert matrix to vector; data in matrices is stored column-by-column
	vec v = vectorise(A);
	v.print("v:");

	// dot or inner product
	cout << "as_scalar(r*q): " << as_scalar(r * q) << endl;

	// outer product
	cout << "q*r: " << endl << q * r << endl;

	// multiply-and-accumulate operation (no temporary matrices are created)
	cout << "accu(A % B) = " << accu(A % B) << endl;

	// example of a compound operation
	B += 2.0 * A.t();
	B.print("B:");

	// imat specifies an integer matrix
	imat AA;
	imat BB;

	AA << 1 << 2 << 3 << endr << 4 << 5 << 6 << endr << 7 << 8 << 9;
	BB << 3 << 2 << 1 << endr << 6 << 5 << 4 << endr << 9 << 8 << 7;

	// comparison of matrices (element-wise); output of a relational operator is a umat
	umat ZZ = (AA >= BB);
	ZZ.print("ZZ:");

	// cubes ("3D matrices")
	cube Q(B.n_rows, B.n_cols, 2);

	Q.slice(0) = B;
	Q.slice(1) = 2.0 * B;

	Q.print("Q:");

	// 2D field of matrices; 3D fields are also supported
	field < mat > F(4, 3);

	for (uword col = 0; col < F.n_cols; ++col)
		for (uword row = 0; row < F.n_rows; ++row) {
			F(row, col) = randu < mat > (2, 3); // each element in field<mat> is a matrix
		}

	F.print("F:");

}

