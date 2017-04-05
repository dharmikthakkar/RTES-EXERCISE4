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

#include <sys/time.h>
#include <time.h>
using namespace cv;
using namespace std;

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

int main( int argc, char** argv )
{
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    CvCapture* capture = cvCreateCameraCapture(0);
    IplImage* frame;

    while(1)
    {
        frame=cvQueryFrame(capture);
	temp_time=readTOD();     
        if(!frame) break;

        cvShowImage("Capture Example", frame);
	printf("Image @ %lf\n", temp_time);
        char c = cvWaitKey(33);
        if( c == 27 ) break;
    }

    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
};
