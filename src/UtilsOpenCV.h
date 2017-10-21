/* ----------------------------------------------------------------------------
 * Copyright 2017, Massachusetts Institute of Technology,
 * Cambridge, MA 02139
 * All Rights Reserved
 * Authors: Luca Carlone, et al. (see THANKS for the full author list)
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file   UtilsOpenC.h
 * @brief  Utilities to interface GTSAM with OpenCV
 * @author Luca Carlone
 */

#ifndef UtilsOpenCV_H_
#define UtilsOpenCV_H_

#include <stdlib.h>
#include <opengv/point_cloud/methods.hpp>
#include <gtsam/geometry/Cal3DS2.h>
#include <gtsam/geometry/Cal3_S2.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/navigation/ImuBias.h>
#include <opencv2/core/core.hpp>
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/opencv.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <sys/time.h>

namespace VIO {

// Scalars
using size_t  = std::size_t;
using int64_t = std::int64_t;
using Timestamp = std::int64_t;
using uint8_t = std::uint8_t;

// Typedefs of commonly used Eigen matrices and vectors.
using Point2 = gtsam::Point2;
using Point3 = gtsam::Point3;
using Vector3 = gtsam::Vector3;
using Vector6 = gtsam::Vector6;
using Matrix3 = gtsam::Matrix33;
using Matrix6 = gtsam::Matrix66;
using Matrices3 = std::vector<gtsam::Matrix3, Eigen::aligned_allocator<gtsam::Matrix3>>;
using Vectors3 = std::vector<Vector3, Eigen::aligned_allocator<Vector3>>;

// float version
using Vector3f = Eigen::Matrix<float, 3, 1>;
using Vector6f = Eigen::Matrix<float, 6, 1>;
using Matrix3f = Eigen::Matrix<float, 3, 3>;
using Matrix6f = Eigen::Matrix<float, 6, 6>;
using Matrixf = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>;
using Matrices3f = std::vector<Matrix3f, Eigen::aligned_allocator<Matrix3f>>;
using Vectors3f = std::vector<Vector3f, Eigen::aligned_allocator<Vector3f>>;

enum Kstatus {
  VALID, NO_LEFT_RECT, NO_RIGHT_RECT, NO_DEPTH, FAILED_ARUN
};

// Definitions relevant to frame types
using FrameId = int;
using LandmarkId = int;
using LandmarkIds = std::vector<LandmarkId>;
using KeypointCV = cv::Point2f;
using KeypointsCV = std::vector<KeypointCV>;
using StatusKeypointCV = std::pair<Kstatus,KeypointCV>;
using StatusKeypointsCV = std::vector<StatusKeypointCV>;
using BearingVectors = std::vector<Vector3, Eigen::aligned_allocator<Vector3>>;

class UtilsOpenCV {

public:
  /* ----------------------------------------------------------------------------- */
  // Open files with name output_filename, and checks that it is valid
  static void OpenFile(const std::string output_filename,
      std::ofstream& outputFile){
    outputFile.open(output_filename.c_str()); outputFile.precision(20);
    if (!outputFile.is_open()){
      std::cout << "Cannot open file: " << output_filename << std::endl;
      throw std::runtime_error("OpenFile: cannot open the file!!!");
    }
  }
  /* ------------------------------------------------------------------------ */
  // compares 2 cv::Mat
  static bool CvMatCmp(const cv::Mat mat1, const cv::Mat mat2, const double tol = 1e-7){
    // treat two empty mat as identical as well
    if (mat1.empty() && mat2.empty()) {
      std::cout << "CvMatCmp: asked comparison of 2 empty matrices" << std::endl;
      return true;
    }
    // if dimensionality of two mats are not identical, these two mats are not identical
    if (mat1.cols != mat2.cols || mat1.rows != mat2.rows || mat1.dims != mat2.dims) {
      return false;
    }

    // Compare the two matrices!
    cv::Mat diff = mat1 - mat2;
    return cv::checkRange(diff, true, 0, -tol, tol);
  }
  /* ------------------------------------------------------------------------ */
  // comparse 2 cvPoints
  static bool CvPointCmp(const cv::Point2f &p1, const cv::Point2f &p2, const double tol = 1e-7) {
    return std::abs(p1.x - p2.x) <= tol && std::abs(p1.y - p2.y) <= tol;
  }
  /* ----------------------------------------------------------------------------- */
  // converts a vector of 16 elements listing the elements of a 4x4 3D pose matrix by rows
  // into a pose3 in gtsam
  static gtsam::Pose3 Vec2pose(const std::vector<double> vecRows, const int n_rows, const int n_cols){
    if(n_rows!=4 || n_cols!=4)
      throw std::runtime_error("Vec2pose: wrong dimension!");

    gtsam::Matrix T_BS_mat(n_rows,n_cols); // allocation
    int idx = 0;
    for (int r = 0; r < n_rows; r++) {
      for (int c = 0; c < n_cols; c++) {
        T_BS_mat(r, c) = vecRows[idx];
        idx++;
      }
    }
    return gtsam::Pose3(T_BS_mat);
  }
  /* ----------------------------------------------------------------------------- */
  // Converts a gtsam pose3 to a 3x3 rotation matrix and translation vector
  // in opencv format (note: the function only extracts R and t, without changing them)
  static std::pair<cv::Mat,cv::Mat> Pose2cvmats(const gtsam::Pose3 pose){
    gtsam::Matrix3 rot = pose.rotation().matrix();
    cv::Mat R = cv::Mat(3,3,CV_64F);
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 3; c++) {
        R.at<double>(r, c) = rot(r,c);
      }
    }
    gtsam::Vector3 tran = pose.translation();
    cv::Mat T = cv::Mat(3,1,CV_64F);
    for (int r = 0; r < 3; r++)
      T.at<double>(r,0) = tran(r);

    return std::make_pair(R,T);
  }
  /* ----------------------------------------------------------------------------- */
  // Converts a rotation matrix and translation vector from opencv to gtsam pose3
  static gtsam::Pose3 Cvmats2pose(const cv::Mat& R, const cv::Mat& T){
    gtsam::Matrix poseMat = gtsam::Matrix::Identity(4,4);
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 3; c++) {
        poseMat(r,c) = R.at<double>(r, c);
      }
    }
    for (int r = 0; r < 3; r++)
      poseMat(r,3) = T.at<double>(r,0);

    return gtsam::Pose3(poseMat);
  }
  /* ----------------------------------------------------------------------------- */
  // Converts a 3x3 rotation matrix from opencv to gtsam Rot3
  static gtsam::Rot3 Cvmat2rot(const cv::Mat& R){
    gtsam::Matrix rotMat = gtsam::Matrix::Identity(3,3);
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 3; c++) {
        rotMat(r,c) = R.at<double>(r, c);
      }
    }
    return gtsam::Rot3(rotMat);
  }
  /* ----------------------------------------------------------------------------- */
  // Converts a camera matrix from opencv to gtsam::Cal3_S2
  static gtsam::Cal3_S2 Cvmat2Cal3_S2(const cv::Mat& M){
    double fx = M.at<double>(0,0);
    double fy = M.at<double>(1,1);
    double s = M.at<double>(0,1);
    double u0 = M.at<double>(0,2);
    double v0 = M.at<double>(1,2);
    return gtsam::Cal3_S2(fx, fy, s, u0, v0);
  }
  /* ----------------------------------------------------------------------------- */
  // Converts a camera matrix from opencv to gtsam::Cal3_S2
  static cv::Mat Cal3_S2ToCvmat (const gtsam::Cal3_S2& M){
    cv::Mat C = cv::Mat::eye(3, 3, CV_64F);
    C.at<double>(0, 0) = M.fx();
    C.at<double>(1, 1) = M.fy();
    C.at<double>(0, 1) = M.skew();
    C.at<double>(0, 2) = M.px();
    C.at<double>(1, 2) = M.py();
    return C;
  }
  /* ----------------------------------------------------------------------------- */
  // converts an opengv transformation (3x4 [R t] matrix) to a gtsam::Pose3
  static gtsam::Pose3 Gvtrans2pose(const opengv::transformation_t RT){
    gtsam::Matrix poseMat = gtsam::Matrix::Identity(4,4);
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 4; c++) {
        poseMat(r,c) = RT(r,c);
      }
    }
    return gtsam::Pose3(poseMat);
  }
  /* ----------------------------------------------------------------------------- */
  // Crops pixel coordinates avoiding that it falls outside image
  static cv::Point2f CropToSize(cv::Point2f px, cv::Size size){
    cv::Point2f px_cropped = px;
    px_cropped.x = std::min(px_cropped.x, float(size.width-1));
    px_cropped.x = std::max(px_cropped.x, float(0.0));
    px_cropped.y = std::min(px_cropped.y, float(size.height-1));
    px_cropped.y = std::max(px_cropped.y, float(0.0));
    return px_cropped;
  }
  /* ----------------------------------------------------------------------------- */
  // crop to size and round pixel coordinates to integers
  static cv::Point2f RoundAndCropToSize(cv::Point2f px, cv::Size size){
    cv::Point2f px_cropped = px;
    px_cropped.x = round(px_cropped.x);
    px_cropped.y = round(px_cropped.y);
    return CropToSize(px_cropped,size);
  }
  /* --------------------------------------------------------------------------------------- */
  // get good features to track from image (wrapper for opencv goodFeaturesToTrack)
  static std::vector<cv::Point2f> ExtractCorners(cv::Mat img, const double qualityLevel = 0.01, const double minDistance = 10,
      const int blockSize = 3, const double k = 0.04, const int maxCorners = 100, const bool useHarrisDetector = false) {

    cv::TermCriteria criteria = cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);
    // Extract the corners
    std::vector<cv::Point2f> corners;

    try{
      cv::goodFeaturesToTrack(img, corners, 100, qualityLevel,
          minDistance, cv::noArray(), blockSize, useHarrisDetector, k);

      cv::Size winSize = cv::Size(10, 10);
      cv::Size zeroZone = cv::Size(-1, -1);
      cv::cornerSubPix(img, corners, winSize, zeroZone, criteria);
    }catch(...){
      std::cout << "ExtractCorners: no corner found in image" << std::endl;
      // corners remains empty
    }
    return corners;
  }
  template<typename T> struct myGreaterThanPtr
  {  bool operator()(const std::pair< const T* , T > a,
      const std::pair< const T* , T > b) const { return *(a.first) > *(b.first); } };
  /* --------------------------------------------------------------------------------------- */
  // get good features to track from image (wrapper for opencv goodFeaturesToTrack)
  static std::pair< std::vector<cv::Point2f> , std::vector<double> >
  MyGoodFeaturesToTrackSubPix(cv::Mat image,
      int maxCorners, double qualityLevel, double minDistance,
      cv::Mat mask, int blockSize,
      bool useHarrisDetector, double harrisK ){

    // outputs:
    std::vector<double> scores;
    std::vector<cv::Point2f> corners;

    try{
      cv::Mat eig, tmp;
      double maxVal = 0; double minVal; cv::Point minLoc; cv::Point maxLoc;
      // get eigenvalue image & get peak
      if( useHarrisDetector )
    	  cv::cornerHarris( image, eig, blockSize, 3, harrisK );
      else
    	  cv::cornerMinEigenVal( image, eig, blockSize, 3 );

      // cut off corners below quality level
      cv::minMaxLoc( eig, &minVal, &maxVal, &minLoc, &maxLoc, mask );
      cv::threshold( eig, eig, maxVal*qualityLevel, 0, CV_THRESH_TOZERO ); // cut stuff below quality
      cv::dilate( eig, tmp, cv::Mat());

      cv::Size imgsize = image.size();

      // create corners
      std::vector< std::pair< const float* , float > > tmpCornersScores;

      // collect list of pointers to features - put them into temporary image
      for( int y = 1; y < imgsize.height - 1; y++ )
      {
        const float* eig_data = (const float*)eig.ptr(y);
        const float* tmp_data = (const float*)tmp.ptr(y);
        const uchar* mask_data = mask.data ? mask.ptr(y) : 0;

        for( int x = 1; x < imgsize.width - 1; x++ )
        {
          float val = eig_data[x];
          if( val != 0 && val == tmp_data[x] && (!mask_data || mask_data[x]) ){
            tmpCornersScores.push_back( std::make_pair( eig_data + x, val) );
          }
        }
      }

      std::sort( tmpCornersScores.begin(), tmpCornersScores.end(), myGreaterThanPtr<float>() );

      // put sorted corner in other struct
      size_t i, j, total = tmpCornersScores.size(), ncorners = 0;

      if(minDistance >= 1)
      {
        // Partition the image into larger grids
        int w = image.cols;
        int h = image.rows;

        const int cell_size = cvRound(minDistance);
        const int grid_width = (w + cell_size - 1) / cell_size;
        const int grid_height = (h + cell_size - 1) / cell_size;

        std::vector<std::vector<cv::Point2f> > grid(grid_width*grid_height);

        minDistance *= minDistance;

        for( i = 0; i < total; i++ )
        {
          int ofs = (int)((const uchar*)tmpCornersScores[i].first - eig.data);
          int y = (int)(ofs / eig.step);
          int x = (int)((ofs - y*eig.step)/sizeof(float));
          double eigVal = double( tmpCornersScores[i].second );

          bool good = true;

          int x_cell = x / cell_size;
          int y_cell = y / cell_size;

          int x1 = x_cell - 1;
          int y1 = y_cell - 1;
          int x2 = x_cell + 1;
          int y2 = y_cell + 1;

          // boundary check
          x1 = std::max(0, x1);
          y1 = std::max(0, y1);
          x2 = std::min(grid_width-1, x2);
          y2 = std::min(grid_height-1, y2);

          for( int yy = y1; yy <= y2; yy++ )
          {
            for( int xx = x1; xx <= x2; xx++ )
            {
              std::vector <cv::Point2f> &m = grid[yy*grid_width + xx];

              if( m.size() )
              {
                for(j = 0; j < m.size(); j++)
                {
                  float dx = x - m[j].x;
                  float dy = y - m[j].y;

                  if( dx*dx + dy*dy < minDistance )
                  {
                    good = false;
                    goto break_out;
                  }
                }
              }
            }
          }

          break_out:

          if(good)
          {
            // printf("%d: %d %d -> %d %d, %d, %d -- %d %d %d %d, %d %d, c=%d\n",
            //    i,x, y, x_cell, y_cell, (int)minDistance, cell_size,x1,y1,x2,y2, grid_width,grid_height,c);
            grid[y_cell*grid_width + x_cell].push_back(cv::Point2f((float)x, (float)y));
            corners.push_back(cv::Point2f((float)x, (float)y));
            scores.push_back(eigVal);
            ++ncorners;
            if( maxCorners > 0 && (int)ncorners == maxCorners )
              break;
          }
        }
      }
      else
      {
        for( i = 0; i < total; i++ )
        {
          int ofs = (int)((const uchar*)tmpCornersScores[i].first - eig.data);
          int y = (int)(ofs / eig.step);
          int x = (int)((ofs - y*eig.step)/sizeof(float));
          double eigVal = double( tmpCornersScores[i].second );
          corners.push_back(cv::Point2f((float)x, (float)y));
          scores.push_back(eigVal);
          ++ncorners;
          if( maxCorners > 0 && (int)ncorners == maxCorners )
            break;
        }
      }

      // subpixel accuracy: TODO: create function for the next 4 lines
      cv::TermCriteria criteria = cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);
      cv::Size winSize = cv::Size(10, 10);
      cv::Size zeroZone = cv::Size(-1, -1);

      cv::cornerSubPix(image, corners, winSize, zeroZone, criteria);
    }catch(...){
      std::cout << "ExtractCorners: no corner found in image" << std::endl;
      // corners remains empty
    }
    return std::make_pair(corners, scores);
  }
  /* --------------------------------------------------------------------------------------- */
  // rounds entries in a unit3, such that largest entry is saturated to +/-1 and the other become 0
  static gtsam::Unit3 RoundUnit3(const gtsam::Unit3 x){

    gtsam::Vector3 x_vect_round = gtsam::Vector3::Zero();
    gtsam::Vector3 x_vect = x.unitVector();
    double max_x = (x_vect.cwiseAbs()).maxCoeff(); // max absolute value
    for(size_t i=0;i<3;i++){
      if( fabs( fabs(x_vect(i)) - max_x) < 1e-4){ // found max element
        x_vect_round(i) = x_vect(i) / max_x; // can be either -1 or +1
        break; // tie breaker for the case in which multiple elements attain the max
      }
    }
    return gtsam::Unit3(x_vect_round);
  }
  /* --------------------------------------------------------------------------------------- */
  // rounds number to a specified number of decimal digits
  // (digits specifies the number of digits to keep AFTER the decimal point)
  static double RoundToDigit(const double x, const int digits = 2){
    double dec = pow(10,digits); // 10^digits
    double y = double( round(x * dec) ) / dec;
    return y;
  }
  /* --------------------------------------------------------------------------------------- */
  // converts doulbe to sting with desired number of digits (total number of digits)
  static std::string To_string_with_precision(const double a_value, const int n = 3)
  {
    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
  }
  /* --------------------------------------------------------------------------------------- */
  // converts time from nanoseconds to seconds
  static double NsecToSec(const std::int64_t timestamp)
  {
    return double(timestamp) * 1e-9;
  }
  /* --------------------------------------------------------------------------------------- */
  // (NOT TESTED): converts time from seconds to nanoseconds
  static std::int64_t SecToNsec(const double timeInSec)
  {
    return double(timeInSec * 1e9);
  }
  /* --------------------------------------------------------------------------------------- */
  // (NOT TESTED): get current time in seconds
  static double GetTimeInSeconds()
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t time_usec = tv.tv_sec * (int64_t)1e6 + tv.tv_usec;
    return ((double) time_usec * 1e-6);
  }
  /* --------------------------------------------------------------------------------------- */
  // given two gtsam::Pose3 computes the relative rotation and translation errors: rotError,tranError
  static std::pair<double,double> ComputeRotationAndTranslationErrors(
      const gtsam::Pose3 expectedPose, const gtsam::Pose3 actualPose, const bool upToScale = false){
    // compute errors
    gtsam::Rot3 rotErrorMat = (expectedPose.rotation()).between(actualPose.rotation());
    gtsam::Vector3 rotErrorVector = gtsam::Rot3::Logmap(rotErrorMat);
    double rotError = rotErrorVector.norm();

    gtsam::Vector3 actualTranslation = actualPose.translation().vector();
    gtsam::Vector3 expectedTranslation = expectedPose.translation().vector();
    if(upToScale){
      double normExpected = expectedTranslation.norm();
      double normActual = actualTranslation.norm();
      if(normActual > 1e-5)
        actualTranslation = normExpected * actualTranslation / normActual; // we manually add the scale here
    }
    gtsam::Vector3 tranErrorVector = expectedTranslation - actualTranslation;
    double tranError = tranErrorVector.norm();
    return std::make_pair(rotError,tranError);
  }
  /* ----------------------------------------------------------------------------- */
  // reads image and converts to 1 channel image
  static cv::Mat ReadAndConvertToGrayScale(const std::string img_name, bool const equalize = false)
  {
    cv::Mat img = cv::imread(img_name, cv::IMREAD_ANYCOLOR);
    if (img.channels() > 1)
      cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
    if(equalize){ // Apply Histogram Equalization
      std::cout << "- Histogram Equalization for image: " << img_name << std::endl;
      cv::equalizeHist(img, img);
    }
    return img;
  }
  /* ----------------------------------------------------------------------------- */
  // reorder block entries of covariance from state: [bias, vel, pose] to [pose vel bias]
  static gtsam::Matrix Covariance_bvx2xvb(const gtsam::Matrix COV_bvx)
  {
    gtsam::Matrix cov_xvb = COV_bvx;
    // fix diagonals: poses
    cov_xvb.block<6,6>(0,0) = COV_bvx.block<6,6>(9,9);
    // fix diagonals: velocity: already in place
    // fix diagonals: biases
    cov_xvb.block<6,6>(9,9) = COV_bvx.block<6,6>(0,0);

    // off diagonal, pose-vel
    cov_xvb.block<6,3>(0,6) = COV_bvx.block<6,3>(9,6);
    cov_xvb.block<3,6>(6,0) = (cov_xvb.block<6,3>(0,6)).transpose();
    // off diagonal, pose-bias
    cov_xvb.block<6,6>(0,9) = COV_bvx.block<6,6>(9,0);
    cov_xvb.block<6,6>(9,0) = (cov_xvb.block<6,6>(0,9)).transpose();
    // off diagonal, vel-bias
    cov_xvb.block<3,6>(6,9) = COV_bvx.block<3,6>(6,0);
    cov_xvb.block<6,3>(9,6) = (cov_xvb.block<3,6>(6,9)).transpose();

    return cov_xvb;
  }
  /* --------------------------------------------------------------------------------------- */
  static void PlainMatchTemplate( const cv::Mat stripe, const cv::Mat templ, cv::Mat& result ){
    int result_cols =  stripe.cols - templ.cols + 1;
    int result_rows = stripe.rows - templ.rows + 1;

    result.create( result_rows, result_cols, CV_32FC1 );
    float diffSq = 0, tempSq = 0, stripeSq = 0;
    for(int ii=0; ii<templ.rows;ii++){
      for(int jj=0; jj<templ.cols;jj++){
        tempSq += pow((int) templ.at<uchar>(ii,jj),2);
        // std::cout << " templ.at<double>(ii,jj) " << (int) templ.at<uchar>(ii,jj) << std::endl;
      }
    }
    for(size_t i=0; i<result_rows;i++){
      for(size_t j=0; j<result_cols;j++){
        diffSq = 0; stripeSq = 0;

        for(int ii=0; ii<templ.rows;ii++){
          for(int jj=0; jj<templ.cols;jj++){
            diffSq += pow( (int) templ.at<uchar>(ii,jj) - (int)  stripe.at<uchar>(i+ii,j+jj), 2);
            stripeSq += pow( (int)  stripe.at<uchar>(i+ii,j+jj) , 2);
          }
        }
        result.at<float>(i,j) = diffSq / sqrt(tempSq * stripeSq);
      }
    }
  }

  /* ----------------------------------------------------------------------------- */
  // add circles in the image at desired position/size/color
  static void DrawCirclesInPlace(cv::Mat& img, const std::vector<cv::Point2f> imagePoints,
      const cv::Scalar color = cv::Scalar(0, 255, 0),
      const double msize = 3,
      const std::vector<int> pointIds = std::vector<int>(),
      const int remId = 1e9){

    cv::Point2f textOffset = cv::Point2f(-10,-5); // text offset
    if (img.channels() < 3)
      cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
    for (size_t i = 0; i < imagePoints.size(); i++){
      cv::circle(img, imagePoints[i], msize, color, 2);
      if(pointIds.size() == imagePoints.size()) // we also have text
        cv::putText(img, std::to_string(pointIds[i] % remId),
            imagePoints[i] + textOffset, CV_FONT_HERSHEY_COMPLEX, 0.5, color);
    }
  }
  /* ----------------------------------------------------------------------------- */
  // add squares in the image at desired position/size/color
  static void DrawSquaresInPlace(cv::Mat& img, const std::vector<cv::Point2f> imagePoints,
      const cv::Scalar color = cv::Scalar(0, 255, 0),
      const double msize = 10, const
      std::vector<int> pointIds = std::vector<int>(),
      const int remId = 1e9){

    cv::Point2f textOffset = cv::Point2f(-10,-5); // text offset
    if (img.channels() < 3)
      cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
    for (size_t i = 0; i < imagePoints.size(); i++){
      cv::Rect square = cv::Rect(imagePoints[i].x-msize/2, imagePoints[i].y-msize/2, msize, msize);
      rectangle( img, square, color, 2);
      if(pointIds.size() == imagePoints.size()) // we also have text
        cv::putText(img, std::to_string(pointIds[i] % remId),
            imagePoints[i] + textOffset, CV_FONT_HERSHEY_COMPLEX, 0.5, color);
    }
  }
  /* ----------------------------------------------------------------------------- */
  // add x in the image at desired position/size/color
  static void DrawCrossesInPlace(cv::Mat& img, const std::vector<cv::Point2f> imagePoints,
      const cv::Scalar color = cv::Scalar(0, 255, 0),
      const double msize = 0.2,
      const std::vector<int> pointIds = std::vector<int>(),
      const int remId = 1e9){

    cv::Point2f textOffset = cv::Point2f(-10,-5); // text offset
    cv::Point2f textOffsetToCenter = cv::Point2f(-3,+3); // text offset
    if (img.channels() < 3)
      cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
    for (size_t i = 0; i < imagePoints.size(); i++){
      cv::putText(img, "X", imagePoints[i] + textOffsetToCenter, CV_FONT_HERSHEY_COMPLEX, msize, color, 2);
      if(pointIds.size() == imagePoints.size()) // we also have text
        cv::putText(img, std::to_string(pointIds[i] % remId),
            imagePoints[i] + textOffset, CV_FONT_HERSHEY_COMPLEX, 0.5, color);
    }
  }
  /* ----------------------------------------------------------------------------- */
  // add text (vector of doubles) in the image at desired position/size/color
  static void DrawTextInPlace(cv::Mat& img, const std::vector<cv::Point2f> imagePoints,
      const cv::Scalar color = cv::Scalar(0, 255, 0),
      const double msize = 0.4,
      const std::vector<double> textDoubles = std::vector<double>()){
    cv::Point2f textOffset = cv::Point2f(-12,-5); // text offset
    if (img.channels() < 3)
      cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
    for (size_t i = 0; i < imagePoints.size(); i++){
      if(imagePoints.size() == textDoubles.size()) // write text
        cv::putText(img, To_string_with_precision(textDoubles.at(i), 3),
            imagePoints[i] + textOffset, CV_FONT_HERSHEY_COMPLEX, msize, color);
    }
  }
  /* ----------------------------------------------------------------------------- */
  // concatenate two images and return results as a new mat
  static cv::Mat ConcatenateTwoImages(const cv::Mat imL_in, const cv::Mat imR_in) {
    cv::Mat imL = imL_in.clone();
    if (imL.channels() == 1){
      cv::cvtColor(imL, imL, cv::COLOR_GRAY2BGR);
    }
    cv::Mat imR = imR_in.clone();
    if (imR.channels() == 1){
      cv::cvtColor(imR, imR, cv::COLOR_GRAY2BGR);
    }
    cv::Size szL = imL.size();
    cv::Size szR = imR.size();
    cv::Mat originalLR(szL.height, szL.width+szR.width, CV_8UC3);
    cv::Mat left(originalLR, cv::Rect(0, 0, szL.width, szL.height));
    imL.copyTo(left);
    cv::Mat right(originalLR, cv::Rect(szL.width, 0, szR.width, szR.height));
    imR.copyTo(right);
    return originalLR;
  }
  /* --------------------------------------------------------------------------------------- */
  // draw corner matches and return results as a new mat
  static cv::Mat DrawCornersMatches(const cv::Mat img1, const std::vector<cv::Point2f> &corners1,
      const cv::Mat img2, const std::vector<cv::Point2f> &corners2,
      const std::vector<cv::DMatch> matches, const bool randomColor = false)
  {
    cv::Mat canvas = UtilsOpenCV::ConcatenateTwoImages(img1, img2);
    cv::Point2f ptOffset = cv::Point2f(img1.cols, 0);
    cv::RNG rng(12345);
    for (int i = 0; i < matches.size(); i++) {
      cv::Scalar color;
      if(randomColor)
        color = cv::Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255));
      else
        color =  cv::Scalar(0, 255, 0);
      cv::line(canvas, corners1[matches[i].queryIdx], corners2[matches[i].trainIdx] + ptOffset, color);
      cv::circle(canvas, corners1[matches[i].queryIdx], 3, color, 2);
      cv::circle(canvas, corners2[matches[i].trainIdx] + ptOffset, 3, color, 2);
    }
    return canvas;
  }
  /* --------------------------------------------------------------------------------------- */
  static cv::Mat DrawCircles(const cv::Mat img, const StatusKeypointsCV imagePoints,
      const std::vector<double> circleSizes = std::vector<double>())
  {
    KeypointsCV valid_imagePoints;
    std::vector<cv::Scalar> circleColors;
    for(size_t i=0; i<imagePoints.size(); i++){
      if(imagePoints[i].first == Kstatus::VALID){ // it is a valid point!
        valid_imagePoints.push_back(imagePoints[i].second);
        circleColors.push_back(cv::Scalar(0, 255, 0)); // green
      }
      else if(imagePoints[i].first == Kstatus::NO_RIGHT_RECT){ // template matching did not pass
        valid_imagePoints.push_back(imagePoints[i].second);
        circleColors.push_back(cv::Scalar(0, 0, 255));
      }
      else{
        valid_imagePoints.push_back(imagePoints[i].second); // disparity turned out negative
        circleColors.push_back(cv::Scalar(0, 0, 255));
      }
    }
    return UtilsOpenCV::DrawCircles(img, valid_imagePoints, circleColors, circleSizes);
  }
  /* --------------------------------------------------------------------------------------- */
  static cv::Mat DrawCircles(const cv::Mat img, const KeypointsCV imagePoints,
      const std::vector<cv::Scalar> circleColors = std::vector<cv::Scalar>(),
      const std::vector<double> circleSizes = std::vector<double>())
  {
    bool displayWithSize = false; // if true size of circles is proportional to depth
    bool displayWithText = true; // if true display text with depth
    KeypointCV textOffset = KeypointCV(-10,-5); // text offset
    cv::Mat img_color = img.clone();
    if (img_color.channels() < 3) {
      cv::cvtColor(img_color, img_color, cv::COLOR_GRAY2BGR);
    }
    for (size_t i = 0; i < imagePoints.size(); i++)
    {
      double circleSize = 3;
      cv::Scalar circleColor = cv::Scalar(0, 255, 0);

      if (displayWithSize && circleSizes.size() == imagePoints.size())
        circleSize = 5 * std::max(circleSizes[i],0.5);

      if(circleColors.size() == imagePoints.size())
        circleColor = circleColors[i];

      cv::circle(img_color, imagePoints[i], circleSize, circleColor, 2);

      if(displayWithText && circleSizes.size() == imagePoints.size() && circleSizes[i] != -1){
        cv::putText(img_color, UtilsOpenCV::To_string_with_precision(circleSizes[i]),
            imagePoints[i] + textOffset, CV_FONT_HERSHEY_COMPLEX, 0.4, circleColor);
      }
    }
    return img_color;
  }
  /* --------------------------------------------------------------------------------------- */
  static void DrawCornersMatchesOneByOne(const cv::Mat img1, const std::vector<cv::Point2f> &corners1,
      const cv::Mat img2, const std::vector<cv::Point2f> &corners2,
      const std::vector<cv::DMatch> matches)
  {
    cv::Mat canvas = UtilsOpenCV::ConcatenateTwoImages(img1, img2);
    cv::Point2f ptOffset = cv::Point2f(img1.cols, 0);

    for (int i = 0; i < matches.size(); i++) {
      cv::Mat baseCanvas = canvas.clone();
      printf("Match %d\n", i);
      cv::line(baseCanvas, corners1[matches[i].queryIdx],
          corners2[matches[i].trainIdx] + ptOffset,
          cv::Scalar(0, 255, 0));
      cv::imshow("Match one by one", baseCanvas);
      cv::waitKey(0);
    }
  }
  /* ----------------------------------------------------------------------------- */
  //  print standard vector with header:
  template< typename T >
  static void PrintVector(const std::vector<T> vect,const std::string vectorName){
    std::cout << vectorName << std::endl;
    for(auto si : vect) std::cout << " " << si;
    std::cout << std::endl;
  }
  /* ----------------------------------------------------------------------------- */
  //  sort vector and remove duplicate elements
  template< typename T >
  static void VectorUnique(std::vector<T>& v){
    // e.g.: std::vector<int> v{1,2,3,1,2,3,3,4,5,4,5,6,7};
    std::sort(v.begin(), v.end()); // 1 1 2 2 3 3 3 4 4 5 5 6 7
    auto last = std::unique(v.begin(), v.end());
    // v now holds {1 2 3 4 5 6 7 x x x x x x}, where 'x' is indeterminate
    v.erase(last, v.end());
  }
  /* ----------------------------------------------------------------------------- */
  //  find max absolute value of matrix entry
  static double MaxAbsValue(gtsam::Matrix M){
    double maxVal = 0;
    for(size_t i=0; i < M.rows(); i++){
      for(size_t j=0; j < M.cols(); j++){
        maxVal = std::max(maxVal,fabs(M(i,j)));
      }
    }
    return maxVal;
  }
};

} // namespace VIO
#endif /* UtilsOpenCV_H_ */

