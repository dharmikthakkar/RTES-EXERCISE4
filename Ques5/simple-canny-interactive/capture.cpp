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
#include <opencv2/imgproc/imgproc.hpp>

#include <sys/time.h>
#include <time.h>
#include <bits/stdc++.h>

using namespace cv;
using namespace std;

#ifdef RES1
#define HRES 640        //Horizontal resolution i.e. width
#define VRES 480        //Vertical resolution i.e. height
#endif

#ifdef RES2
#define HRES 320	//Horizontal resolution i.e. width
#define VRES 240	//Vertical resolution i.e. height
#endif

#ifdef RES3
#define HRES 160        //Horizontal resolution i.e. width
#define VRES 120        //Vertical resolution i.e. height
#endif

// Transform display window
char timg_window_name[] = "Edge Detector Transform";

#ifdef CANNY
int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
Mat canny_frame, cdst, timg_gray, timg_grad;
#endif

#ifdef HOUGH
int dev=0;
Mat gray, canny_frame, cdst;
vector<Vec4i> lines;
#endif

#ifdef HOUGH_E
int dev=0;
Mat gray;
vector<Vec3f> circles;
#endif

IplImage* frame;
struct timeval tv;
char str1[200];
int temp=0;
double temp_time=0, prev_frame_time=0;
double ave_framedt=0.0, ave_frame_rate=0.0, fc=0.0, framedt=0.0;

double readTOD(void)
{
    double ft=0.0;
    if ( gettimeofday(&tv, NULL) != 0 )
    {
        perror("readTOD");
        return 0.0;
    }
    else
    {
        ft = ((double)(((double)tv.tv_sec) + (((double)tv.tv_usec) /1000000.0)));
   	ft = ft*1000;
    }
    return ft;
}
void display_timestamp(){
    if(temp > 2)
	{
	    fc=(double)temp;
	    ave_framedt=((fc-1.0)*ave_framedt + framedt)/fc;
	    ave_frame_rate=1.0/(ave_framedt/1000.0);
	}
     framedt=temp_time - prev_frame_time;
     prev_frame_time=temp_time;
     printf("Frame @ %u sec, %lu usec, dt=%5.2lf msec, avedt=%5.2lf msec, rate=%5.2lf fps\n", 
           (unsigned)tv.tv_sec, 
           (unsigned long)tv.tv_usec,
           framedt, ave_framedt, ave_frame_rate);
}
#ifdef CANNY
/* Trackbar callback function */
void CannyThreshold(int, void*)
{
    temp++;
    Mat mat_frame(frame);

    cvtColor(mat_frame, timg_gray, CV_RGB2GRAY);	//Converts RGB image to gray scale image

    /// Reduce noise with a kernel 3x3
    blur( timg_gray, canny_frame, Size(3,3) );	

    /// Canny detector
    Canny( canny_frame, canny_frame, lowThreshold, lowThreshold*ratio, kernel_size );

    /// Using Canny's output as a mask, we display our result
    timg_grad = Scalar::all(0); //Fills the timg_grad matrix with all 0s

    mat_frame.copyTo( timg_grad, canny_frame);	//copies canny_frame to timg_grad


/*
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);


    sprintf(str1, "Image_%llu.png", (unsigned long long)readTOD());   
    try {
        imwrite(str1, timg_grad, compression_params);
    }
    catch (runtime_error& ex) {
        fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
       // return 1;
    }
*/
    imshow( timg_window_name, timg_grad );	//displays the timg_grad image
    display_timestamp();
}
#endif

int main( int argc, char** argv )
{
    CvCapture* capture;
    int dev=0;
    double start=0, stop=0;
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
   // Create a Trackbar for user to enter threshold
    #ifdef CANNY
     // Creates a window
    namedWindow( timg_window_name, CV_WINDOW_AUTOSIZE );
    createTrackbar( "Min Threshold:", timg_window_name, &lowThreshold, max_lowThreshold, CannyThreshold );
    #endif
    //Initializes  capturing frames from the camera device
    capture = (CvCapture *)cvCreateCameraCapture(dev);
    // Sets width and height for the capture
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);
    start = readTOD();
    stop = readTOD();
    while(1)
    {
	//Grabs, decompresses and returns the video frame
        frame=cvQueryFrame(capture);
        if(!frame) break;
        temp_time=readTOD();
	#ifdef CANNY
	//Perform Canny edge detection and displays the output image
        CannyThreshold(0, 0);
	#endif

	#ifdef HOUGH
	temp++;
	Mat mat_frame(cvarrToMat(frame));
        Canny(mat_frame, canny_frame, 50, 200, 3);

        cvtColor(canny_frame, cdst, CV_GRAY2BGR);
        cvtColor(mat_frame, gray, CV_BGR2GRAY);

        HoughLinesP(canny_frame, lines, 1, CV_PI/180, 50, 50, 10);

        for( size_t i = 0; i < lines.size(); i++ )
        {
          Vec4i l = lines[i];
          line(mat_frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
        }

/*     
        //if(!frame) break;
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(9);


        // cvShowImage seems to be a problem in 3.1
        //cvShowImage("Capture Example", frame);
	 sprintf(str1, "Image_%llu.png", (unsigned long long)readTOD());   
	 try {
		imwrite(str1, mat_frame, compression_params);
	 }
	 catch (runtime_error& ex) {
		fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
	       // return 1;
	 }
*/	 
	imshow("Capture Example", mat_frame);
	display_timestamp();
	#endif

	#ifdef HOUGH_E
        temp++;
	Mat mat_frame(cvarrToMat(frame));
        
        // Does not work in OpenCV 3.1
        //cvtColor(mat_frame, gray, CV_BGR2GRAY);
        cvtColor(mat_frame, gray, COLOR_BGR2GRAY);

        GaussianBlur(gray, gray, Size(9,9), 2, 2);

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
/*
        // Does not work in OpenCV 3.1
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(9);

       //cvShowImage("Capture Example", frame);
	 sprintf(str1, "Image_%llu.png", (unsigned long long)readTOD());   
	 try {
		imwrite(str1, mat_frame, compression_params);
	 }
	 catch (runtime_error& ex) {
		fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
	       // return 1;
	 }
*/	
        imshow("Capture Example", mat_frame);
        display_timestamp();
	#endif



	stop=readTOD();
	//Press q to quit
        char q = cvWaitKey(33);
        if(temp >= 100)
        {
            printf("got quit\n"); 
            break;
        }
    }
    // Releases the CvCapture structure
    cvReleaseCapture(&capture);
    printf("transform called %d times with resolution %d x %d\n\n", temp, HRES, VRES);
};
