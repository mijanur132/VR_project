#include"curl/curl.h"
#include"image.h"
#include<fstream>
#include<conio.h>
#include<string.h>
#include "config.h"
#include "ssim.h"

#include <future>
#include <mutex>
#include <thread>
#include <queue>

#include <C:\opencv\build\include\opencv2\opencv.hpp>
#include <C:\opencv\build\include\opencv2\core\core.hpp>
#include <C:\opencv\build\include\opencv2\highgui\highgui.hpp>

using namespace cv;
using namespace std;


std::string filename;
int Is_MInv_calculated;
M33 M_Inv;

//................................... http.................................//
string datax;
size_t download(char* buf, size_t size, size_t nmemb, void* userP)
{	//
	//variable data to be saved
	//size*nmemb is the size of the buf(fer)
	for (int c = 0; c < size*nmemb; c++)
	{
		datax.push_back(buf[c]);
	}
	return size * nmemb; //return the number of bytes we handled
}
string fetchTextOverHttp(char* addr)
{	//		//variable data to be saved
	curl_global_init(CURL_GLOBAL_ALL); //pretty simple
	CURL* conHandle = curl_easy_init(); //make an "easy" handle
		curl_easy_setopt(conHandle, CURLOPT_URL, addr);
	curl_easy_setopt(conHandle, CURLOPT_VERBOSE, 1L); //outputs status
	curl_easy_setopt(conHandle, CURLOPT_WRITEFUNCTION, &download); //set our callback to handle data
	curl_easy_perform(conHandle); //get the file
	return datax;
	//cout << datax << endl; //should output an html file (if the set url is valid)
}
//................................... http End.................................//


void thredCatchFrame(int MxchunkN, int chunkD, int &fps, std::queue<Mat> &frameQ)
{	
	int chunkN;
	Mat frame;
	for (int i = 1; i <= MxchunkN; i++)
	{
		cout << "here1" << endl;
		chunkN = i;
		string fn = "http://127.0.0.5:80/RollerInput_";
		std::ostringstream oss;
		oss << fn << (chunkN) << ".avi";
		string filename = oss.str();
		VideoCapture cap1(filename);
		if (!cap1.isOpened())
		{
			cout << "Cannot open the video file: " << endl;
			waitKey(100);
		}
		fps = cap1.get(CAP_PROP_FPS);
		cout << fps << endl;
		for (int j = 1; j <= chunkD * fps; j++)
		{

			cap1 >> frame;
			fps = cap1.get(CAP_PROP_FPS);
			if (!frame.empty())
			{
				cout << "frame pushed: " << j << endl;
				frameQ.push(frame.clone());
			}
			else
			{
				cout << "Cannot open the video file: " << endl;
				break;
			}
		}
		cap1.release();
	}
	cout << "All Frame Loaded" << endl;
	return;

}
 
void thredProcessFrame(PPC camera2, Path path1, int MxchunkN, int chunkD, int fps, int cf, struct samplingvar& svar, float var[10], std::queue<Mat> &frameQ, vector <Mat>& conv)
{
	cout << "here2" << endl;
	int segi = 0;
	int chunkN = 0;
	Mat ret1;
	int frameN = 0;
	int segref = 0;
	int condition = 1;
	while (condition)
	{
		// Get a next frame
		Mat frame;
		
		if (!frameQ.empty())
		{
			frame = frameQ.front();
			frameQ.pop();
			cout << "Frame queue not empty" << endl;

			// Process the frame
			if (!frame.empty())
			{
				frameN++;
				print("frameN: " << frameN << endl);
				segi = path1.GetCamIndex(frameN, fps, segi);
				chunkN = frameN % (fps*chunkD);
				if (chunkN == 0)
				{
					segref = segi;
					print("segref: " << segref << endl);
				}

				Mat heatmap3c = Mat::zeros(camera2.h, camera2.w, frame.type());
				Mat heatmap = Mat::zeros(camera2.h, camera2.w, DataType<double>::type);
				ret1 = path1.CRERI2Conv(frame, var, cf, path1.cams[segi], segref, heatmap, &svar);
				//conv.push_back(ret1);

				namedWindow("sample", WINDOW_NORMAL);
				resizeWindow("sample", 800, 600);
				imshow("sample", ret1);
				waitKey(30);
				if (frameN == chunkD * fps*MxchunkN)
				{
					condition = 0;
				}
			}
		}
	}
	int starting_frame = 0;
	int ending_frame = fps * chunkN*chunkD;
	filename = "./Video/encodingtest/newmethod/rollerh264convtemp";
	//videowriterhelperx(1, fps, ret1.cols, ret1.rows, starting_frame, ending_frame, conv);
	return;
}


void testDownloadVideoHttp()
{
	std::queue<Mat> frameQ;
	vector <Mat> conv;
	vector <Mat> hmap;
	vector<float> min;
	vector<float> avg;
	struct samplingvar svar;
	Mat ret1;
	Path path1;

	float hfov = 90.0f;
	float corePredictionMargin = 0.8;
	int w = 960;
	int h = 512;  //540 for perfect 2160 p but here we have 2048
	PPC camera2(hfov*corePredictionMargin, w*corePredictionMargin, h*corePredictionMargin);
	path1.LoadHMDTrackingData("./Video/roller2.txt", camera2);
	int lastframe = 1800;
	

	int cf = 5;
	float var[10];
	float x;
	string datax=fetchTextOverHttp("http://127.0.0.5:80/encoding_variable.txt");
	std::istringstream f(datax);
	std::string line;
	int i = 0;
	while (std::getline(f, line)) 
	{
		var[i] = stoi(line);
		cout << var[i] << '\n';
		i++;
	}

	int chunkD = 4;
	int chunkN;
	int fps=0;
	int MxchunkN = 4;

	//thredCatchFrame(MxchunkN, chunkD, fps, frameQ);
	auto futurex = std::async(std::launch::async, thredCatchFrame, MxchunkN, chunkD, std::ref(fps), std::ref(frameQ));		
	fps = 29;
	//auto futurex1 = std::async(std::launch::async, thredProcessFrame, camera2, path1, MxchunkN, chunkD, fps, cf, svar, var, frameQ, conv);
	
	//futurex1.get();	
	thredProcessFrame(camera2, path1, MxchunkN, chunkD, fps, cf, svar, var, frameQ, conv);
	futurex.get();
}

Mat diffimgage(Mat backgroundImage, Mat currentImage) {
	cv::Mat diffImage;
	cv::absdiff(backgroundImage, currentImage, diffImage);

	cv::Mat foregroundMask = cv::Mat::zeros(diffImage.rows, diffImage.cols, CV_8UC1);

	float threshold = 30.0f;
	float dist;

	for (int j = 0; j < diffImage.rows; ++j)
	{
		for (int i = 0; i < diffImage.cols; ++i)
		{
			cv::Vec3b pix = diffImage.at<cv::Vec3b>(j, i);

			dist = (pix[0] * pix[0] + pix[1] * pix[1] + pix[2] * pix[2]);
			dist = sqrt(dist);

			if (dist > threshold)
			{
				foregroundMask.at<unsigned char>(j, i) = 255;
			}
		}
	}

	return diffImage;
}

void getcheckerboard()
{	
	ERI eri(3840, 2160);	
	Mat convImage;
	upload_image("./Image/checkerbigcolor1.PNG", convImage);   //Image must be square size
	Mat eriMat(eri.h, eri.w, convImage.type());
	float fov[2];
	fov[0] = 90.0f;
	fov[1] = 90.0f;
	PPC camera1(fov, convImage.cols);
	camera1.PositionAndOrient(V3(0, 0, 0), V3(1, 0, 0), V3(0, 1, 0));
	
	camera1.Tilt(90);


	for (int i = 0; i < eri.h; i++)
	{
		for (int j = 0; j < eri.w; j++)
		{
			V3 p = eri.Unproject(i, j);
			V3 pp;
			if (!camera1.Project(p, pp))
				continue;
			if (pp[0] < convImage.cols && pp[0] >= 0 && pp[1] >= 0 && pp[1] < convImage.rows)
			{
				//print(pp[1] << " " << pp[0] << " " << i << " " << j<<" "<<camera1.h<<" "<<camera1.w << endl);
				eriMat.at<Vec3b>(i, j) = convImage.at<Vec3b>((int)pp[1], (int)pp[0]);
			}
		}

	}



	namedWindow("sample", WINDOW_NORMAL);
	resizeWindow("sample", 800, 400);
	imshow("sample", eriMat);	
	waitKey(1000);

	upload_image("./Image/checkerbigcolor2.PNG", convImage);   //Image must be square size
	camera1.PositionAndOrient(V3(0, 0, 0), V3(1, 0, 0), V3(0, 1, 0));

	for (int i = 0; i < eri.h; i++)
	{
		for (int j = 0; j < eri.w; j++)
		{
			V3 p = eri.Unproject(i, j);
			V3 pp;
			if (!camera1.Project(p, pp))
				continue;
			if (pp[0] < convImage.cols && pp[0] >= 0 && pp[1] >= 0 && pp[1] < convImage.rows)
			{
				//print(pp[1] << " " << pp[0] << " " << i << " " << j<<" "<<camera1.h<<" "<<camera1.w << endl);
				eriMat.at<Vec3b>(i, j) = convImage.at<Vec3b>((int)pp[1], (int)pp[0]);
			}
		}

	}

	imshow("sample", eriMat);	
	waitKey(1000);

	upload_image("./Image/checkerbigcolor3.PNG", convImage);   //Image must be square size
	camera1.PositionAndOrient(V3(0, 0, 0), V3(1, 0, 0), V3(0, 1, 0));
	camera1.Pan(90);


	for (int i = 0; i < eri.h; i++)
	{
		for (int j = 0; j < eri.w; j++)
		{
			V3 p = eri.Unproject(i, j);
			V3 pp;
			if (!camera1.Project(p, pp))
				continue;
	
			if (pp[0] < convImage.cols && pp[0] >= 0 && pp[1] >= 0 && pp[1] < convImage.rows)
			{
				//print(pp[1] << " " << pp[0] << " " << i << " " << j<<" "<<camera1.h<<" "<<camera1.w << endl);
				eriMat.at<Vec3b>(i, j) = convImage.at<Vec3b>((int)pp[1], (int)pp[0]);
			}
		}

	}
	camera1.PositionAndOrient(V3(0, 0, 0), V3(1, 0, 0), V3(0, 1, 0));
	camera1.Pan(180);

	imshow("sample", eriMat);	
	waitKey(1000);

	upload_image("./Image/checkerbigcolor4.PNG", convImage);   //Image must be square size

	for (int i = 0; i < eri.h; i++)
	{
		for (int j = 0; j < eri.w; j++)
		{
			V3 p = eri.Unproject(i, j);
			V3 pp;
			if (!camera1.Project(p, pp))
				continue;
			if (pp[0] < convImage.cols && pp[0] >= 0 && pp[1] >= 0 && pp[1] < convImage.rows)
			{
				//print(pp[1] << " " << pp[0] << " " << i << " " << j<<" "<<camera1.h<<" "<<camera1.w << endl);
				eriMat.at<Vec3b>(i, j) = convImage.at<Vec3b>((int)pp[1], (int)pp[0]);
			}
		}

	}

	camera1.PositionAndOrient(V3(0, 0, 0), V3(1, 0, 0), V3(0, 1, 0));
	camera1.Pan(270);

	imshow("sample", eriMat);	
	waitKey(1000);
	upload_image("./Image/checkerbigcolor5.PNG", convImage);   //Image must be square size

	for (int i = 0; i < eri.h; i++)
	{
		for (int j = 0; j < eri.w; j++)
		{
			V3 p = eri.Unproject(i, j);
			V3 pp;
			if (!camera1.Project(p, pp))
				continue;
			if (pp[0] < convImage.cols && pp[0] >= 0 && pp[1] >= 0 && pp[1] < convImage.rows)
			{
				//print(pp[1] << " " << pp[0] << " " << i << " " << j<<" "<<camera1.h<<" "<<camera1.w << endl);
				eriMat.at<Vec3b>(i, j) = convImage.at<Vec3b>((int)pp[1], (int)pp[0]);
			}
		}

	}
	camera1.PositionAndOrient(V3(0, 0, 0), V3(1, 0, 0), V3(0, 1, 0));
	camera1.Tilt(-90);

	upload_image("./Image/checkerbigcolor6.PNG", convImage);   //Image must be square size

	for (int i = 0; i < eri.h; i++)
	{
		for (int j = 0; j < eri.w; j++)
		{
			V3 p = eri.Unproject(i, j);
			V3 pp;
			if (!camera1.Project(p, pp))
				continue;
			if (pp[0] < convImage.cols && pp[0] >= 0 && pp[1] >= 0 && pp[1] < convImage.rows)
			{
				//print(pp[1] << " " << pp[0] << " " << i << " " << j<<" "<<camera1.h<<" "<<camera1.w << endl);
				eriMat.at<Vec3b>(i, j) = convImage.at<Vec3b>((int)pp[1], (int)pp[0]);
			}
		}

	}

	
	imshow("sample", eriMat);
	img_write("./Image/checkerbigERI.PNG", eriMat);
	waitKey(10000);

}



void testforwardbackward()
{
	Mat eriPixels; 
	upload_image(IMAGE, eriPixels);  
	ERI eri(eriPixels.cols, eriPixels.rows); 
	int cfov=90;
	PPC camera1(cfov, cameraW, cameraH); 
	Mat convPixels = Mat::zeros(cameraH, cameraW, eriPixels.type()); 
	Mat convPixelsreverse = Mat::zeros(cameraH, cameraW, eriPixels.type()); 

	//camera1.Tilt(120);
	eri.ERI2Conv(eriPixels, convPixels, camera1);


	namedWindow("sample", WINDOW_NORMAL);
	resizeWindow("sample", 800, 400);
	imshow("sample", convPixels);


	//img_write("./Image/CONV_image.png", output_image_mat);// write an image
	eri.Conv2ERI(convPixels, convPixelsreverse, eriPixels, camera1);
	namedWindow("sample1", WINDOW_NORMAL);
	resizeWindow("sample1", 800, 400);

	imshow("sample1", convPixelsreverse);	
	waitKey(10000);
	
	
}

void img_write(const char *s1, cv::InputArray s2) {

	vector<int> compression_params;
	compression_params.push_back(IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(9);
	string str1 ="\""+string(s1)+ "\"";
	imwrite(s1, s2, compression_params);
	waitKey(1);
}



void playstillmanually()
{
	Mat eriPixels; 
		upload_image(IMAGE, eriPixels);  
		ERI eri(eriPixels.cols, eriPixels.rows); 
		
		PPC camera1(70.0f, 90.0f, 500); 
		Mat convPixels = Mat::zeros(cameraH, cameraW, eriPixels.type()); 
		
		
	for (int i = 0; i < 1500; i++)
	{
		namedWindow("sample", WINDOW_NORMAL);
		resizeWindow("sample", 800, 400);

		camera1.Pan(10);
		cout << i * 10 << endl;
		eri.ERI2Conv(eriPixels, convPixels, camera1);
		cout << camera1.w << " " << camera1.h << endl;
		cout << convPixels.cols << " " << convPixels.rows << endl;
		//imshow("sample", convPixels);
		waitKey(1000);
		

	}
}
//*/



void check_interpolation() {
	
	PPC camera1(cFoV, cameraW, cameraH);
	PPC camera2(cFoV, cameraW, cameraH);
	camera1.Pan(90.0f);

	Mat source_image_mat;
	upload_image(IMAGE, source_image_mat);  //this function upload image of equirect form

	Mat output_image_mat = cv::Mat::zeros(cameraH, cameraW, source_image_mat.type());
	Mat output_image_mat_1 = cv::Mat::zeros(cameraH, cameraW, source_image_mat.type());
	ERI eri_image(source_image_mat.cols, source_image_mat.rows);
	eri_image.ERI2Conv(source_image_mat, output_image_mat, camera1);
	imshow("CONV_image", output_image_mat);
	waitKey(100);
	eri_image.ERI2Conv(source_image_mat, output_image_mat_1, camera2);
	imshow("CONV_image1", output_image_mat_1);
	waitKey(100);

	for (int i = 0; i < NUM_INTERP_frameN; i++)
	{
		
		//cout << i << endl;
		PPC interPPC;
		interPPC.SetInterpolated(&camera1,&camera2, i, NUM_INTERP_frameN);		
		eri_image.ERI2Conv(source_image_mat, output_image_mat, interPPC);
		imshow("CONV_imagex", output_image_mat);
		waitKey(10);
		
	}
	
}



int testPlayBackHMDPathStillImage()
{	
	Mat eriPixels; 
	char fname[] = "./Image/RollerCoasterFrame3to1.jpg";
	upload_image(fname, eriPixels);
	ERI eri(eriPixels.cols, eriPixels.rows);
	Path path1;
	PPC camera1(cFoV, cameraW, cameraH);
	Mat convPixels = cv::Mat::zeros(cameraH, cameraW, eriPixels.type());
	path1.LoadHMDTrackingData("./Video/roller.txt", camera1);
	path1.PlayBackPathStillImage(eriPixels, eri, convPixels);	
	return 0;

}



int testPlayBackHMDPathVideo()
{
	
	float hfov = 90.0f;
	float corePredictionMargin = 1;
	int w = 960;
	int h = 540;
	PPC camera1(hfov*corePredictionMargin, w*corePredictionMargin, h*corePredictionMargin);
	Mat convPixels = cv::Mat::zeros(camera1.h, camera1.w, IMAGE_TYPE);	
	Path path1;
	int lastframe =1500;
	path1.LoadHMDTrackingData("./Video/roller.txt", camera1);
	path1.PlayBackPathVideo("./Video/roller.mkv", convPixels,lastframe);
	//path1.PlayBackPathVideo("http://127.0.0.5:80/input1.avi", convPixels, lastframe);
	return 0;

}

int testPlayBackHMDPathVideoPixelInterval()
{
	PPC camera1(cFoV, cameraW, cameraH);
	Mat convPixels = cv::Mat::zeros(cameraH, cameraW, IMAGE_TYPE);
	Path path1;
	int lastframe = 1500;
	path1.LoadHMDTrackingData("./Video/roller.txt", camera1);
	path1.PlayBackPathVideoPixelInterval("./Video/roller_2000_1000.mp4", convPixels, lastframe);
	return 0;

}

int testViewDirectionAvgRotation() 
{
		PPC camera1(cFoV, cameraW, cameraH);
		Path path1;
		path1.LoadHMDTrackingData("./Video/diving.txt", camera1);
		path1.VDrotationAvg();
		return 0;
}


int testWriteh264() {
	Path path1;
	int lastframe = 1000;
	int codec = VideoWriter::fourcc('H', '2', '6', '4');
	path1.WriteH264("./Video/roller.mkv", lastframe, codec);
	path1.WriteH264("./Video/paris.mkv", lastframe,codec);
	path1.WriteH264("./Video/ny.mkv", lastframe, codec);
	path1.WriteH264("./Video/diving_original.mkv", lastframe, codec);

	return 0;
}



int testWriteh264tiles() {
	Path path1;
	int lastframe = 1000;	
	int codec = VideoWriter::fourcc('H', '2', '6', '4');
	path1.WriteH264tiles("./Video/roller.mkv", lastframe, 6, 4, codec);	
	path1.WriteH264tiles("./Video/paris.mkv", lastframe, 6, 4, codec);
	path1.WriteH264tiles("./Video/ny.mkv", lastframe, 6, 4, codec);
	path1.WriteH264tiles("./Video/diving_original.mkv", lastframe, 6, 4, codec);

	
	path1.WriteH264tiles("./Video/roller.mkv", lastframe, 30, 30, codec);
	path1.WriteH264tiles("./Video/paris.mkv", lastframe, 30, 30, codec);
	path1.WriteH264tiles("./Video/ny.mkv", lastframe,30, 30, codec);
	path1.WriteH264tiles("./Video/diving_original.mkv", lastframe, 30, 30, codec);
	
	
	return 0;

}

void testBoundingBox()
{
	
	Path path1;
	int lastFrame = 100;
	PPC camera1(110.0f, 800, 400);
	//cout << camera1.GetVD() << endl;
	path1.LoadHMDTrackingData("./Video/roller.txt", camera1);
	path1.DrawBoundinigBoxTemp("./video/roller.mkv", lastFrame);

}
	
void testbilinear()
{
	Path path1;
	Mat frame;	
	upload_image("./Image/check_rb5x5.png", frame);
	Mat outframe(10*frame.rows, 10*frame.cols, frame.type());

	for (int i = 0; i < outframe.rows; i++)
	{
		for (int j = 0; j < outframe.cols; j++)
		{
			float fi;
			float fj;
			fi = (float)(i * frame.rows)/ (float)outframe.rows;
			fj = (float)(j * frame.cols) / (float)outframe.cols;
			path1.bilinearinterpolation(outframe, frame, i, j, fi, fj);
			//outframe.at<Vec3b>(i, j) = frame.at<Vec3b>((int) fi, (int)fj);
		}

	}

	namedWindow("sample", WINDOW_NORMAL);
	resizeWindow("sample",800,800);
	imshow("sample", outframe(Rect(outframe.rows/2-50,outframe.cols / 2 - 50,100,100)));
	waitKey(10000);

}

void getssim()
{
	int block_size = 10;
	compute_quality_metrics("./Image/test_conv_real.PNG", "./Image/test_conv_nl1.PNG", block_size);
	compute_quality_metrics("./Image/test_conv_real.PNG", "./Image/test_conv_ln1.PNG", block_size);

}

void testEncodingDecoding()
{
	int compressionfactor =10;
	Path path1;	
	Mat frame;
	Mat retencode;
	Mat retdecode;
	upload_image("./Image/checkerbig.PNG", frame);

	
	float hfov = 90.0f;
	float corePredictionMargin = 1;
	int w = 960;
	int h = 540;
	PPC corePPC(hfov*corePredictionMargin, w*corePredictionMargin , h*corePredictionMargin);
	PPC encodeRefPPC = corePPC;  //always next to corePPC before pan or tilt
	//corePPC.Pan(50.0f);
	//corePPC.Tilt(10.0f);

	PPC userPPC(hfov, w, h);
	userPPC.Pan(25.0f);
	userPPC.Tilt(20.0f);

	struct var encodevar;
	ERI eri(frame.cols,frame.rows);
	Vec3b linecolor(0, 0, 255);
	//eri.ERI2ConvDrawBorderinERI(frame, corePPC, linecolor);
	Vec3b linecolor1(0, 100, 255);
	//eri.ERI2ConvDrawBorderinERI(frame, userPPC, linecolor1);
	namedWindow("sample", WINDOW_NORMAL);
	resizeWindow("sample", 640, 360);
	imshow("sample", frame);	
	waitKey(1000);
	img_write("./Image/test_original.PNG", frame);
	
	retencode = path1.EncodeNewNonLinV2(frame, &encodevar, corePPC, encodeRefPPC, compressionfactor);	
	//retencode = path1.EncodeLinear(frame, &encodevar, corePPC, compressionfactor);

	img_write("./Image/test_encoded.PNG", retencode);	
	imshow("sample", retencode);
	waitKey(100);
	
	
	float var[10];
	var[0] = frame.cols;
	var[1] = frame.rows;
	var[2] = encodevar.We;
	var[3] = encodevar.Het;//*/	

	
	retdecode = path1.DecodeNewNonLinV2(retencode, var, compressionfactor, corePPC);
	//retdecode = path1.DecodeLinear(retencode, var, compressionfactor, corePPC);
	
	
	imshow("sample",retdecode);
	img_write("./Image/test_decoded.PNG",retdecode);
	waitKey(100);

	//Mat diff = diffimgage(frame, retdecode);
	//imshow("sample", diff);
	//img_write("./Image/test_diff.PNG", diff);
	//waitKey(100);

	Mat convPixels(userPPC.h, userPPC.w, frame.type());

	eri.ERI2Convtemp(retdecode, convPixels, userPPC);
	imshow("sample", convPixels);
	waitKey(100);
	img_write("./Image/test_conv.PNG", convPixels);
/*
	eri.ERI2Conv(frame, convPixels, userPPC);
	imshow("sample", convPixels);
	waitKey(100);
	img_write("./Image/test_conv_real.PNG", convPixels);

*/

}

void testvideoEncodeNew4s(int chunDurationsec) {
	vector <Mat> encoded;	
	struct var encodevar;
	Path path1;


	float hfov = 90.0f;
	float corePredictionMargin = 1;
	int w = 960;
	int h = 512;
	PPC corePPC(hfov*corePredictionMargin, w*corePredictionMargin, h*corePredictionMargin);
	PPC encodeRefPPC(hfov*corePredictionMargin, w*corePredictionMargin, h*corePredictionMargin);  //always next to corePPC before pan or tilt
	   	
	int cf = 5;

	path1.LoadHMDTrackingData("./Video/roller.txt", corePPC);
	VideoCapture cap("./Video/roller.MKV");
	if (!cap.isOpened())
	{
		cout << "Cannot open the video file: " << endl;
		waitKey(100000);
	}
	int fps = cap.get(CAP_PROP_FPS);
	int lastframe = 1000;	
	int segi = 0;
	Mat ret; 
	float var[10];
	
	for (int fi = 0; fi < lastframe; fi++)
	{
		print("fi: "<<fi<<endl);
		Mat frame;
		cap >> frame;
		if (frame.empty())
		{
			cout << fi << endl;
			cout << "Can not read video frame: " << endl;
			break;
		}
		if (fi % (chunDurationsec*fps) == 0)
		{
			segi = path1.GetCamIndex(fi, fps, segi);
			print("segi: "<< segi << endl);
		}

		ret = path1.EncodeNewNonLinV2(frame, &encodevar, path1.cams[segi], encodeRefPPC, cf);
		encoded.push_back(ret);
		if ((fi + 1) % (chunDurationsec*fps) == 0 && fi > 0)		
		{
			int chunkN = (fi + 1) / (chunDurationsec*fps);
			cout << chunkN << endl;
			int sf = chunDurationsec * fps*(chunkN - 1);
			int ef = chunDurationsec * fps*chunkN;
			filename = "./Video/encodingtest/newmethod/RollerInput_";
			cout << fps << ret.cols << ret.rows <<" sf: "<< sf<<" ef: " << ef << endl;
			
			videowriterhelperx(chunkN, fps, ret.cols, ret.rows, sf, ef, encoded);
		}
		if (fi == 2)
		{			
			var[0] = encodevar.colN;
			var[1] = encodevar.rowN;
			var[2] = encodevar.We;
			var[3] = encodevar.Het;			
		}
		
	}  

	ofstream output("./Video/encodingtest/newmethod/Roller_encoding_variable.txt");
	output << var[0] << endl;
	output << var[1] << endl;
	output << var[2] << endl;
	output << var[3] << endl;
	output.close();
	

}



void videowriterhelperx(int chunkN ,int fps,int cols, int rows, int starting_frame,int ending_frame, vector<Mat> file2wr )
{
	
	
	std::ostringstream oss1;
	oss1 << filename << chunkN << ".avi";
	string ofilename = oss1.str();
	VideoWriter writer1;
	int codec = VideoWriter::fourcc('H', '2', '6', '4');
	writer1.set(VIDEOWRITER_PROP_QUALITY, 10);
	cout << "Writing videofile: " << ofilename << codec << endl;	
	writer1.open(ofilename, codec, fps, Size(cols, rows), true);

	if (!writer1.isOpened())
	{
		cerr << "Could not open the output video file for write\n";
		return;
	}
	for (int i = starting_frame; i < ending_frame; i++)
	{
		writer1.write(file2wr[i]);
	}
	writer1.release();


}


void testrotationxyframe()
{
	Mat frame;
	upload_image("./Image/360_equirectangular_800_400.JPG", frame);
	if (!frame.data)                              // Check for invalid input
	{
		cout << "Could not open or find the image" << std::endl;
		return;
	}


	float hfov = 110.0f;
	PPC camera(hfov, 800, 400);

	Path path1;
	
	Mat newERI = Mat::zeros(frame.rows, frame.cols, frame.type());

	V3 p = camera.GetVD();
	camera.Pan(0.0f);
	camera.Tilt(45.0f);
	V3 p1 = camera.GetVD();

	// build local coordinate system of RERI
	V3 xaxis = camera.a.UnitVector();
	V3 yaxis = camera.b.UnitVector()*-1.0f;
	V3 zaxis = xaxis ^ yaxis;
	M33 reriCS;
	reriCS[0] = xaxis;
	reriCS[1] = yaxis;
	reriCS[2] = zaxis;


	path1.RotateXYaxisERI2RERI(frame, newERI, p, p1, reriCS);	
	

	namedWindow("sample", WINDOW_NORMAL);
	resizeWindow("sample", 400, 400);

	imshow("sample", newERI);
	waitKey(10000);
	

}



void testMousecontrol()
{
	ERI_INIT;
	int cond = 1;
	while (cond == 1)
	{

		eri.ERI2Conv(eriPixels, convPixels, camera1);
		imshow("CONV_image", convPixels);
		waitKey(1);

		int ch = getch();
		cout << ch << endl;
		switch (ch)
		{
		case 77:
			camera1.Pan(5.0f);
			break;
		case 75:
			camera1.Pan(-5.0f);
			break;
		case 72:
			camera1.Tilt(5.0f);
			break;
		case 80:
			camera1.Tilt(-5.0f);
			break;
		case 113:
			cond = 0;
			break;

		}
	}

}



/***************************************/
/**************Additional code*************************/

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




void testvideodecodeNcompare()
{
	vector <Mat> conv;
	vector <Mat> hmap;
	vector<float> min;
	vector<float> avg;
	struct samplingvar svar;
	Mat ret1;
	Path path1;
	PPC camera2(120.0f, 800, 400);
	path1.LoadHMDTrackingData("./Video/roller.txt", camera2);
	int lastframe = 1800;
	int segi = 0;

	int cf = 5;
	float var[10];
	float x;
	ifstream varfile("./Video/encodingtest/encoding_variable.txt");

	if (varfile.is_open())
	{
		int i = 0;
		while (varfile >> x)
		{
			var[i] = (int)x;
			cout << var[i] << '\n';
			i++;
		}
		varfile.close();
	}

	else cout << "Unable to open file";

	int chunkD = 4;
	int chunkN;
	int fps;
	int MxchunkN = 4;
	for (int i = 1; i <= MxchunkN; i++)
	{
		chunkN = i;
		string fn = "http://127.0.0.5:80/input";
		std::ostringstream oss;
		oss << fn << (chunkN) << ".avi";
		string filename = oss.str();

		VideoCapture cap1(filename);
		if (!cap1.isOpened())
		{
			cout << "Cannot open the video file: " << endl;
			waitKey(100000);
		}
		fps = cap1.get(CAP_PROP_FPS);
		int segref = 0;

		for (int j = fps * (chunkN - 1)*chunkD; j < fps*chunkN*chunkD; j++)
		{

			Mat encodframe;
			cap1 >> encodframe;
			if (encodframe.empty())
			{
				cout << j << endl;
				cout << "Can not read video frame: " << endl;
				break;
			}
			segi = path1.GetCamIndex(j, fps, segi);
			print("chunN: " << i << " frame: " << j << " segi: " << segi << endl;);
			if (j%fps == 0)
			{
				segref = segi;
				print("segref: " << segref << endl);
			}

			V3 p = path1.cams[segi].GetVD() - path1.cams[segref].GetVD();
			//print(p << " prev: "<< path1.cams[segi].GetVD()<<endl<<endl);
			p = V3(0, 0, -1) + p;
			p = p.UnitVector();

			Mat heatmap3c = Mat::zeros(camera2.h, camera2.w, encodframe.type());
			Mat heatmap = Mat::zeros(camera2.h, camera2.w, DataType<double>::type);
			//camera2.PositionAndOrient(V3(0, 0, 0), p, V3(0, 1, 0));
			ret1 = path1.CRERI2Conv(encodframe, var, cf, path1.cams[segi], segref, heatmap, &svar);

			//ret1 is the conventional image, heatmap is the heatmap of quality and svar has vto and vtin parameter: max and min sampling interval
			//generate color for heatmap//

			float vtin = svar.vtin;
			float vto = svar.vto;
			min.push_back(svar.min);
			avg.push_back(svar.avg);
			float bb;
			for (int i = 0; i < camera2.h; i++)
			{
				for (int j = 0; j < camera2.w; j++)
				{
					bb = heatmap.at<float>(i, j);
					float factor = (float)bb / (float)(vtin);
					int colora = (float)255 / (float)factor;
					colora = (colora <= 255) ? colora : 255;
					Vec3b insidecolorx(colora, colora, 255);
					heatmap3c.at<Vec3b>(i, j) = insidecolorx;
					//cout << heatmap3c.at<Vec3b>(i, j) << endl;
				}
			}

			hmap.push_back(heatmap3c);
			conv.push_back(ret1);
		}
	}

	ofstream output("./Video/encodingtest/min.txt");
	for (int i = 0; i < min.size(); i++)
	{
		output << min[i] << "\n";
	}
	output.close();

	ofstream output1("./Video/encodingtest/avg.txt");
	for (int i = 0; i < avg.size(); i++)
	{
		output1 << avg[i] << "\n";
	}
	output1.close();

	int starting_frame = 0;
	int ending_frame = fps * chunkN*chunkD;
	filename = "./Video/encodingtest/rollerh264conv";
	videowriterhelperx(111, fps, ret1.cols, ret1.rows, starting_frame, ending_frame, conv);
	filename = "./Video/encodingtest/rollerh264Hmap";
	videowriterhelperx(111, fps, ret1.cols, ret1.rows, starting_frame, ending_frame, hmap);


}

int testTiling() {
	PPC camera1(cFoV, cameraW, cameraH);
	PPC camera2(cFoV, cameraW, cameraH);
	PPC camera3(cFoV, cameraW, cameraH);
	PPC camera4(cFoV, cameraW, cameraH);
	PPC camera5(cFoV, cameraW, cameraH);
	Path path1;
	Path path2;
	Path path3;
	Path path4;
	Path path5;
	int lastframe = 121;

	path5.LoadHMDTrackingData("./Video/roller.txt", camera1);
	
	int m = 1;
	int n = 1;
	int t = 1;
	
	path5.ConvPixel2ERITile("./Video/roller.mkv", lastframe, m, n, t);

	m = 2;
	n = 2;
	t = 1;
	path5.ConvPixel2ERITile("./Video/roller.mkv", lastframe, m, n, t);
	
	m = 4;
	n = 6;
	t = 1;
	path5.ConvPixel2ERITile("./Video/roller.mkv", lastframe, m, n, t);

	m = 8;
	n = 8;
	t = 1;
	path5.ConvPixel2ERITile("./Video/roller.mkv", lastframe, m, n, t);

	m = 10;
	n = 10;
	t = 1;
	path5.ConvPixel2ERITile("./Video/roller.mkv", lastframe, m, n, t);
	STOP;
	m = 20;
	n = 20;
	t = 4;
	path5.ConvPixel2ERITile("./Video/roller.mkv", lastframe, m, n, t);

	m = 30;
	n = 30;
	t = 4;
	path5.ConvPixel2ERITile("./Video/roller.mkv", lastframe, m, n, t);
	return 0;

}

// load  one frame and load one camera from path file and display that frame 

int out_video_file(Mat &output_image_mat, ERI eri_image, Path path1)
{

	VideoCapture cap(VIDEO);
	if (!cap.isOpened()) {
		cout << "Cannot open the video file" << endl;
		system("pause");

	}

	namedWindow("MyVideo", WINDOW_NORMAL);

	int fi = 1;
	while (1)
	{
		Mat frame;
		cap >> frame;

		if (frame.empty())
		{
			cout << "empty" << endl;
			break;
		}
		path1.cams[fi].PositionAndOrient(V3(0, 0, 0), V3(0, 0, 1), V3(0, 1, 0));
		eri_image.ERI2Conv(frame, output_image_mat, path1.cams[fi]);
		imshow("MyVideo", output_image_mat);
		fi++;

		if (waitKey(1) >= 0)
			break;
		char c = (char)waitKey(10);
		if (c == 27)
			break;
	}
	cout << "finish" << endl;
	return 0;
}

//check wether interpolation works between two position of a still image. 

int testPlayBackManualPathStillImage() {
	ERI_INIT;

	int framesN = 30;
	path1.AppendCamera(camera1, framesN);
	camera1.Pan(90);
	path1.AppendCamera(camera1, framesN);
	camera1.Pan(-90);
	path1.AppendCamera(camera1, framesN);
	path1.PlayBackPathStillImage(eriPixels, eri, convPixels);
	return 0;

}


void testvideoendecodenew() {
	vector <Mat> encoded;
	vector <Mat> conv;
	struct var encodevar;
	Path path1;
	PPC camera1(90.0f, 800, 400);
	int cf = 5;

	path1.LoadHMDTrackingData("./Video/roller.txt", camera1);
	VideoCapture cap("./Video/roller.MKV");
	if (!cap.isOpened())
	{
		cout << "Cannot open the video file: " << endl;
		waitKey(100000);
	}

	int lastframe = 720;
	int fps = 30;
	int segi = 0;
	Mat ret; Mat ret1;

	for (int fi = 0; fi < lastframe; fi++)
	{
		Mat frame;
		cap >> frame;
		if (frame.empty())
		{
			cout << fi << endl;
			cout << "Can not read video frame: " << endl;
			break;
		}
		segi = path1.GetCamIndex(fi, fps, segi);

		ret = path1.EncodeNewNonLinV2(frame, &encodevar, path1.cams[segi], camera1, cf);
		encoded.push_back(ret);
		//namedWindow("sample", WINDOW_NORMAL);
		//resizeWindow("sample", 800, 800);
		//imshow("sample", ret);
		//waitKey(1000);
		float var[10];
		var[0] = frame.cols;
		var[1] = frame.rows;
		var[2] = encodevar.We;
		var[3] = encodevar.Het;//*/	

		PPC camera2(120.0f, 800, 400);
		camera2.Pan(120.0f);
		//camera2.Tilt(25.0f);
		//ret1 =  path1.CRERI2Conv(encodframe, var, cf, camera2, heatmap, &svar);
		//conv.push_back(ret1);
		//imshow("sample", ret1);
		waitKey(1000);
	}

	VideoWriter writer;
	int codec = VideoWriter::fourcc('H', '2', '6', '4');
	writer.set(VIDEOWRITER_PROP_QUALITY, 30);
	string filename = "./Video/encodingtest/rollerh264encod.MKV";
	cout << "Writing videofile: " << filename << codec << endl;
	writer.open(filename, codec, fps, Size(ret.cols, ret.rows), true);

	if (!writer.isOpened())
	{
		cerr << "Could not open the output video file for write\n";
		return;
	}
	for (int i = 0; i < encoded.size(); i++)
	{
		writer.write(encoded[i]);
	}
	writer.release();

	VideoWriter writer1;
	writer1.set(VIDEOWRITER_PROP_QUALITY, 30);
	filename = "./Video/encodingtest/rollerh264conv.MKV";
	cout << "Writing videofile: " << filename << codec << endl;
	writer1.open(filename, codec, fps, Size(ret1.cols, ret1.rows), true);

	if (!writer1.isOpened())
	{
		cerr << "Could not open the output video file for write\n";
		return;
	}
	for (int i = 0; i < conv.size(); i++)
	{
		writer1.write(conv[i]);
	}
	writer1.release();
}


int testvideoencodedecode() {

	vector<float> We;
	vector<float> Het;
	vector<float>Heb;
	vector<float> R0x;
	vector<float>R0y;
	vector<float> R0R1;
	vector<float>R0R4;
	int compressionfactor = 5;
	Path path1;
	vector <Mat> encodedbuffer;
	PPC camera1(110.0f, 800, 400);
	path1.LoadHMDTrackingData("./Video/roller.txt", camera1);
	int lastframe = 5;
	encodedbuffer = path1.videoencode("./Video/roller_2000_1000.mp4", lastframe, We, Het, Heb, R0x, R0y, R0R1, R0R4, compressionfactor);
	path1.videodecode("./Video/encodingtest/rollerh264encod.MKV", lastframe, 2000, 1000, We, Het, Heb, R0x, R0y, R0R1, R0R4, compressionfactor);
	//change the frame height and width in this funciton
	return 0;

}

void testRotationxy()
{
	Mat eriPixels;
	upload_image("./Image/360_equirectangular_800_400.JPG", eriPixels);
	if (!eriPixels.data)                              // Check for invalid input
	{
		cout << "Could not open or find the image" << std::endl;
		return;
	}


	float hfov = 110.0f;
	PPC camera(hfov, 800, 400);

	Path path1;
	path1.LoadHMDTrackingData("./Video/roller.txt", camera);
	path1.BuildERI2RERIVideo(eriPixels, camera);



}

