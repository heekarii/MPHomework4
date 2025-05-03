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

void paintLayer(IplImage *dst, IplImage *ref, int size, int mode);
void paint(IplImage *src, IplImage *dst, int size[], int mode);
void paintStroke(IplImage *src, BrushStroke *s, int k);


int main() {
	IplImage *src = cvLoadImage("c:\\temp\\lena.jpg");
	CvSize isize = cvGetSize(src);

	IplImage *dst = cvCreateImage(isize, 8, 3);
	cvSet(dst, cvScalar(255,255,255));

	int size[5] = { 32, 16, 8, 4, 2 };
	cvShowImage("src", src);
	paint(src, dst, size, 0);
	cvWaitKey();

	return 0;
}

void paint(IplImage *src, IplImage *dst, int size[], int mode) {
    int h = src->height;
    int w = src->width;
    IplImage *ref = cvCreateImage(cvSize(h, w), 8, 3);
    cvShowImage("dst", dst);
    cvWaitKey();
    int k = 3;

    for (int i = 0; i < 5; i++) {
        cvSmooth(src, ref, CV_GAUSSIAN, size[i] * 4 + 1);
        paintLayer(dst, ref, size[i], mode);
    }
}


void paintLayer(IplImage *dst, IplImage *ref, int size, int mode) {
    int grid = size - 1;
    int arSize = (dst->width / grid) * (dst->height / grid);
    BrushStroke *s = new BrushStroke[arSize];
    int cnt = 0;

    for (int y = 0; y <= dst->height - grid; y += grid) {
        for (int x = 0; x <= dst->width - grid; x += grid) {
            float err = 0;
            float maxErr = -1.0f;
            BrushStroke p;

            for (int u = y; u < y + grid; u++) {
                for (int v = x; v < x + grid; v++) {
                    CvScalar a = cvGet2D(dst, u, v);
                    CvScalar b = cvGet2D(ref, u, v);
                    err += getDiff(a, b);
                }
            }
            err /= (grid * grid);

            for (int u = y; u < y + grid; u++) {
                for (int v = x; v < x + grid; v++) {
                    float e = getDiff(cvGet2D(dst, u, v), cvGet2D(ref, u, v));
                    if (e > maxErr) {
                        maxErr = e;
                        p.sp = cvPoint(v, u);
                        p.size = size;
                    }
                }
            }
            p.c = cvGet2D(ref, p.sp.y, p.sp.x); // 색상은 ref 기준
            s[cnt++] = p;
        }
    }

    for (int i = cnt - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(s[i], s[j]);
    }

    if (mode == 0) {
        paintStroke(dst, s, cnt);
    }
    else if (mode == 1) {

    }

    cvShowImage("dst", dst);
    cvWaitKey();
    delete[] s;
}

void paintStroke(IplImage *dst, BrushStroke *s, int k) {
    for (int i = 0; i < k; i++) {
        cvCircle(dst, s[i].sp, s[i].size, s[i].c, -1);
    }
}