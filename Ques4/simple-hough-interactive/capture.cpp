/*
 *
 *  Example by Sam Siewert 
 * 
 *  Source : OpenCV Documentation
 *    
 *  The code demonstrates the use of OpenCV functions Canny and HoughLinesP 
 *  to detect lines in an image.
 *  Hough Transform is a continuous transformation used to detect straight lines. 
 *  A canny edge detection is done before applying standard or probabilistic 
 *  hough transform, since hough transform needs a pre-processing edge detection. 
 *  
 *  Code Flow - 
 *  1. Create a window that can be used as a placeholder for images and trackbars.
 *  2. Create a capture device. If user has specified one as part of command line 
 *     argument, it will be used. Else, a default one with camera index 0 will be 
 *     used. Set capture window attributes, height and width.  
 *  3. Start capturing from capture device. Grab a frame to work with.
 *  4. Detect the edges in the image using the Canny edge detector.
 *  5. Apply standard or probabilistic hough transform to detect straight lines in 
 *     image. In this code, probabilistic hough transform is used.
 *  6. Display the hough transform results by drawing the lines detected.
 *  7. Wait for the user exits the program.
 *  8. Stop capturing frames.
 *  9. Destroy created window.
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
    /* Create a window - with CV_WINDOW_AUTOSIZE, the window size is automatically adjusted to fit the displayed image */
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    
    //CvCapture* capture = (CvCapture *)cvCreateCameraCapture(0);
    //CvCapture* capture = (CvCapture *)cvCreateCameraCapture(argv[1]);
    CvCapture* capture;
    IplImage* frame; /* C structure to store the image in the memory */
    int dev=0;
    Mat gray, canny_frame, cdst;

    vector<Vec4i> lines; /* vector that will store the parameters (rho,theta) of the detected lines */ 

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

        Mat mat_frame(frame); /* Copy Constructor */
        
	/* Canny Edge detector with thresholds (50 (low) and 200 (high)) and sobel kernel size 3 */
	Canny(mat_frame, canny_frame, 50, 200, 3);

        //cvtColor(canny_frame, cdst, CV_GRAY2BGR);
        //cvtColor(mat_frame, gray, CV_BGR2GRAY);

	/* Apply probabilistic hough transform */
        HoughLinesP(canny_frame, lines, 1, CV_PI/180, 50, 50, 10);

	/* Display the result by drawing lines detected by hough transform */
        for( size_t i = 0; i < lines.size(); i++ )
        {
          Vec4i l = lines[i];
          line(mat_frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
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
