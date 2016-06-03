#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <iostream>
#include "bsgmm.hpp"
using namespace std;
NODEPTR pixelGMMBuffer, pixelPtr;
static int height, width;
NODE Create_Node()
{
  NODE tmp;
  tmp.GMMCount = 1;
  return tmp;
}

gaussian Create_gaussian( double r, double g, double b, double variance, double weight )
{
  gaussian tmp;
  tmp.mean[0] = r;
  tmp.mean[1] = g;
  tmp.mean[2] = b;
  tmp.variance = variance;
  tmp.weight = weight;
  return tmp;
}

void freeMem()
{
  delete[] pixelGMMBuffer;
}

int main( int argc, char *argv[] )
{
  cv::Mat inputImg, outputImg;
  cv::VideoCapture capture( argv[1] );
  if ( !capture.read( inputImg ) )
  {
    std::cout << " Can't recieve input from source ";
    exit( EXIT_FAILURE );
  }
  cv::cvtColor( inputImg, inputImg, CV_BGR2YCrCb );
  outputImg = cv::Mat( inputImg.rows, inputImg.cols, CV_8U, cv::Scalar( 0 ) );
  width = inputImg.rows;
  height = inputImg.cols;
  uchar *inputPtr;
  uchar *outputPtr;
  pixelGMMBuffer = new NODE[inputImg.rows * inputImg.cols];
  for ( int i = 0; i < inputImg.rows; i++ )
  {
    inputPtr = inputImg.ptr( i );
    for ( int j = 0; j < inputImg.cols; j++ )
    {
      pixelGMMBuffer[i * inputImg.cols + j] = Create_Node();
      pixelGMMBuffer[i * inputImg.cols + j].arr[0] = Create_gaussian( *inputPtr, *( inputPtr + 1 ), *( inputPtr + 2 ), defaultVariance, 1.0 );
    }
  }
  capture.read( inputImg );
  outputImg = cv::Mat( inputImg.rows, inputImg.cols, CV_8UC1, cv::Scalar( 0 ) );
  while ( true )
  {
    if ( !capture.read( inputImg ) )
    {
      break;
    }
    pixelPtr = pixelGMMBuffer;
    inputPtr = inputImg.ptr( 0 );
    outputPtr = outputImg.ptr( 0 );
    for ( int j = 0; j < width * height; j ++ )
    {
      double MahalDis;
      double newVariance = 0.0;
      double var = 0.0;
      double totalWeight = 0.0;
      double close = false;
      double rVal = *( inputPtr++ );
      double gVal = *( inputPtr++ );
      double bVal = *( inputPtr++ );
      int tmpIndex;
      int background = 0;
      if ( pixelPtr->GMMCount > defaultGMMCount )
      {
        pixelPtr->GMMCount--;
      }
      for ( int k = 0; k < pixelPtr->GMMCount; k++ )
      {
        double weight = pixelPtr->arr[k].weight;
        weight = weight * alpha_bar + prune;
        if ( close == false )
        {
          double dR = rVal - pixelPtr->arr[k].mean[0];
          double dG = gVal - pixelPtr->arr[k].mean[1];
          double dB = bVal - pixelPtr->arr[k].mean[2];
          var = pixelPtr->arr[k].variance;
          MahalDis = ( dR * dR + dG * dG + dB * dB );
          if ( ( totalWeight < cfbar ) && ( MahalDis < 16.0 * var * var ) )
          {
            background = 255;
          }
          if ( MahalDis < 9.0 * var * var )
          {
            weight += alpha;
            close = true;
            double mult = alpha / weight;
            pixelPtr->arr[k].mean[0] += mult * dR;
            pixelPtr->arr[k].mean[1] += mult * dG;
            pixelPtr->arr[k].mean[2] += mult * dB;
            newVariance = var + mult * ( MahalDis - var );
            pixelPtr->arr[k].variance = newVariance < 5.0 ? 5.0 : ( newVariance > 20.0 ? 20.0 : newVariance );
            tmpIndex = k;
          }
        }
        if ( weight < -prune )
        {
          weight = 0;
          pixelPtr->GMMCount--;
        }
        totalWeight += weight;
        pixelPtr->arr[k].weight = weight;
      }
      if ( close == false )
      {
        tmpIndex = pixelPtr->GMMCount;
        pixelPtr->arr[pixelPtr->GMMCount++] = Create_gaussian( rVal, gVal, bVal, defaultVariance, alpha );
      }
      for ( int i = 0; i < pixelPtr->GMMCount; i++ )
      {
        pixelPtr->arr[i].weight /= totalWeight;
      }
      for ( int i = tmpIndex; i > 0; i-- )
      {
        if ( pixelPtr->arr[i].weight <= pixelPtr->arr[i - 1].weight )
        {
          break;
        }
        else
        {
          swap( pixelPtr->arr[i], pixelPtr->arr[i - 1] );
        }
      }
      *outputPtr++ = background;
      pixelPtr++;
    }
    cv::imshow( "video", inputImg );
    cv::imshow( "GMM", outputImg );
    if ( cv::waitKey( 1 ) > 0 )
    {
      break;
    }
  }
  freeMem();
  return EXIT_SUCCESS;
}
