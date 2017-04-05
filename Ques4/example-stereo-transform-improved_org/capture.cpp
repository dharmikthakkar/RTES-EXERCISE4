/*
 *
 *  Example by Sam Siewert 
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;


int main( int argc, char** argv )
{
    /* Creates a window */
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    //CvCapture* capture = cvCreateCameraCapture(0);
    CvCapture* capture;
    IplImage* frame;
    int dev=0;

    if(argc > 1)
    {
        printf("argv[1]=%s\n", argv[1]);
        sscanf(argv[1], "%d", &dev);
        printf("Will open video device %d\n", dev);
	/* Start capturing frames from camera */
        capture = cvCreateCameraCapture(dev);
    }

    while(1)
    {
	/* Grab a frame from a camera or video file, decompress and return it */
        frame=cvQueryFrame(capture);
     
        if(!frame) break;

        cvShowImage("Capture Example", frame);

        char c = cvWaitKey(33);
        if( c == 27 ) break;
    }

    /* Stop capturing frames */
    cvReleaseCapture(&capture);

    /* Destroy created window */
    cvDestroyWindow("Capture Example");
    
};
