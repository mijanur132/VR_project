#pragma once

#include "v3.h"
#include <C:\opencv\build\include\opencv2/opencv.hpp>
#include <C:\opencv\build\include\opencv2\core\core.hpp>
#include <C:\opencv\build\include\opencv2\highgui\highgui.hpp>
#include<math.h>

using namespace cv;

//class FrameBuffer;

class PPC {
public:
	V3 a, b, c, C; // eye is 3D point C
	int w, h; // image resolution

  // horizontal field of view and image resolution
  // this constructor places the eye at (0, 0, 0), view direction is (0, 0, -1), and the up vector is (0, 1, 0)
	PPC(float hfov, int _w, int _h);

  // takes a 3D point and projects it on the image plane
  // return 0 if the point is behind the head
  // the image pixel coordinates of the projected point are in pp[0] (pixel column) and pp[1] (pixel row)
  int Project(V3 p, V3 &pp);
	
  V3 GetVD();
	float GetFocalLength();
	void ChangeFocalLength(float scf);
	void PositionAndOrient(V3 C1, V3 L1, V3 vpv);
	void SetInterpolated(PPC *ppc0, PPC *ppc1, int stepi, int stepsN);
	void Pan(float angled);
	void Tilt(float angled);
	void Roll(float angled);
//	void Visualize(PPC *ppc3, FrameBuffer *fb3, float vf, V3 colv);
	V3 UnprojectPixel(float uf, float vf, float currf);
	V3 Unproject(V3 pP);
//	void SetIntrinsicsHW();
//	void SetExtrinsicsHW();

	//int upload_image(string path, Mat &image);  //palash
};