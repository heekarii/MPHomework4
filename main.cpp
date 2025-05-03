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
void paintSplineStroke(IplImage *src, IplImage *dst, BrushStroke *s, int k);
float getGradient(CvScalar a, CvScalar b);
float getLuminace(CvScalar a);

int main() {
    char path[101];
    printf("=============================================\n");
    printf("Department of Software, Sejong University");
    printf("Multimedia Programming Homework #4\n");
    printf("Painterly Rendering\n");
    printf("=============================================\n");
    while (1) {
        printf("Input File Path:");
        scanf("%s", path);
        if (cvLoadImage((const char *)path) == nullptr) {
            printf("File not Found!\n");
            continue;
        }
        else {
            break;
        }
    }
    IplImage *src = cvLoadImage((const char *) path);
    int mode;
    while (1) {
        printf("Select Drawing Mode (0=circle, 1=stroke):");
        scanf("%d", &mode);
        if (mode == 0 || mode == 1)
            break;
        else {
            printf("Wrong Drawing Mode!");
            continue;
        }
    }
	CvSize isize = cvGetSize(src);

	IplImage *dst = cvCreateImage(isize, 8, 3);
	cvSet(dst, cvScalar(255,255,255));

	int size[5] = { 32, 16, 8, 4, 2 };
	cvShowImage("src", src);
	paint(src, dst, size, mode);
	cvWaitKey();

	return 0;
}

void paint(IplImage *src, IplImage *dst, int size[], int mode) {
    int h = src->height;
    int w = src->width;
    IplImage *ref = cvCreateImage(cvSize(h, w), 8, 3);
    cvShowImage("dst", dst);
    cvWaitKey(1000);

    for (int i = 0; i < 5; i++) {
        cvSmooth(src, ref, CV_GAUSSIAN, 0, 0, size[i]);
        paintLayer(dst, ref, size[i], mode);
    }
}

void paintLayer(IplImage *dst, IplImage *ref, int size, int mode) {
    int grid = size;
    int arSize = (dst->width / grid) * (dst->height / grid);
    BrushStroke *s = new BrushStroke[arSize];
    int cnt = 0;

    for (int y = 0; y <= dst->height - grid; y += grid) {
        for (int x = 0; x <= dst->width - grid; x += grid) {
            float err = 0;
            float maxErr = FLT_MIN;
            BrushStroke p = { size, cvScalar(255,255,255), cvPoint(0, 0) };

            for (int u = y; u < y + grid; u++) {
                for (int v = x; v < x + grid; v++) {
                    if (u > ref->height - 1) continue;
                    if (v > ref->width - 1)continue;
                    CvScalar a = cvGet2D(dst, u, v);
                    CvScalar b = cvGet2D(ref, u, v);
                    err += getDiff(a, b);
                }
            }
            err /= (grid * grid);

            for (int u = y; u < y + grid; u++) {
                for (int v = x; v < x + grid; v++) {
                    if (u > ref->height - 1) continue;
                    if (v > ref->width - 1)continue;
                    float e = getDiff(cvGet2D(dst, u, v), cvGet2D(ref, u, v));
                    if (e > maxErr) {
                        maxErr = e;
                        p.sp = cvPoint(v, u);
                        p.size = size;
                    }
                }
            }
            p.c = cvGet2D(ref, p.sp.y, p.sp.x);
            if (err > 25)
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
        paintSplineStroke(ref, dst, s, cnt);
    }

    cvShowImage("dst", dst);
    cvWaitKey(1000);
    delete[] s;
}

void paintStroke(IplImage *dst, BrushStroke *s, int k) {
    for (int i = 0; i < k; i++) {
        cvCircle(dst, s[i].sp, s[i].size, s[i].c, -1);
    }
}

void paintSplineStroke(IplImage *ref, IplImage *dst, BrushStroke *s, int k) {
    const int minLen = 7;
    const int maxLen = 15;
    const float fc = 0.75f;

    int width = ref->width;
    int height = ref->height;

    for (int i = 0; i < k; i++) {
        CvPoint start = s[i].sp;
        int R = s[i].size;
        CvScalar color = s[i].c;
        float lastDx = 0.0f, lastDy = 0.0f;
        CvPoint pt = start;

        s[i].route = new CvPoint[64];
        int routeLen = 0;
        s[i].route[routeLen++] = pt;

        for (int l = 1; l < maxLen; l++) {
            int x = pt.x, y = pt.y;
            {
                CvScalar refC = cvGet2D(ref, y, x);
                CvScalar dstC = cvGet2D(dst, y, x);
                float diffCanvas = getDiff(refC, dstC);
                float diffStroke = getDiff(refC, color);
                if (l >= minLen && diffCanvas < diffStroke)
                    break;
            }

            float gx, gy; 
            {
                int xm1 = x - 1 > 0 ? x - 1 : 0;
                int xp1 = x + 1 < width - 1 ? x + 1 : width - 1;
                int ym1 = y - 1 > 0 ? y - 1 : 0;
                int yp1 = y + 1 < height - 1 ? y + 1 : height - 1;
                float Lxm1 = getLuminace(cvGet2D(ref, y, xm1));
                float Lxp1 = getLuminace(cvGet2D(ref, y, xp1));
                float Lym1 = getLuminace(cvGet2D(ref, ym1, x));
                float Lyp1 = getLuminace(cvGet2D(ref, yp1, x));
                gx = (Lxp1 - Lxm1) * 0.5f;
                gy = (Lyp1 - Lym1) * 0.5f;
                if (abs(gx) < 0.3f && abs(gy) < 0.3f)
                    break;
            }
            float dx = -gy;
            float dy = gx;
            if (dx * lastDx + dy * lastDy < 0) {
                dx = -dx; dy = -dy;
            }
            dx = fc * dx + (1 - fc) * lastDx;
            dy = fc * dy + (1 - fc) * lastDy;
            {
                float m = sqrt(dx * dx + dy * dy);
                dx /= m; 
                dy /= m;
            }
            pt.x = (int)(pt.x + R * dx + 0.5f);
            pt.y = (int)(pt.y + R * dy + 0.5f);

            if (pt.x < 0 || pt.x >= width ||
                pt.y < 0 || pt.y >= height)
                break;

            s[i].route[routeLen++] = pt;
            lastDx = dx; 
            lastDy = dy;
        }

        for (int j = 0; j + 1 < routeLen; j++) {
            cvLine(dst, s[i].route[j], s[i].route[j + 1], color, R);
        }
        delete[] s[i].route;
    }
}

float getGradient(CvScalar a, CvScalar b) {
    return getLuminace(a) - getLuminace(b);
}

float getLuminace(CvScalar f) {
    return 0.3f * f.val[2] + 0.59f * f.val[1] + f.val[0];
}
