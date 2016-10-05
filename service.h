#ifndef __SERVICE_H__
#define __SERVICE_H__
#include <openbr/openbr_plugin.h>
#include <QString>
#include <fstream>
#include <iostream>
#include <string>
#include <QFile>
#define MAX_NUM 30
#define MAXDATA(a,b) ( (a) > (b) ) ? (a):(b)
const QString dataPath = "/home/mini/OPEL/service/data/";
const QString fileName = "predata.txt";
typedef struct _targetDataSet
{
	QString name;
	QString imgPath;
	QString age;
	QString gender;
	QString personalInfo;
	QString email;
}targetDataSet;


class ImageProcessing
{
	public:
		ImageProcessing();
		~ImageProcessing();
		bool initialize(const QString& algorithmType);
		float comparesImg(const QString &targetPath, const QString &queryPath);
	  void printTemplate(const br::Template &t);	
		void parseJsonData(void);
		void showMatchingScore(float score, targetDataSet* dtset);
		int ageEstimation(const QString &targetPath);
		QString genderEstimation(const QString &targetPath);
		targetDataSet* getMatchingItem(const QString &queryPath);
	private:
		
		QSharedPointer<br::Transform> ageEst;
		QSharedPointer<br::Transform> genderEst;
		QSharedPointer<br::Transform> transform;
		QSharedPointer<br::Distance> distance;
		targetDataSet** dataSet;
};

#endif /*__SERVICE_H__*/
