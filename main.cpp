#include <opencv2/opencv.hpp>

struct BrushStroke {
	int size;
	CvScalar c;
	CvPoint sp;
	CvPoint *route;
};

void paint(IplImage *src, IplImage *dst, int size[]) {
	int h = src->height;
	int w = src->width;
	IplImage *ref = cvCreateImage(cvSize(h, w), 8, 3);


}

int main() {
	IplImage *src = cvLoadImage("c:\\temp\\lena.jpg");
	CvSize size = cvGetSize(src);

	IplImage *dst = cvCreateImage(size, 8, 3);
	cvSet(dst, cvScalar(255, 255, 255));




	return 0;
}