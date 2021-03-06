// objectTracking.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <opencv\highgui.h>
#include <opencv\cv.h>
#include <opencv2\opencv.hpp>

using namespace cv;


// Initial min & max HSV filter values for trackbars
int H_MIN = 0;
int H_MAX = 256;
int S_MIN = 0;
int S_MAX = 256;
int V_MIN = 0;
int V_MAX = 256;

// default capture width & height
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
// Max number of objects to be detected in framw
const int MAX_NUM_OBJECTS = 50;
// Minimum and Maximum object area
const int MIN_OBJECT_AREA = 20 * 20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT * FRAME_WIDTH / 1.5;
// name for window headers
const String OG_WINDOW_NAME = "Original Image";
const String HSV_WINDOW_NAME = "HSV Image";
const String THRESHOLDED_WINDOW_NAME = "Thresholded Image";
const String MORPH_WINDOW_NAME = "After Morphological Operations";
const String TRACKBAR_WINDOW_NAME = "Trackbars";

void onTrackbar(int, void*)
{
	// function gets called whenever trackbar position is changed
}

String intToString(int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

void createTrackbars()
{
	// create window
	namedWindow(TRACKBAR_WINDOW_NAME, 0);

	// create memory to store trackbar name on window
	char TrackbarName[50];
	sprintf_s(TrackbarName, "H_MIN", H_MIN);
	sprintf_s(TrackbarName, "H_MAX", H_MAX);
	sprintf_s(TrackbarName, "S_MIN", S_MIN);
	sprintf_s(TrackbarName, "S_MAX", S_MAX);
	sprintf_s(TrackbarName, "V_MIN", V_MIN);
	sprintf_s(TrackbarName, "V_MAX", V_MAX);

	//create trackbars and insert them into window
	// 3 parameters are address of the variable that is changing when the trackbar is moved(eg. H_MIN),
	//the max value the tracker can move (e.g H_MAX),
	// And the function that is called whenever the trackbar is moved
	createTrackbar("H_MIN", TRACKBAR_WINDOW_NAME, &H_MIN, H_MAX, onTrackbar);
	createTrackbar("H_MAX", TRACKBAR_WINDOW_NAME, &H_MAX, H_MAX, onTrackbar);
	createTrackbar("S_MIN", TRACKBAR_WINDOW_NAME, &S_MIN, S_MAX, onTrackbar);
	createTrackbar("S_MAX", TRACKBAR_WINDOW_NAME, &S_MAX, S_MAX, onTrackbar);
	createTrackbar("V_MIN", TRACKBAR_WINDOW_NAME, &V_MIN, V_MAX, onTrackbar);
	createTrackbar("V_MAX", TRACKBAR_WINDOW_NAME, &V_MAX, V_MAX, onTrackbar);
}

void drawObject(int x, int y, Mat &frame)
{
	//use openCV drawin functions to draw crosshairs
	circle(frame, Point(x, y), 20, Scalar(0, 255, 0), 2);
	if (y - 25 > 0)
		line(frame, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
	else
		line(frame, Point(x, y), Point(x, 0), Scalar(0, 255, 0), 2);
	if (y + 25 < FRAME_HEIGHT)
		line(frame, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	else
		line(frame, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(0, 255, 0), 2);
	if (x - 25 > 0)
		line(frame, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	else
		line(frame, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
	if (x + 25 < FRAME_WIDTH)
		line(frame, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
	else 
		line(frame, Point(x, y), Point(FRAME_WIDTH, y), Scalar(0, 255, 0), 2);

	putText(frame, intToString(x) + "," + intToString(y),Point(x,y + 30),1,1,Scalar(0,255,0),2);
}

void morphOps(Mat &thresh)
{
	// Create structuring element tp dilate and erode image
	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3,3));
	// Dilate with larger element to make sure object is nicely visible
	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);

	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);
}

void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed)
{
	Mat temp;
	threshold.copyTo(temp);
	// Vector needed for output findContours
	std::vector<std::vector<Point> > contours;
	std::vector<Vec4i> hierarchy;
	//find contours of filtered image using OpenCv findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	// use moments function to find our filtered object
	double refArea = 0;
	bool objectFound = false;
	if (hierarchy.size() > 0)
	{
		int numObjects = hierarchy.size();
		// if number of objects greater than MAX_NUM_OBJECTS we have a noisey filter
		if (numObjects < MAX_NUM_OBJECTS)
		{
			for (int index = 0; index >= 0; index = hierarchy[index][0])
			{
				Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;
				// if the area is less than 20px by 20px then it is probably just noise
				// if the area is the same as the 3/2 of the image size, probably just a bad filter
				// we only want the object with the largest area so we safe a reference area each iteration
				// and compare it to the area in the next iteration
				if (area > MIN_OBJECT_AREA && area < MAX_OBJECT_AREA && area > refArea)
				{
					x = moment.m10 / area;
					y = moment.m01 / area;
					objectFound = true;
					refArea = area;
				}
				else objectFound = false;
			}
			// let user know you found an object
			if (objectFound == true)
			{
				putText(cameraFeed, "Tracking Object",Point(0,25), 2, 1, Scalar(0,255,0), 2);
				// Draw object location on screen
				drawObject(x, y, cameraFeed);
			}
		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 25), 2, 1, Scalar(0, 255, 0), 2);
	}
}


int main()
{
	bool trackObjects = true;
	bool useMorphOps = true;

	std::cout << "\nMove trackbar position to illumuminate only the" << std::endl;
	std::cout << "object you wish to track in the Threshold window\n" << std::endl;
	system("PAUSE");

	// Matrix to store each frame of the webcam feed
	Mat cameraFeed;
	// Matrix storage for HSV image
	Mat HSV;
	// Matrix storage for threshold image
	Mat threshold;

	int x = 0, y = 0;

	createTrackbars();
	// video capture object to aquire webcam feed
	VideoCapture capture;
	//open capture object at location 0
	capture.open(0);
	// set height and width og capture frame
	capture.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
	capture.set(CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
	// strart infinite loop where webcam feed is copied to cameraFeed matrix
	while (true)
	{
		// store image to matrix
		capture.read(cameraFeed);
		//convert frame from BGR to HSV colorspace
		cvtColor(cameraFeed, HSV,COLOR_BGR2HSV);
		// Filter HSV image between values and store filtered image to threshold matrix
		inRange(HSV, Scalar(H_MIN,S_MIN,V_MIN), Scalar(H_MAX,S_MAX,V_MAX), threshold);
		// perform morphological operations on thresholded image to eliminate noise and emphazise filtred object(s)
		if (useMorphOps)
			morphOps(threshold);
		// Pass in thresholded frame to our object tracking function
		// function will return x and y cordinates of the filtered object
		if (trackObjects)
			trackFilteredObject(x, y, threshold, cameraFeed);
		// show frames
		imshow(THRESHOLDED_WINDOW_NAME, threshold);
		imshow(OG_WINDOW_NAME, cameraFeed);
		imshow(HSV_WINDOW_NAME, HSV);

		// delay 30ms so that screen can refresh
		// image will not appear without this waitKey() command
		waitKey(30);
	}
    return 0;
}

