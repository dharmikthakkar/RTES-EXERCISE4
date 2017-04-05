/*
 *
 *  Example by Sam Siewert 
 * 
 *  Source : OpenCV Documentation
 *    
 *  The code demonstrates the use of OpenCV function Canny to detect edges in an image.
 * 
 *  Canny Edge Detection Algorithm - 
 *  1. Low-Pass Filtering: 
 *     As a pre-processing step, low-pass filtering is done to filter out any noise. A 
 *     gaussian filter of kernel size >= 3 is used for this purpose.
 *  2. Intensity Gradient Computation:
 *     The intensity gradient of the image is computed.
 *     a. 2-D Sobel mask Sx and Sy (x and y direction) is applied to compute horizontal
 *        and vertical gradient Gx and Gy.
 *     b. The gradient strength and direction is computed using Gx and Gy. The direction 
 *        computed is rounded to one of the four possible angles (0, 45, 90 or 135). 
 *  3. Non-maximum suppression:
 *     Remove any pixels that aren't considered as part of the edge. After this stage, 
 *     only most probable true edges will remain.
 *  4. Hysterisis (Thresholding):
 *     Canny edge detection algorithm uses two thresholds : lower and upper
 *     a. If a pixel gradient is higher than the upper threshold, the pixel is accepted 
 *        as an edge.
 *     b. If a pixel gradient value is below the lower threshold, then it is rejected.
 *     c. If the pixel gradient is between the two thresholds, then it will be accepted 
 *        only if it is connected to a pixel that is above the upper threshold.  
 * 
 *  Code Flow - 
 *  1. Create a named window that can be used as a placeholder for images and trackbars.
 *     Create a trackbar for the user to enter lower threshold for the canny edge detector.
 *  2. Create a capture device. If user has specified one as part of command line 
 *     argument, it will be used. Else, a default one with camera index 0 will be 
 *     used. Set capture window attributes, height and width.  
 *  3. Start capturing from capture device. Grab a frame to work with.
 *  4. Convert the frame to grayscale using OpenCV's cvtColor function. 
 *  5. Apply a Gaussian blur to filter out any noise. 
 *  6. Detect the edges in the image using the Canny edge detector.
 *  7. Display the result of Canny edge detector with edges highlighted.
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

#define HRES 640	//Horizontal resolution i.e. width
#define VRES 480	//Vertical resolution i.e. height

// Transform display window
char timg_window_name[] = "Edge Detector Transform";

int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
Mat canny_frame, cdst, timg_gray, timg_grad;

IplImage* frame;

/* Trackbar callback function */

void CannyThreshold(int, void*)
{
    Mat mat_frame(frame);

    cvtColor(mat_frame, timg_gray, CV_RGB2GRAY);	//Converts RGB image to gray scale image

    /// Reduce noise with a kernel 3x3
    blur( timg_gray, canny_frame, Size(3,3) );	

    /// Canny detector
    Canny( canny_frame, canny_frame, lowThreshold, lowThreshold*ratio, kernel_size );

    /// Using Canny's output as a mask, we display our result
    timg_grad = Scalar::all(0); //Fills the timg_grad matrix with all 0s

    mat_frame.copyTo( timg_grad, canny_frame);	// use copyTo to map only the areas of the image that are identified as edges

    imshow( timg_window_name, timg_grad );	//displays the timg_grad image

}


int main( int argc, char** argv )
{
    CvCapture* capture;
    int dev=0;

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
    
    // Creates a window
    namedWindow( timg_window_name, CV_WINDOW_AUTOSIZE );
    // Create a Trackbar for user to enter threshold
    createTrackbar( "Min Threshold:", timg_window_name, &lowThreshold, max_lowThreshold, CannyThreshold );
    /* Each time the Trackbar registers an action, the callback function CannyThreshold will be invoked. */  
  
    //Initializes capturing frames from the camera device
    capture = (CvCapture *)cvCreateCameraCapture(dev);
    // Sets width and height for the capture
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);

    while(1)
    {
	//Grabs, decompresses and returns the video frame
        frame=cvQueryFrame(capture);
        if(!frame) break;

	//Perform Canny edge detection and displays the output image
        CannyThreshold(0, 0);

	//Press q to quit
        char q = cvWaitKey(33);
        if( q == 'q' )
        {
            printf("got quit\n"); 
            break;
        }
    }
    // Releases the CvCapture structure
    cvReleaseCapture(&capture);
    
    // Destroy created window
    cvDestroyWindow(timg_window_name);
    
};
