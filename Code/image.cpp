#pragma once
#include"image.h"
#include "v3.h"
#include "ppc.h"
#include"m33.h"
#include"ERI.h"

#include <C:\opencv\build\include\opencv2/opencv.hpp>
#include <C:\opencv\build\include\opencv2\core\core.hpp>
#include <C:\opencv\build\include\opencv2\highgui\highgui.hpp>


int Is_MInv_calculated;
M33 M_Inv;

int upload_image(string path, Mat &image) {
	Mat image_temp;
	string path_temp = path;
	image_temp = imread(path_temp.c_str(), IMREAD_COLOR);
	if (!image_temp.data)                              // Check for invalid input
	{
		cout << "Could not open or find the image" << std::endl;
		return -1;
	}

	image = image_temp;


	return 0;
}

int ERI2Conv(Mat &source_image_mat, Mat &output_image_mat, ERI eri_image, PPC camera1) {



	for (int i = 0; i < source_image_mat.rows; ++i)
	{
		for (int j = 0; j < source_image_mat.cols; ++j)
		{

			V3 p = eri_image.Unproject(i, j);
			V3 pp;
			if (!camera1.Project(p, pp))
				continue;

			if (pp[0] < camera1.w && pp[0] >= 0 && pp[1] >= 0 && pp[1] < camera1.h)
			{
				output_image_mat.at<cv::Vec3b>((int)pp[1], (int)pp[0]) = source_image_mat.at<cv::Vec3b>(i, j);  //pp[0]=column
				//source_image_mat.at<cv::Vec3b>(i, j) = 0;
			}
		}
		cout << "row: " << i << "     \r";
	}

	return 0;
}


int Conv2ERI(Mat &source_image_mat, Mat &output_image_mat, ERI eri_image, PPC camera1)
{
	int source_W, source_H;
	source_W = source_image_mat.cols;
	source_H = source_image_mat.rows;

	for (int i = 0; i < source_image_mat.rows; ++i)
	{
		for (int j = 0; j < source_image_mat.cols; ++j)
		{


			V3 p = camera1.UnprojectPixel(j, i, 100);
			//cout << "I,J: " << i << ","<<j<<" "<<"p:"<<p<<endl;   // there may be some issue here with i and j
		//	int Inew = eri_image.Lat2PixI(eri_image.GetXYZ2Latitude(p), source_H);
			//int Jnew = eri_image.Lon2PixJ(eri_image.GetXYZ2Longitude(p), source_W);
			//output_image_mat.at<cv::Vec3b>(Inew, Jnew) = source_image_mat.at<cv::Vec3b>(i, j);
			//source_image_mat.at<cv::Vec3b>(i, j) = 0;

		}
	}

	return 0;

}


int EachPixelConv2ERI(ERI eri_image, PPC camera1,int u, int v, int &pixelI, int &pixelJ)
{			
			
			V3 p = camera1.UnprojectPixel(0.5f+u,0.5f+v, camera1.GetFocalLength());		
			p = p.UnitVector();
			pixelI = eri_image.Lat2PixI(eri_image.GetXYZ2Latitude(p));
			pixelJ = eri_image.Lon2PixJ(eri_image.GetXYZ2Longitude(p));
			//cout << "x,y->" << x << "," << y << " pixX,Y->" << pixelX << "," << pixelY << endl;

			return 0;
}


int ERI2Conv_back_mapped(Mat &source_image_mat, Mat &output_image_mat, ERI eri_image, PPC camera1) 
{

	float pixelX, pixelY = 0;

	for (int i = 0; i < output_image_mat.rows; ++i)
	{
		for (int j = 0; j < output_image_mat.cols; ++j)
		{
			//cout << "(I,J->)" << i << "," << j << endl;
			//EachPixelConv2ERI(eri_image, camera1, i, j, pixelX, pixelY, source_image_mat.cols, source_image_mat.rows);
			//output_image_mat.at<cv::Vec3b>(i, j) = source_image_mat.at<cv::Vec3b>(pixelX, pixelY);  
			//source_image_mat.at<cv::Vec3b>(pixelX, pixelY) = 0;
			
		}
		
	}

	return 0;
}

int ERI2Conv_back_mapped_v2(Mat &source_image_mat, Mat &output_image_mat, ERI eri_image, PPC camera1)
{
	int pixelI, pixelJ = 0;

	for (int v = 0; v < camera1.h; v++)
	{
		for (int u = 0; u < camera1.w; u++)
		{
			
			EachPixelConv2ERI(eri_image, camera1, u, v, pixelI, pixelJ);
			output_image_mat.at<cv::Vec3b>(v, u) = source_image_mat.at<cv::Vec3b>(pixelI, pixelJ);			
					
		}

	}

	return 0;
}


/*

int ERI2Conv_bothway_test(Mat &source_image_mat, Mat &output_image_mat, ERI eri_image, PPC camera1)
{
	float pixelX, pixelY = 0;
	
	for (int i = 0; i < 20; ++i)
	{
		for (int j = 0; j < 20; ++j)
		{
			
			V3 p = eri_image.Unproject(i, j);
			V3 pp;
			camera1.Project(p, pp);
			
			
			EachPixelConv2ERI(eri_image, camera1, pp[1], pp[0], pixelX, pixelY, source_image_mat.cols, source_image_mat.rows);
			cout << i<<","<< j<<"," <<pp[1] << "," << pp[0] << "," << pixelX << "," << pixelY << endl;
				
		}		

	}

	return 0;
}

int ERI2Conv_bothway_test2(Mat &source_image_mat, Mat &output_image_mat, ERI eri_image, PPC camera1)
{
	float pixelX, pixelY = 0;

	for (int i = 0; i < 20; ++i)
	{
		for (int j = 0; j < 20; ++j)
		{
			EachPixelConv2ERI(eri_image, camera1, i,j, pixelX, pixelY, source_image_mat.cols, source_image_mat.rows);
			V3 p = eri_image.Unproject(pixelX, pixelY);
			V3 pp;
			camera1.Project(p, pp);			
			cout << i << "," << j << "," << pixelX << "," << pixelY <<","<< pp[1] << "," << pp[0] << endl;

		}

	}

	return 0;
}
*/