#ifndef NEURONGEOGRAPHER_H
#define NEURONGEOGRAPHER_H

#include <cmath>

#include <qlist.h>

#include "integratedDataTypes.h"
#include "ImgAnalyzer.h"

using namespace std;
using namespace integratedDataTypes;

class NeuronGeoGrapher
{
public:
	/***************** Constructors and Basic Data/Function Members *****************/
	// Not needed at the moment. Will implement later if necessary.
	/********************************************************************************/



	/*************************** Vector Geometry ***************************/
	template<class T> // Get the vecotr formed by 2 NeuronSWC nodes.
	static inline vector<T> getVector_NeuronSWC(const NeuronSWC& startNode, const NeuronSWC& endNode);

	template<class T> // Get the unit displacement vector between 2 vectors ==> Needs to redefine starting and ending locations.
	static inline vector<T> getDispUnitVector(const vector<T>& endingLoc, const vector<T>& startingLoc);

	// Get the vector between 2 NeuronSWC nodes with the vector's starting location (startNode). 
	// The starting location in each dimension and its components are stored in pairs separately. 
	template<class T> 
	static inline vector<pair<T, T>> getVectorWithStartingLoc_btwn2nodes(const NeuronSWC& startNode, const NeuronSWC& endNode);

	template<class T> // Get cosine between 2 given vecotrs.
	static inline T getVectorCosine(const vector<T>& vector1, const vector<T>& vector2);
	
	template<class T> // Get sine between 2 given vectors.
	static inline T getVectorSine(const vector<T>& vector1, const vector<T>& vector2);

	template<class T> // Get the included angle of 2 given vectors in pi.
	static inline T getPiAngle(const vector<T>& vector1, const vector<T>& vector2);

	template<class T> // Get the included angle of 2 given vectors in radian.
	static inline T getRadAngle(const vector<T>& vector1, const vector<T>& vector2);

	// Find out the projection vector onto the given axial vector from the projecting vector.
	static vector<pair<float, float>> getProjectionVector(const vector<pair<float, float>>& axialVector, const vector<pair<float, float>>& projectingVector);
	/***********************************************************************/



	/*************************** Segment Geometry **************************/
	template<class T> // Calculate the sum of turning angles from the segment head to tail.
	static inline T selfTurningRadAngleSum(const vector<vector<T>>& inputSegment);

	// With specified pointing directions for each segments, returns the included angle between the 2 segments. 
	static double segPointingCompare(const segUnit& elongSeg, const segUnit& connSeg, connectOrientation connOrt);

	// This method calculates the "turning angle" from elongating segment to connected segment. 
	// The turning angle is defined as the angle formed by displacement vector of elongating segment and the displacement vector from elongating point to connecting point.
	static double segTurningAngle(const segUnit& elongSeg, const segUnit& connSeg, connectOrientation connOrt);

	static segUnit segmentStraighten(const segUnit& inputSeg);
	/***********************************************************************/



	/****************** Polar Coordinate System Operations *****************/
	template<class T> // Converts NeuronSWC to polarNeuronSWC with specified origin.
	static inline polarNeuronSWC CartesianNode2Polar(const NeuronSWC& inputNode, vector<T> origin = { 0, 0, 0 });

	// Converts polarNeuronSWC to NeuronSWC.
	static inline NeuronSWC polar2CartesianNode(const polarNeuronSWC& inputPolarNode);

	template<class T> // Converts input NeuronSWC list to polarNeuronSWC list with specified origin.
	static inline void nodeList2polarNodeList(const QList<NeuronSWC>& inputNodeList, vector<polarNeuronSWC>& outputPolarNodeList, vector<T> origin = { 0, 0, 0 });

	// 
	static inline void polarNodeList2nodeList(const vector<polarNeuronSWC>& inputPolarNodeList, QList<NeuronSWC>& outputNodeList);

	static inline boost::container::flat_map<int, int> polarNodeID2locMap(const vector<polarNeuronSWC>& inputPolarNodeList);

	static boost::container::flat_map<double, boost::container::flat_set<int>> getShellByRadius_ID(const vector<polarNeuronSWC>& inputPolarNodeList);

	static boost::container::flat_map<double, boost::container::flat_set<int>> getShellByRadius_loc(const vector<polarNeuronSWC>& inputPolarNodeList, double thickness = 1);
	/***********************************************************************/


	/*********** SWC - ImgAnalyzer::ConnectedComponent Analysis ************/
	static inline void findChebyshevCenter_compList(vector<connectedComponent>& inputCompList) { ImgAnalyzer::ChebyshevCenter_connCompList(inputCompList); }
	/***********************************************************************/
};

template<class T>
inline vector<T> NeuronGeoGrapher::getVector_NeuronSWC(const NeuronSWC& startNode, const NeuronSWC& endNode)
{
	vector<T> vec(3);
	vec[0] = endNode.x - startNode.x;
	vec[1] = endNode.y - startNode.y;
	vec[2] = endNode.z - startNode.z;
	return vec;
}

template<class T>
inline vector<T> NeuronGeoGrapher::getDispUnitVector(const vector<T>& endingLoc, const vector<T>& startingLoc)
{
	T disp = sqrt((endingLoc.at(0) - startingLoc.at(0)) * (endingLoc.at(0) - startingLoc.at(0)) +
				  (endingLoc.at(1) - startingLoc.at(1)) * (endingLoc.at(1) - startingLoc.at(1)) +
				  (endingLoc.at(2) - startingLoc.at(2)) * (endingLoc.at(2) - startingLoc.at(2)));
	vector<T> dispUnitVector;
	dispUnitVector.push_back((endingLoc.at(0) - startingLoc.at(0)) / disp);
	dispUnitVector.push_back((endingLoc.at(1) - startingLoc.at(1)) / disp);
	dispUnitVector.push_back((endingLoc.at(2) - startingLoc.at(2)) / disp);

	return dispUnitVector;
}

template<class T>
inline vector<pair<T, T>> NeuronGeoGrapher::getVectorWithStartingLoc_btwn2nodes(const NeuronSWC& startNode, const NeuronSWC& endNode)
{
	vector<T> thisVector = NeuronGeoGrapher::getVector_NeuronSWC<T>(startNode, endNode);

	vector<pair<T, T>> outputVectorPairs(3);
	outputVectorPairs[0] = pair<T, T>(startNode.x, thisVector.at(0));
	outputVectorPairs[1] = pair<T, T>(startNode.y, thisVector.at(1));
	outputVectorPairs[2] = pair<T, T>(startNode.z, thisVector.at(2));

	return outputVectorPairs;
}

template<class T>
inline T NeuronGeoGrapher::getVectorCosine(const vector<T>& vector1, const vector<T>& vector2)
{
	T dot = (vector1.at(0) * vector2.at(0) + vector1.at(1) * vector2.at(1) + vector1.at(2) * vector2.at(2));
	T sq1 = (vector1.at(0) * vector1.at(0) + vector1.at(1) * vector1.at(1) + vector1.at(2) * vector1.at(2));
	T sq2 = (vector2.at(0) * vector2.at(0) + vector2.at(1) * vector2.at(1) + vector2.at(2) * vector2.at(2));

	return dot / sqrt(sq1 * sq2);
}

template<class T>
inline T NeuronGeoGrapher::getVectorSine(const vector<T>& vector1, const vector<T>& vector2)
{
	T thisVectorCos = NeuronGeoGrapher::getVectorCosine<T>(vector1, vector2);
	T thisVectorSin = sqrt(1 - thisVectorCos * thisVectorCos);

	return thisVectorSin;
}

template<class T>
inline T NeuronGeoGrapher::getPiAngle(const vector<T>& vector1, const vector<T>& vector2)
{
	T dot = (vector1.at(0) * vector2.at(0) + vector1.at(1) * vector2.at(1) + vector1.at(2) * vector2.at(2));
	T sq1 = (vector1.at(0) * vector1.at(0) + vector1.at(1) * vector1.at(1) + vector1.at(2) * vector1.at(2));
	T sq2 = (vector2.at(0) * vector2.at(0) + vector2.at(1) * vector2.at(1) + vector2.at(2) * vector2.at(2));
	T angle = acos(dot / sqrt(sq1 * sq2));

	if (std::isnan(acos(dot / sqrt(sq1 * sq2)))) return -10;
	else return angle / PI;
}

template<class T>
inline T NeuronGeoGrapher::getRadAngle(const vector<T>& vector1, const vector<T>& vector2)
{
	T dot = (vector1.at(0) * vector2.at(0) + vector1.at(1) * vector2.at(1) + vector1.at(2) * vector2.at(2));
	T sq1 = (vector1.at(0) * vector1.at(0) + vector1.at(1) * vector1.at(1) + vector1.at(2) * vector1.at(2));
	T sq2 = (vector2.at(0) * vector2.at(0) + vector2.at(1) * vector2.at(1) + vector2.at(2) * vector2.at(2));
	T angle = acos(dot / sqrt(sq1 * sq2));

	if (std::isnan(acos(dot / sqrt(sq1 * sq2)))) return -10;
	else return angle;
}

template<class T>
inline T NeuronGeoGrapher::selfTurningRadAngleSum(const vector<vector<T>>& inputSegment)
{
	T radAngleSum = 0;
	for (vector<vector<T>>::const_iterator it = inputSegment.begin() + 1; it != inputSegment.end() - 1; ++it)
	{
		vector<T> vector1(3), vector2(3);
		vector1[0] = it->at(0) - (it - 1)->at(0);
		vector1[1] = it->at(1) - (it - 1)->at(1);
		vector1[2] = it->at(2) - (it - 1)->at(2);
		vector2[0] = (it + 1)->at(0) - it->at(0);
		vector2[1] = (it + 1)->at(1) - it->at(1);
		vector2[2] = (it + 1)->at(2) - it->at(2);
		//cout << "(" << vector1[0] << ", " << vector1[1] << ", " << vector1[2] << ") (" << vector2[0] << ", " << vector2[1] << ", " << vector2[2] << ")" << endl;
		T radAngle = NeuronGeoGrapher::getRadAngle(vector1, vector2);

		if (radAngle == -1) continue;
		else radAngleSum = radAngleSum + radAngle;
	}

	return radAngleSum;
}

template<class T>
inline polarNeuronSWC NeuronGeoGrapher::CartesianNode2Polar(const NeuronSWC& inputNode, vector<T> origin)
{
	polarNeuronSWC newPolarNode;
	newPolarNode.ID = inputNode.n;
	newPolarNode.type = inputNode.type;
	newPolarNode.parent = inputNode.parent;
	newPolarNode.CartesianX = inputNode.x;
	newPolarNode.CartesianY = inputNode.y;
	newPolarNode.CartesianZ = inputNode.z;
	newPolarNode.polarOriginX = origin.at(0);
	newPolarNode.polarOriginY = origin.at(1);
	newPolarNode.polarOriginZ = origin.at(2);
	
	vector<double> nodeVec(3);
	nodeVec[0] = inputNode.x - origin.at(0);
	nodeVec[1] = inputNode.y - origin.at(1);
	nodeVec[2] = inputNode.z - origin.at(2);

	vector<double> projectedVec(3);	
	projectedVec[0] = inputNode.x;
	projectedVec[1] = inputNode.y;
	projectedVec[2] = origin.at(2);

	vector<double> horizontal_refVec(3);
	horizontal_refVec[0] = origin.at(0) + 10;
	horizontal_refVec[1] = origin.at(1);
	horizontal_refVec[2] = origin.at(2);

	newPolarNode.radius = sqrt((double(inputNode.x) - origin.at(0)) * (double(inputNode.x) - origin.at(0)) + (double(inputNode.y) - origin.at(1)) * (double(inputNode.y) - origin.at(1)) + (double(inputNode.z) - origin.at(2)) * (double(inputNode.z) - origin.at(2)));
	newPolarNode.theta = NeuronGeoGrapher::getRadAngle(projectedVec, horizontal_refVec);
	newPolarNode.phi = NeuronGeoGrapher::getRadAngle(nodeVec, projectedVec);

	return newPolarNode;
}

inline NeuronSWC NeuronGeoGrapher::polar2CartesianNode(const polarNeuronSWC& inputPolarNode)
{
	NeuronSWC newNode;
	newNode.n = inputPolarNode.ID;
	newNode.type = inputPolarNode.type;
	newNode.parent = inputPolarNode.parent;
	newNode.x = inputPolarNode.CartesianX;
	newNode.y = inputPolarNode.CartesianY;
	newNode.z = inputPolarNode.CartesianZ;

	return newNode;
}

inline void NeuronGeoGrapher::polarNodeList2nodeList(const vector<polarNeuronSWC>& inputPolarNodeList, QList<NeuronSWC>& outputNodeList)
{
	outputNodeList.clear();
	for (vector<polarNeuronSWC>::const_iterator polarNodeIt = inputPolarNodeList.begin(); polarNodeIt != inputPolarNodeList.end(); ++polarNodeIt)
		outputNodeList.push_back(NeuronGeoGrapher::polar2CartesianNode(*polarNodeIt));
}

template<class T>
inline void NeuronGeoGrapher::nodeList2polarNodeList(const QList<NeuronSWC>& inputNodeList, vector<polarNeuronSWC>& outputPolarNodeList, vector<T> origin)
{
	outputPolarNodeList.clear();
	for (QList<NeuronSWC>::const_iterator nodeIt = inputNodeList.begin(); nodeIt != inputNodeList.end(); ++nodeIt)
		outputPolarNodeList.push_back(NeuronGeoGrapher::CartesianNode2Polar(*nodeIt, origin));
}

inline boost::container::flat_map<int, int> NeuronGeoGrapher::polarNodeID2locMap(const vector<polarNeuronSWC>& inputPolarNodeList)
{
	boost::container::flat_map<int, int> outputMap;
	for (vector<polarNeuronSWC>::const_iterator it = inputPolarNodeList.begin(); it != inputPolarNodeList.end(); ++it)
		outputMap.insert(pair<int, int>(it->ID, int(it - inputPolarNodeList.begin())));

	return outputMap;
}

#endif