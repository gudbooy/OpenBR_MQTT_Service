#include "service.h"
int main() 
{
	float result;
	ImageProcessing* faceRecognition = new ImageProcessing();
	faceRecognition->initialize("FaceRecognition");
	faceRecognition->parseJsonData();
//	result = faceRecognition->comparesImg("/home/mini/OPEL/service/data/face_detected.jpg", "/home/mini/OPEL/service/data/hi.jpg");	
	targetDataSet* matchedItem = faceRecognition->getMatchingItem("/home/mini/OPEL/service/data/face_detected.jpg");
	
	std::cout << "Result : " <<  matchedItem->name.toStdString() << "!!!!" << std::endl;
//	printf("Genuine match score : %.3f\n", result);

	delete faceRecognition;
	
	return 0;
}
