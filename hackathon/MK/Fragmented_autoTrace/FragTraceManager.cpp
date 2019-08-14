#include <deque>
#include <cstdlib>
#include <thread>
#include <omp.h>

#include <boost/container/flat_set.hpp>

#include "processManager.h"
#include "FragTraceManager.h"
#include "FeatureExtractor.h"

FragTraceManager::FragTraceManager(const Image4DSimple* inputImg4DSimplePtr, workMode mode, bool slices)
{
	char* numProcsC;
	numProcsC = getenv("NUMBER_OF_PROCESSORS");
	string numProcsString(numProcsC);
	this->numProcs = stoi(numProcsString);

	this->mode = mode;

	int dims[3];
	dims[0] = inputImg4DSimplePtr->getXDim();
	dims[1] = inputImg4DSimplePtr->getYDim();
	dims[2] = inputImg4DSimplePtr->getZDim();
	cout << " -- Current image block dimensions: " << dims[0] << " " << dims[1] << " " << dims[2] << endl;
	int totalbyte = inputImg4DSimplePtr->getTotalBytes();
	unsigned char* img1Dptr = new unsigned char[dims[0] * dims[1] * dims[2]];
	memcpy(img1Dptr, inputImg4DSimplePtr->getRawData(), totalbyte);
		
	//if (mode == dendriticTree)
	//{
		/*unsigned char* dendrite1Dptr = new unsigned char[(dims[0] / 2) * (dims[1] / 2) * dims[2]];
		int downFacs[3];
		downFacs[0] = 2;
		downFacs[1] = 2;
		downFacs[2] = 1;
		ImgProcessor::imgDownSampleMax(img1Dptr, dendrite1Dptr, dims, downFacs);
		delete[] img1Dptr;
		img1Dptr = dendrite1Dptr;
		dendrite1Dptr = nullptr; // [dendrite1Dptr]'s memory block is passed to [img1Dptr] and cannot be freed. 
								 // Therefore, we cannot "delete[] dendrite1Dptr". But we can use nullptr to nullify dendrite1Dptr to ensure the safety.

		dims[0] = dims[0] / 2;
		dims[1] = dims[1] / 2;*/
	//}
	
	vector<vector<unsigned char>> imgSlices;
	ImgProcessor::imgStackSlicer(img1Dptr, imgSlices, dims);
	registeredImg inputRegisteredImg;
	inputRegisteredImg.imgAlias = "currBlockSlices"; // This is the original images.
	inputRegisteredImg.dims[0] = dims[0];
	inputRegisteredImg.dims[1] = dims[1];
	inputRegisteredImg.dims[2] = dims[2];
	delete[] img1Dptr;

	int sliceNum = 0;
	for (vector<vector<unsigned char>>::iterator sliceIt = imgSlices.begin(); sliceIt != imgSlices.end(); ++sliceIt)
	{
		++sliceNum;
		string sliceName;
		if (sliceNum / 10 == 0) sliceName = "000" + to_string(sliceNum) + ".tif";
		else if (sliceNum / 100 == 0) sliceName = "00" + to_string(sliceNum) + ".tif";
		else if (sliceNum / 1000 == 0) sliceName = "0" + to_string(sliceNum) + ".tif";
		else sliceName = to_string(sliceNum) + ".tif";

		unsigned char* slicePtr = new unsigned char[dims[0] * dims[1]];
		for (int i = 0; i < sliceIt->size(); ++i) slicePtr[i] = sliceIt->at(i);
		myImg1DPtr my1Dslice(new unsigned char[dims[0] * dims[1]]);
		memcpy(my1Dslice.get(), slicePtr, sliceIt->size());
		inputRegisteredImg.slicePtrs.insert({ sliceName, my1Dslice });
		delete[] slicePtr;

		sliceIt->clear();
	}
	imgSlices.clear();

	this->fragTraceImgManager.imgDatabase.clear();
	this->fragTraceImgManager.imgDatabase.insert({ inputRegisteredImg.imgAlias, inputRegisteredImg });

	this->adaImgName.clear();
	this->histThreImgName.clear();
	cout << " -- Image slice preparation done." << endl;
	cout << "Image acquisition done. Start fragment tracing.." << endl;

	// ******* QProgressDialog is automatically in separate thread, otherwise, the dialog can NEVER be updated ******* //
	// NOTE: The parent widget it FragTraceManager, not FragTraceControlPanel. Thus FragTraceControlPanel is still frozen since FragTraceManager is not finished yet.
	//       FragTraceControlPanel and FragTraceManager are on the same thread.
	this->progressBarDiagPtr = new QProgressDialog(this); 
	this->progressBarDiagPtr->setWindowTitle("Neuron segment generation in progress");
	this->progressBarDiagPtr->setMinimumWidth(400);
	this->progressBarDiagPtr->setRange(0, 100);
	this->progressBarDiagPtr->setModal(true);
	this->progressBarDiagPtr->setCancelButtonText("Abort");
	// *************************************************************************************************************** //
}




// ***************** TRACING PROCESS CONTROLING FUNCTION ***************** //
bool FragTraceManager::imgProcPipe_wholeBlock()
{
	cout << "number of slices: " << this->fragTraceImgManager.imgDatabase.begin()->second.slicePtrs.size() << endl;
	V3DLONG dims[4];
	dims[0] = this->fragTraceImgManager.imgDatabase.begin()->second.dims[0];
	dims[1] = this->fragTraceImgManager.imgDatabase.begin()->second.dims[1];
	dims[2] = 1;
	dims[3] = 1;

	int imgDims[3];
	imgDims[0] = dims[0];
	imgDims[1] = dims[1];
	imgDims[2] = this->fragTraceImgManager.imgDatabase.begin()->second.slicePtrs.size();

	if (this->ada) this->adaThre("currBlockSlices", dims, this->adaImgName);
	
	if (this->cutoffIntensity != 0)
	{
		this->simpleThre(this->adaImgName, dims, "ada_cutoff");
		
		if (this->gammaCorrection)
		{
			this->gammaCorrect("ada_cutoff", dims, "gammaCorrected");
			this->histThreImg3D("gammaCorrected", dims, this->histThreImgName);   // -> still needs this with gamma?
			if (!this->mask2swc(this->histThreImgName, "blobTree")) return false;
		}
		else 
		{
			if (this->mode == dendriticTree)
			{
				//clock_t begin = clock();
				if (!this->mask2swc("ada_cutoff", "blobTree")) return false;
				//clock_t end = clock();
				//float elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
				//cout << "Connected component TIME ELAPSED: " << elapsed_secs << " SECS" << endl << endl;
			}
			else
			{
				this->histThreImg3D("ada_cutoff", dims, this->histThreImgName);
				if (!this->mask2swc(this->histThreImgName, "blobTree")) return false;
			}
		}
	}
	else
	{
		if (this->gammaCorrection)
		{
			this->gammaCorrect(this->adaImgName, dims, "gammaCorrected");
			this->histThreImg3D("gammaCorrected", dims, this->histThreImgName);
			if (!this->mask2swc(this->histThreImgName, "blobTree")) return false;
		}
		else
		{
			this->histThreImg3D(this->adaImgName, dims, this->histThreImgName);
			if (!this->mask2swc(this->histThreImgName, "blobTree")) return false;
		}
	}

	NeuronTree denScaleBackTree, finalOutputTree;
	if (this->mode == axon)
	{
		profiledTree objSkeletonProfiledTree;
		if (!this->generateTree(axon, objSkeletonProfiledTree)) return false;;
		this->fragTraceTreeManager.treeDataBase.insert({ "objSkeleton", objSkeletonProfiledTree });
		//QString skeletonTreeNameQ = this->finalSaveRootQ + "/skeletonTree.swc";
		//writeSWC_file(skeletonTreeNameQ, objSkeletonTree);

		NeuronTree MSTbranchBreakTree = TreeGrower::branchBreak(objSkeletonProfiledTree);
		profiledTree objBranchBreakTree(MSTbranchBreakTree);
		this->fragTraceTreeManager.treeDataBase.insert({ "objBranchBreakTree", objBranchBreakTree });
		//QString branchBreakTreeName = this->finalSaveRootQ + "/branchBreakTree.swc";
		//writeSWC_file(branchBreakTreeName, objBranchBreakTree.tree);

		profiledTree downSampledProfiledTree = NeuronStructUtil::treeDownSample(objBranchBreakTree, 2);
		QString downSampledTreeName = this->finalSaveRootQ + "/downSampledTreeTest.swc";
		writeSWC_file(downSampledTreeName, downSampledProfiledTree.tree);

		// Iterative segment elongation / connection
		profiledTree newIteredConnectedTree = this->fragTraceTreeGrower.itered_connectSegsWithinClusters(downSampledProfiledTree, 5);

		if (this->minNodeNum > 0) finalOutputTree = NeuronStructUtil::singleDotRemove(newIteredConnectedTree.tree, this->minNodeNum);
		else finalOutputTree = newIteredConnectedTree.tree;
	} 
	else if (this->mode == dendriticTree)
	{
		profiledTree profiledDenTree;
		if (!this->generateTree(dendriticTree, profiledDenTree)) return false;
		this->fragTraceTreeManager.treeDataBase.insert({ "objSkeleton", profiledDenTree });

		profiledTree downSampledDenTree = NeuronStructUtil::treeDownSample(profiledDenTree, 2); // reduce zig-zagging

		NeuronTree floatingExcludedTree;
		if (this->minNodeNum > 0) floatingExcludedTree = NeuronStructUtil::singleDotRemove(downSampledDenTree, this->minNodeNum);
		else floatingExcludedTree = downSampledDenTree.tree;

		profiledTree dnSampledProfiledTree(floatingExcludedTree);
		//QString beforeSpikeRemoveSWCfullName = this->finalSaveRootQ + "\\beforeSpikeRemove.swc";
		//writeSWC_file(beforeSpikeRemoveSWCfullName, dnSampledProfiledTree.tree);
		profiledTree spikeRemovedProfiledTree = TreeGrower::itered_spikeRemoval(dnSampledProfiledTree, 2);
		float angleThre = (float(2) / float(3)) * PI;
		profiledTree hookRemovedProfiledTree = TreeGrower::removeHookingHeadTail(spikeRemovedProfiledTree, angleThre);

		//finalOutputTree = dnSampledProfiledTree.tree;
		finalOutputTree = hookRemovedProfiledTree.tree; // cancel image volume downsampling since the polar coord approach is fast
													    // without downsampling, tracing result's inacurracy is remedied.
	}

	for (QList<NeuronSWC>::iterator nodeIt = finalOutputTree.listNeuron.begin(); nodeIt != finalOutputTree.listNeuron.end(); ++nodeIt)
		nodeIt->type = 16;

	if (this->finalSaveRootQ != "")
	{
		QString localSWCFullName = this->finalSaveRootQ + "/currBlock.swc";
		writeSWC_file(localSWCFullName, finalOutputTree);
	}
	
	if (this->progressBarDiagPtr->isVisible()) this->progressBarDiagPtr->close();

	emit emitTracedTree(finalOutputTree);
}
// *********************************************************************** //



/*************************** Image Enhancement ***************************/
void FragTraceManager::adaThre(const string inputRegImgName, V3DLONG dims[], const string outputRegImgName)
{
	registeredImg adaSlices;
	adaSlices.imgAlias = outputRegImgName;
	adaSlices.dims[0] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[0];
	adaSlices.dims[1] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[1];
	adaSlices.dims[2] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[2];

	int sliceDims[3];
	sliceDims[0] = adaSlices.dims[0];
	sliceDims[1] = adaSlices.dims[1];
	sliceDims[2] = 1;
	for (map<string, myImg1DPtr>::iterator sliceIt = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.begin();
		sliceIt != this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.end(); ++sliceIt)
	{
		myImg1DPtr my1Dslice(new unsigned char[sliceDims[0] * sliceDims[1]]);
		ImgProcessor::simpleAdaThre(sliceIt->second.get(), my1Dslice.get(), sliceDims, this->simpleAdaStepsize, this->simpleAdaRate);
		adaSlices.slicePtrs.insert({ sliceIt->first, my1Dslice });
	}
	this->fragTraceImgManager.imgDatabase.insert({ adaSlices.imgAlias, adaSlices });

	if (this->saveAdaResults)
	{
		QString saveRootQ = this->simpleAdaSaveDirQ + "\\" + QString::fromStdString(outputRegImgName) + "_" + QString::fromStdString(to_string(this->simpleAdaStepsize)) + "_" + QString::fromStdString(to_string(this->simpleAdaRate));
		this->saveIntermediateResult(outputRegImgName, saveRootQ, dims);
	}
}

void FragTraceManager::simpleThre(const string inputRegImgName, V3DLONG dims[], const string outputRegImgName)
{
	registeredImg adaSlices;
	adaSlices.imgAlias = outputRegImgName;
	adaSlices.dims[0] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[0];
	adaSlices.dims[1] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[1];
	adaSlices.dims[2] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[2];

	int sliceDims[3];
	sliceDims[0] = adaSlices.dims[0];
	sliceDims[1] = adaSlices.dims[1];
	sliceDims[2] = 1;
	for (map<string, myImg1DPtr>::iterator sliceIt = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.begin();
		sliceIt != this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.end(); ++sliceIt)
	{
		myImg1DPtr my1Dslice(new unsigned char[sliceDims[0] * sliceDims[1]]);
		ImgProcessor::simpleThresh(sliceIt->second.get(), my1Dslice.get(), sliceDims, this->cutoffIntensity);
		adaSlices.slicePtrs.insert({ sliceIt->first, my1Dslice });
	}
	this->fragTraceImgManager.imgDatabase.insert({ adaSlices.imgAlias, adaSlices });

	if (this->saveAdaResults)
	{
		QString saveRootQ = this->simpleAdaSaveDirQ + "\\" + QString::fromStdString(outputRegImgName) + "_" + QString::fromStdString(to_string(this->cutoffIntensity));
		this->saveIntermediateResult(outputRegImgName, saveRootQ, dims);
	}
}

void FragTraceManager::gammaCorrect(const string inputRegImgName, V3DLONG dims[], const string outputRegImgName)
{
	registeredImg gammaSlices;
	gammaSlices.imgAlias = outputRegImgName;
	gammaSlices.dims[0] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[0];
	gammaSlices.dims[1] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[1];
	gammaSlices.dims[2] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[2];

	int sliceDims[3];
	sliceDims[0] = gammaSlices.dims[0];
	sliceDims[1] = gammaSlices.dims[1];
	sliceDims[2] = 1;
	for (map<string, myImg1DPtr>::iterator sliceIt = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.begin();
		sliceIt != this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.end(); ++sliceIt)
	{
		myImg1DPtr my1Dslice(new unsigned char[sliceDims[0] * sliceDims[1]]);
		ImgProcessor::stepped_gammaCorrection(sliceIt->second.get(), my1Dslice.get(), sliceDims, 5);
		gammaSlices.slicePtrs.insert({ sliceIt->first, my1Dslice });
	}
	this->fragTraceImgManager.imgDatabase.insert({ gammaSlices.imgAlias, gammaSlices });

	if (this->saveAdaResults)
	{
		QString saveRootQ = this->simpleAdaSaveDirQ + "\\" + QString::fromStdString(outputRegImgName);
		this->saveIntermediateResult(outputRegImgName, saveRootQ, dims);
	}
}
/********************** END of [Image Enhancement] ***********************/



/*************************** Image Segmentation ***************************/
void FragTraceManager::histThreImg(const string inputRegImgName, V3DLONG dims[], const string outputRegImgName)
{
	if (this->fragTraceImgManager.imgDatabase.find(inputRegImgName) == this->fragTraceImgManager.imgDatabase.end())
	{
		cerr << "No source image found. Do nothing and return.";
	}

	registeredImg histThreSlices;
	histThreSlices.imgAlias = outputRegImgName;
	histThreSlices.dims[0] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[0];
	histThreSlices.dims[1] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[1];
	histThreSlices.dims[2] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[2];

	int sliceDims[3];
	sliceDims[0] = histThreSlices.dims[0];
	sliceDims[1] = histThreSlices.dims[1];
	sliceDims[2] = 1;

	for (map<string, myImg1DPtr>::iterator sliceIt = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.begin();
		sliceIt != this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.end(); ++sliceIt)
	{
		myImg1DPtr my1Dslice(new unsigned char[sliceDims[0] * sliceDims[1]]);
		map<string, float> imgStats = ImgProcessor::getBasicStats_no0(sliceIt->second.get(), sliceDims);
		ImgProcessor::simpleThresh(sliceIt->second.get(), my1Dslice.get(), sliceDims, int(floor(imgStats.at("mean") + this->stdFold * imgStats.at("std"))));
		histThreSlices.slicePtrs.insert({ sliceIt->first, my1Dslice });
	}
	this->fragTraceImgManager.imgDatabase.insert({ histThreSlices.imgAlias, histThreSlices });

	if (this->saveHistThreResults)
	{
		QString saveRootQ = this->histThreSaveDirQ + "\\" + QString::fromStdString(outputRegImgName) + "_std" + QString::fromStdString(to_string(this->stdFold));
		this->saveIntermediateResult(outputRegImgName, saveRootQ, dims);
	}
}

void FragTraceManager::histThreImg3D(const string inputRegImgName, V3DLONG dims[], const string outputRegImgName)
{
	if (this->fragTraceImgManager.imgDatabase.find(inputRegImgName) == this->fragTraceImgManager.imgDatabase.end())
	{
		cerr << "No source image found. Do nothing and return.";
	}

	registeredImg histThreSlices;
	histThreSlices.imgAlias = outputRegImgName;
	histThreSlices.dims[0] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[0];
	histThreSlices.dims[1] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[1];
	histThreSlices.dims[2] = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).dims[2];

	int sliceDims[3];
	sliceDims[0] = histThreSlices.dims[0];
	sliceDims[1] = histThreSlices.dims[1];
	sliceDims[2] = 1;
	map<int, size_t> binMap3D;
	for (map<string, myImg1DPtr>::iterator sliceIt = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.begin();
		sliceIt != this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.end(); ++sliceIt)
	{
		map<int, size_t> sliceHistMap = ImgProcessor::histQuickList(sliceIt->second.get(), sliceDims);
		for (map<int, size_t>::iterator binIt = sliceHistMap.begin(); binIt != sliceHistMap.end(); ++binIt)
		{
			if (binMap3D.find(binIt->first) == binMap3D.end()) binMap3D.insert(*binIt);
			else binMap3D[binIt->first] = binMap3D[binIt->first] + binIt->second;
		}
	}
	map<string, float> histList3D = ImgProcessor::getBasicStats_no0_fromHist(binMap3D);

	for (map<string, myImg1DPtr>::iterator sliceIt = this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.begin();
		sliceIt != this->fragTraceImgManager.imgDatabase.at(inputRegImgName).slicePtrs.end(); ++sliceIt)
	{
		myImg1DPtr my1Dslice(new unsigned char[sliceDims[0] * sliceDims[1]]);
		ImgProcessor::simpleThresh(sliceIt->second.get(), my1Dslice.get(), sliceDims, int(floor(histList3D.at("mean") + this->stdFold * histList3D.at("std"))));
		histThreSlices.slicePtrs.insert({ sliceIt->first, my1Dslice });
	}
	this->fragTraceImgManager.imgDatabase.insert({ histThreSlices.imgAlias, histThreSlices });

	if (this->saveHistThreResults)
	{
		QString saveRootQ = this->histThreSaveDirQ + "\\" + QString::fromStdString(outputRegImgName) + "_std" + QString::fromStdString(to_string(this->stdFold));
		this->saveIntermediateResult(outputRegImgName, saveRootQ, dims);
	}
}

bool FragTraceManager::mask2swc(const string inputImgName, string outputTreeName)
{
	int sliceDims[3];
	sliceDims[0] = this->fragTraceImgManager.imgDatabase.at(inputImgName).dims[0];
	sliceDims[1] = this->fragTraceImgManager.imgDatabase.at(inputImgName).dims[1];
	sliceDims[2] = 1;

	vector<unsigned char**> slice2DVector;
	unsigned char* mipPtr = new unsigned char[sliceDims[0] * sliceDims[1]];
	for (int i = 0; i < sliceDims[0] * sliceDims[1]; ++i) mipPtr[i] = 0;
	for (map<string, myImg1DPtr>::iterator sliceIt = this->fragTraceImgManager.imgDatabase.at(inputImgName).slicePtrs.begin(); 
		sliceIt != this->fragTraceImgManager.imgDatabase.at(inputImgName).slicePtrs.end(); ++sliceIt)
	{
		ImgProcessor::imgMax(sliceIt->second.get(), mipPtr, mipPtr, sliceDims);

		unsigned char** slice2DPtr = new unsigned char*[sliceDims[1]];
		for (int j = 0; j < sliceDims[1]; ++j)
		{
			slice2DPtr[j] = new unsigned char[sliceDims[0]];
			for (int i = 0; i < sliceDims[0]; ++i) slice2DPtr[j][i] = sliceIt->second.get()[sliceDims[0] * j + i];
		}
		slice2DVector.push_back(slice2DPtr);
	}
	
	this->signalBlobs.clear();
	ProcessManager myReporter(this->fragTraceImgAnalyzer.progressReading);
	this->fragTraceImgAnalyzer.reportProcess(ImgAnalyzer::blobMerging);
	QTimer::singleShot(100, this, SLOT(blobProcessMonitor(myReporter)));
	//this->blobProcessMonitor(myReporter);
	std::thread testThread(myReporter);
	this->signalBlobs = this->fragTraceImgAnalyzer.findSignalBlobs(slice2DVector, sliceDims, 3, mipPtr);
	testThread.join();

	NeuronTree blob3Dtree = NeuronStructUtil::blobs2tree(this->signalBlobs, true);
	if (this->finalSaveRootQ != "")
	{
		QString blobTreeFullFilenameQ = this->finalSaveRootQ + "\\blob.swc";
		writeSWC_file(blobTreeFullFilenameQ, blob3Dtree);
	}
	profiledTree profiledSigTree(blob3Dtree);
	this->fragTraceTreeManager.treeDataBase.insert({ outputTreeName, profiledSigTree });

	// ----------- Releasing memory ------------ //
	delete[] mipPtr;
	mipPtr = nullptr;
	for (vector<unsigned char**>::iterator slice2DPtrIt = slice2DVector.begin(); slice2DPtrIt != slice2DVector.end(); ++slice2DPtrIt)
	{
		for (int yi = 0; yi < sliceDims[1]; ++yi)
		{
			delete[] (*slice2DPtrIt)[yi];
			(*slice2DPtrIt)[yi] = nullptr;
		}
		delete[] *slice2DPtrIt;
		*slice2DPtrIt = nullptr;
	}
	slice2DVector.clear();
	// ------- END of [Releasing memory] ------- //
}

// ----------------- Object Classification ----------------- //
void FragTraceManager::smallBlobRemoval(vector<connectedComponent>& signalBlobs, const int sizeThre)
{
	if (signalBlobs.empty())
	{
		cerr << "No signal blobs data exists. Do nothing and return." << endl;
		return;
	}

	cout << " ==> removing small blob, size threshold = " << sizeThre << endl;

	vector<ptrdiff_t> delLocs;
	for (vector<connectedComponent>::iterator compIt = signalBlobs.begin(); compIt != signalBlobs.end(); ++compIt)
		if (compIt->size <= sizeThre) delLocs.push_back(compIt - signalBlobs.begin());

	sort(delLocs.rbegin(), delLocs.rend());
	for (vector<ptrdiff_t>::iterator delIt = delLocs.begin(); delIt != delLocs.end(); ++delIt)
		signalBlobs.erase(signalBlobs.begin() + *delIt);
}
// ------------ END of [Object Classification] ------------- //
/*********************** END of [Image Segmentation] **********************/



/*************************** Final Traced Tree Generation ***************************/
bool FragTraceManager::generateTree(workMode mode, profiledTree& objSkeletonProfiledTree)
{
	cout << endl << "Finishing up processing objects.." << endl;

	if (!this->progressBarDiagPtr->isVisible()) this->progressBarDiagPtr->show();
	this->progressBarDiagPtr->setLabelText("Extracting fragments from 3D signal objects..");

	if (mode == axon)
	{		
		vector<NeuronTree> objTrees;
		NeuronTree finalCentroidTree;
		// ------- using omp to speed up skeletonization process ------- //
#pragma omp parallel num_threads(this->numProcs)
		{
			for (vector<connectedComponent>::iterator it = this->signalBlobs.begin(); it != this->signalBlobs.end(); ++it)
			{
				qApp->processEvents();
				if (this->progressBarDiagPtr->wasCanceled())
				{
					this->progressBarDiagPtr->setLabelText("Process aborted.");
					return false;
				}

				if (it->size < voxelCount) continue;
				if (int(it - this->signalBlobs.begin()) % 500 == 0)
				{
					double progressBarValue = (double(it - this->signalBlobs.begin()) / this->signalBlobs.size()) * 100;
					int progressBarValueInt = ceil(progressBarValue);
					this->progressBarDiagPtr->setValue(progressBarValueInt);
					cout << int(it - this->signalBlobs.begin()) << " ";
				}

				NeuronTree centroidTree;
				boost::container::flat_set<deque<float>> sectionalCentroids = this->fragTraceImgAnalyzer.getSectionalCentroids(*it);
				for (boost::container::flat_set<deque<float>>::iterator nodeIt = sectionalCentroids.begin(); nodeIt != sectionalCentroids.end(); ++nodeIt)
				{
					NeuronSWC newNode;
					newNode.x = nodeIt->at(0);
					newNode.y = nodeIt->at(1);
					newNode.z = nodeIt->at(2);
					newNode.type = 2;
					newNode.parent = -1;
					centroidTree.listNeuron.push_back(newNode);
				}
				finalCentroidTree.listNeuron.append(centroidTree.listNeuron);

				NeuronTree MSTtree = TreeGrower::SWC2MSTtree_boost(centroidTree);
				profiledTree profiledMSTtree(MSTtree);				
				//profiledTree smoothedTree = NeuronStructExplorer::spikeRemove(profiledMSTtree); -> This can cause error and terminate the program. Need to investigate the implementation.
				objTrees.push_back(profiledMSTtree.tree);
			}

			QString finalCentroidTreeNameQ = this->finalSaveRootQ + "/finalCentroidTree.swc";
			writeSWC_file(finalCentroidTreeNameQ, finalCentroidTree);
			cout << endl;
		}

		NeuronTree objSkeletonTree = NeuronStructUtil::swcCombine(objTrees);
		profiledTree outputProfiledTree(objSkeletonTree);
		objSkeletonProfiledTree = outputProfiledTree;
		return true;
		// ------------------------------------------------------------- //
	}
	else if (mode == dendriticTree)
	{
		connectedComponent dendriteComponent = *this->signalBlobs.begin();
		for (vector<connectedComponent>::iterator compIt = this->signalBlobs.begin() + 1; compIt != this->signalBlobs.end(); ++compIt)
			if (compIt->size > dendriteComponent.size) dendriteComponent = *compIt;

		vector<connectedComponent> denComp = { dendriteComponent };
		NeuronTree denBlobTree = NeuronStructUtil::blobs2tree(denComp);
		// ------------------------ FOR DEBUG ------------------------------ //
		QString denBlobSaveNameQ = this->finalSaveRootQ + "\\denBlob.swc"; //
		writeSWC_file(denBlobSaveNameQ, denBlobTree);                      //
		// ----------------------------------------------------------------- //
		vector<int> origin = { 128, 128, 128 };
		NeuronGeoGrapher::nodeList2polarNodeList(denBlobTree.listNeuron, this->fragTraceTreeGrower.polarNodeList, origin);  // Converts NeuronSWC list to polarNeuronSWC list.
		this->fragTraceTreeGrower.radiusShellMap_loc = NeuronGeoGrapher::getShellByRadius_loc(this->fragTraceTreeGrower.polarNodeList);
		this->fragTraceTreeGrower.dendriticTree_shellCentroid(); // Dendritic tree is generated here.

		//QString denSaveName = this->finalSaveRootQ + "\\newDenTest.swc";
		//writeSWC_file(denSaveName, this->fragTraceTreeGrower.treeDataBase.at("dendriticProfiledTree").tree);
		objSkeletonProfiledTree = this->fragTraceTreeGrower.treeDataBase.at("dendriticProfiledTree");
		return true;
	}
}

profiledTree FragTraceManager::segConnectAmongTrees(const profiledTree& inputProfiledTree, float distThreshold)
{
	profiledTree tmpTree = inputProfiledTree; 
	profiledTree outputProfiledTree = this->fragTraceTreeGrower.itered_connectSegsWithinClusters(tmpTree, distThreshold);
	
	bool typeAssigned = false;
	int assignedType;
	for (map<int, segUnit>::iterator segIt = outputProfiledTree.segs.begin(); segIt != outputProfiledTree.segs.end(); ++segIt)
	{
		for (QList<NeuronSWC>::iterator nodeIt = segIt->second.nodes.begin(); nodeIt != segIt->second.nodes.end(); ++nodeIt)
		{
			if (nodeIt->type != 7)
			{
				typeAssigned = true;
				assignedType = nodeIt->type;
				break;
			}
		}

		if (typeAssigned)
		{
			cout << "existing segment found" << endl;
			for (QList<NeuronSWC>::iterator nodeIt = segIt->second.nodes.begin(); nodeIt != segIt->second.nodes.end(); ++nodeIt)
			{
				nodeIt->type = assignedType;
				outputProfiledTree.tree.listNeuron[outputProfiledTree.node2LocMap.at(nodeIt->n)].type = assignedType;
			}
			typeAssigned = false;
			assignedType = 7;
		}
	}

	bool duplicatedRemove = true;
	while (duplicatedRemove)
	{
		for (QList<NeuronSWC>::iterator it = outputProfiledTree.tree.listNeuron.begin(); it != outputProfiledTree.tree.listNeuron.end(); ++it)
		{
			if (it->n == it->parent)
			{
				outputProfiledTree.tree.listNeuron.removeAt(int(it - outputProfiledTree.tree.listNeuron.begin()));
				goto DUPLICATE_REMOVED;
			}
		}
		break;

	DUPLICATE_REMOVED:
		continue;
	}

	QString combinedTreeFullName = this->finalSaveRootQ + "/combinedTree.swc";
	writeSWC_file(combinedTreeFullName, outputProfiledTree.tree);

	return outputProfiledTree;
}
/********************** END of [Final Traced Tree Generation] ***********************/



bool FragTraceManager::blobProcessMonitor(ProcessManager& blobMonitor)
{
	cout << "test" << endl;
	system("pause");
	if (!this->progressBarDiagPtr->isVisible()) this->progressBarDiagPtr->show();
	this->progressBarDiagPtr->setLabelText("Processing each image signal object...");
	qApp->processEvents();

	int progressBarValueInt = blobMonitor.readingFromClient;
	while (progressBarValueInt <= 100)
	{
		if (this->progressBarDiagPtr->wasCanceled())
		{
			this->progressBarDiagPtr->setLabelText("Process aborted.");
			v3d_msg("The process has been terminated.");
			return false;
		}

		this->progressBarDiagPtr->setValue(progressBarValueInt);
	}

	return true;
}