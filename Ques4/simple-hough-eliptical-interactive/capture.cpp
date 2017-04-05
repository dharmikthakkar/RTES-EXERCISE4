/*
 *
 *  Example by Sam Siewert 
 * 
 *  Source : OpenCV Documentation
 *    
 *  The code demonstrates the use of OpenCV functions GaussianBlur, cvtColor and 
 *  HoughCircles to detect circles in an image.
 *  Hough Circle Tranform works in a roughly analogous way to Hough Line transform.
 *  It is used to detect circles in a grayscale image. 
 *  
 *  Code Flow - 
 *  1. Create a window that can be used as a placeholder for images and trackbars.
 *  2. Create a capture device. If user has specified one as part of command line 
 *     argument, it will be used. Else, a default one with camera index 0 will be 
 *     used. Set capture window attributes, height and width.  
 *  3. Start capturing from capture device. Grab a frame to work with.
 *  4. Convert the frame to grayscale using OpenCV's cvtColor function. 
 *  5. Apply a Gaussian blur to reduce noise and avoid false circle detection. 
 *  6. Apply hough circle transform to detect circles in the image.
 *  7. Display the hough circle transform results by drawing the circles detected.
 *  8. Wait for the user exits the program.
 *  9. Stop capturing frames.
 *  10. Destroy created window.
 *     
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#define HRES 640
#define VRES 480


int main( int argc, char** argv )
{
    /* Create a window */
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    //CvCapture* capture = (CvCapture *)cvCreateCameraCapture(0);
    //CvCapture* capture = (CvCapture *)cvCreateCameraCapture(argv[1]);
    CvCapture* capture;
    IplImage* frame;
    int dev=0;
    Mat gray;
    vector<Vec3f> circles;

    if(argc > 1)
    {
        sscanf(argv[1], "%d", &dev);
        printf("using %s\n", argv[1]);
    }
    else if(argc == 1)
        printf("using default\n");

    else
    {
        printf("usage: capture [dev]\n");
        exit(-1);
    }

    /* Start capturing frames from camera */
    capture = (CvCapture *)cvCreateCameraCapture(dev);
    /* Set height property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    /* Set width property */
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);

    while(1)
    {
	/* Grab a frame from a camera or video file, decompress and return it */
        frame=cvQueryFrame(capture);

	/* Copy constructor */
        Mat mat_frame(frame);
        
	/* RGB to GRAY color conversion */
	cvtColor(mat_frame, gray, CV_BGR2GRAY);
        
	/* Blur the image using gaussian filtering to avoid false detections */
	GaussianBlur(gray, gray, Size(9,9), 2, 2);

	/* Find circles in the image using hough transform */
        HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/8, 100, 50, 0, 0);

        printf("circles.size = %d\n", circles.size());

        for( size_t i = 0; i < circles.size(); i++ )
        {
          Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
          int radius = cvRound(circles[i][2]);
          // circle center
          circle( mat_frame, center, 3, Scalar(0,255,0), -1, 8, 0 );
          // circle outline
          circle( mat_frame, center, radius, Scalar(0,0,255), 3, 8, 0 );
        }

        if(!frame) break;

        cvShowImage("Capture Example", frame);

        char c = cvWaitKey(10);
        if( c == 27 ) break;
    }

    /* Stop capturing frames */
    cvReleaseCapture(&capture);

    /* Destroy created window */
    cvDestroyWindow("Capture Example");
    
};
