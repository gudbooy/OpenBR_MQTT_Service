#include "service.h"
#include <openbr/openbr_plugin.h>

ImageProcessing::ImageProcessing()
{
	int argc;
	char** argv;
	this->dataSet = new targetDataSet*[MAX_NUM];
	br::Context::initialize(argc, argv);
}

ImageProcessing::~ImageProcessing()
{
	br::Context::finalize();
	for(int i=0; i<MAX_NUM; i++)
		delete dataSet[i];
	delete[] dataSet;
	dataSet = NULL;
}

void ImageProcessing::printTemplate(const br::Template &t)
{
	const QPoint firstEye = t.file.get<QPoint>("Affine_0");
	const QPoint secondEye = t.file.get<QPoint>("Affine_1");
//	printf("%s eyes: (%d, %d) (%d, %d)\n", qPrintable(t.file.fileName()), firstEye.x(), firstEye.y(), secondEye.x(), secondEye.y());
}

bool ImageProcessing::initialize(const QString& algorithmType)
{
  this->transform = br::Transform::fromAlgorithm(algorithmType);
	this->distance = br::Distance::fromAlgorithm(algorithmType);
}

float ImageProcessing::comparesImg(const QString &targetPath, const QString &queryPath)
{
//	std::cout << "TargetPath : " << targetPath.toStdString() << std::endl;
	if(targetPath.length() == 0)
	{
		return 0;
	}
	br::Template target(targetPath);
	br::Template query(queryPath);

	query >> *transform;
	target >> *transform;

//	this->printTemplate(target);
//	this->printTemplate(query);
	
	return this->distance->compare(target, query);
}
void ImageProcessing::parseJsonData(void)
{
	int cnt = 0;
	std::string name, imgPath, age, gender, personalInfo, email;
	const QString temp = dataPath+fileName;
	const char* preData = temp.toStdString().c_str();
	std::ifstream inFile(preData);
	
	if(inFile == NULL)
	{
		std::cout << "Open Failed" << std::endl;
		return;
	}
	
//	std::cout << "Open Success" << std::endl;
	for(int i=0; i<MAX_NUM; i++)
	 dataSet[i] = new targetDataSet;
	
	std::string tmp;
	while(!inFile.eof())
	{
		inFile >> name >> imgPath >> age >> gender >> personalInfo >> email;
	//	std::cout << name << " " << imgPath << std::endl;
		dataSet[cnt]->name = QString::fromStdString(name);
		dataSet[cnt]->imgPath = QString::fromStdString(imgPath);
		dataSet[cnt]->age = QString::fromStdString(age);
		dataSet[cnt]->gender = QString::fromStdString(gender);
		dataSet[cnt]->personalInfo = QString::fromStdString(personalInfo);
		dataSet[cnt]->email = QString::fromStdString(email);	
//		std::cout << dataSet[cnt]->name.toStdString() << std::endl;
//		std::cout << dataSet[cnt]->imgPath.toStdString() << std::endl;
//		std::cout << dataSet[cnt]->age.toStdString() << std::endl;
//		std::cout << dataSet[cnt]->personalInfo.toStdString() << std::endl;
//		std::cout << dataSet[cnt]->email.toStdString() << std::endl;
		cnt++;

	}
	inFile.close();
}
void ImageProcessing::showMatchingScore(float score, targetDataSet* dtset)
{
	std::cout << "Score : "<< score <<" Name : " << dtset->name.toStdString() << std::endl;
}
targetDataSet* ImageProcessing::getMatchingItem(const QString &queryPath)
{
	float preResult = 0;
  float maxValue = 0;	
	targetDataSet* matchedItem = NULL;
	for(int i=0; i<MAX_NUM; i++)
	{
		if(maxValue < (preResult = comparesImg(dataSet[i]->imgPath, queryPath)))
		{
			maxValue = preResult;
			if(maxValue > 1)
				matchedItem = dataSet[i];	
		}
		showMatchingScore(preResult, dataSet[i]);
	}
	return matchedItem;
}
int ImageProcessing::ageEstimation(const QString &targetPath)
{
	this->ageEst = br::Transform::fromAlgorithm("AgeEstimation");
	br::Template query(targetPath);
	query >> *(this->ageEst);
	return int(query.file.get<float>("Age"));
}
QString ImageProcessing::genderEstimation(const QString &targetPath)
{
	this->genderEst = br::Transform::fromAlgorithm("GenderEstimation");
	br::Template query(targetPath);
	query >> *(this->genderEst);
	return query.file.get<QString>("Gender");
}

