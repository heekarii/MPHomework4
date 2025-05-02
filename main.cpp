#include <opencv2/opencv.hpp>
#include<cstdlib>
#include<ctime>

struct BrushStroke {
	int size;
	CvScalar c;
	CvPoint sp;
	CvPoint *route;
};

inline float getDiff(CvScalar a, CvScalar b) {
	return sqrt((a.val[0] - b.val[0]) * (a.val[0] - b.val[0]) +
				(a.val[1] - b.val[1]) * (a.val[1] - b.val[1]) +
				(a.val[2] - b.val[2]) * (a.val[2] - b.val[2])
	);
}

void paintLayer(IplImage *dst, IplImage *ref, int size) {

	int grid = size - 1;
	int arSize = (dst->width / grid) * (dst->height / grid);
	BrushStroke *s = new BrushStroke[arSize];

	int cnt = 0;
	for (int y = 0; y < dst->height; y += grid) {
		for (int x = 0; x < dst->width; x += grid) {
			if (x < 0 || x > dst->width - 1) continue;
			if (y < 0 || y > dst->height - 1) continue;
			float err = 0;
			float maxErr = FLT_MIN;
			BrushStroke p;
			for (int u = y; u < y + grid; y++) {
				for (int v = x; v < x + grid; x++) {
					if (u > dst->height - 1) continue;
					if (v > dst->width - 1) continue;
					CvScalar a = cvGet2D(dst, u, v);
					CvScalar b = cvGet2D(ref, u, v);
					err += getDiff(a, b);
				}
			}
			err /= (grid * grid);
			printf("x = %d y = %d err = %f\n", x, y, err);
			for (int u = y; u < y + grid; y++)
				for (int v = x; v < x + grid; x++) {
					if (u > dst->height - 1) continue;
					if (v > dst->width - 1) continue;
					CvScalar a = cvGet2D(dst, u, v);
					CvScalar b = cvGet2D(ref, u, v);
					float e = getDiff(a, b);
					if (e > maxErr) {
						maxErr = e;
						p.sp = cvPoint(v, u);
						p.size = size;
						p.c = cvGet2D(dst, y, x);
					}
				}
			s[cnt++] = p;
		}
	}

	for (int k = 0; k < arSize; k++) {
		cvCircle(dst, s[k].sp, s[k].size, s[k].c);
	}

}

void paint(IplImage *src, IplImage *dst, int size[]) {
	int h = src->height;
	int w = src->width;
	IplImage *ref = cvCreateImage(cvSize(h, w), 8, 3);
	
	int k = 3;

	for (int i = 0; i < 5; i++) {
		cvSmooth(src, ref, CV_GAUSSIAN, size[i] * 4 - 1);
		cvShowImage("ref", ref);
		cvWaitKey();
		paintLayer(dst, ref, size[i]);
	}

	cvShowImage("dst", dst);
	cvWaitKey();
}





int main() {
	IplImage *src = cvLoadImage("c:\\temp\\lena.jpg");
	CvSize isize = cvGetSize(src);

	IplImage *dst = cvCreateImage(isize, 8, 3);
	cvSet(dst, cvScalar(INT_MAX, INT_MAX, INT_MAX));

	int size[5] = { 10, 8, 6, 4, 2 };
	for (int i = 0; i < 5; i++) {
		paint(src, dst, size);
	}
	cvShowImage("src", src);
	cvWaitKey();

	return 0;
}