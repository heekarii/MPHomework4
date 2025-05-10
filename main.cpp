#include <opencv2/opencv.hpp>

// 브러쉬 정보 저장하는 구조체
struct BrushStroke {
	int size;
	CvScalar c;
	CvPoint sp;
	CvPoint *route;
};

// 색 차이 계산
inline float getDiff(CvScalar a, CvScalar b) {
	return sqrt((a.val[0] - b.val[0]) * (a.val[0] - b.val[0]) +
				(a.val[1] - b.val[1]) * (a.val[1] - b.val[1]) +
				(a.val[2] - b.val[2]) * (a.val[2] - b.val[2])
	);
}

// function declaration
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
    // size of source image
	CvSize isize = cvGetSize(src);

	IplImage *dst = cvCreateImage(isize, 8, 3);
	cvSet(dst, cvScalar(255,255,255));

    // array of brush size
	int size[5] = { 32, 16, 8, 4, 2 };
	cvShowImage("src", src);
	paint(src, dst, size, mode);
	cvWaitKey();

	return 0;
}

void paint(IplImage *src, IplImage *dst, int size[], int mode) {
    int h = src->height;
    int w = src->width;
    // reference image
    IplImage *ref = cvCreateImage(cvSize(h, w), 8, 3);
    cvShowImage("dst", dst);
    cvWaitKey(1000);

    // 가우시안 블러 처리한 이미지 생성(ref)
    // ref 기반으로 색 칠하기
    for (int i = 0; i < 5; i++) {
        cvSmooth(src, ref, CV_GAUSSIAN, 0, 0, size[i]);
        paintLayer(dst, ref, size[i], mode);
    }
}

void paintLayer(IplImage *dst, IplImage *ref, int size, int mode) {
    // circle grid : brush size 보다 조금 작게
    int grid = size - 1;
    // 그리드 총 개수
    int arSize = (dst->width / grid) * (dst->height / grid);

    // 스트로크 정보를 저장할 배열
    BrushStroke *s = new BrushStroke[arSize];
    int cnt = 0;

    // 그리드 단위로 계산
    for (int y = 0; y <= dst->height - grid; y += grid) {
        for (int x = 0; x <= dst->width - grid; x += grid) {
            // 평균 에러
            float err = 0;
            // 최고 에러 값
            float maxErr = FLT_MIN;
            BrushStroke p = { size, cvScalar(255,255,255), cvPoint(0, 0) };

            // 그리드 내부에서 평균 에러 계산
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

            // 그리드 내부에서 최대 에러 값을 기준으로 브러쉬 정보 저장
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
            // 브러쉬의 색상은 ref 이미지에서 가져옴
            p.c = cvGet2D(ref, p.sp.y, p.sp.x);
            // 평균에러가 T 보다 큰 경우에 브러쉬 정보 저장
            if (err > 25)
                s[cnt++] = p;
        }
    }

    // 칠하는 순서 랜덤으로 정하기
    for (int i = cnt - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(s[i], s[j]);
    }

    // circle
    if (mode == 0) {
        paintStroke(dst, s, cnt);
    }
    // stroke
    else if (mode == 1) {
        paintSplineStroke(ref, dst, s, cnt);
    }

    cvShowImage("dst", dst);
    cvWaitKey(1000);
    delete[] s;
}

// 원 칠하는 함수
void paintStroke(IplImage *dst, BrushStroke *s, int k) {
    for (int i = 0; i < k; i++) {
        cvCircle(dst, s[i].sp, s[i].size, s[i].c, -1);
    }
}

// spline stroke
void paintSplineStroke(IplImage *ref, IplImage *dst, BrushStroke *s, int k) {
    // style parameter
    // constant
    const int minLen = 7;
    const int maxLen = 15;
    // 굴절 계수 -> 작을수록 현재 기울기 방향으로 편향됨.
    const float fc = 0.75f;

    int width = ref->width;
    int height = ref->height;

    // 저장된 브러쉬 정보 갯수만큼 반복
    for (int i = 0; i < k; i++) {
        // 시작점
        CvPoint start = s[i].sp;
        int R = s[i].size;
        CvScalar color = s[i].c;
        // 직전 픽셀에서의 기울기
        float lastDx = 0.0f, lastDy = 0.0f;
        CvPoint pt = start;

        // 붓이 이동할 경로
        s[i].route = new CvPoint[64];
        // 붓이 이동한 횟수
        int routeLen = 0;
        s[i].route[routeLen++] = pt;

        for (int l = 1; l < maxLen; l++) {
            // 좌표 초기화
            int x = pt.x, y = pt.y;
            // 첫번째 종료 조건
            // 붓의 색 & ref 이미지의 색 차이 > ref이미지의 색 & canvas의 색
            {
                CvScalar refC = cvGet2D(ref, y, x);
                CvScalar dstC = cvGet2D(dst, y, x);
                float diffCanvas = getDiff(refC, dstC);
                float diffStroke = getDiff(refC, color);
                if (l >= minLen && diffCanvas < diffStroke)
                    break;
            }

            float gx, gy; 
            // 두번째 종료 조건
            // 기울기가 작을 때
            {
                // 현재 기울기 구하기
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
            // 현재 기울기에 수직으로
            float dx = -gy;
            float dy = gx;
            if (dx * lastDx + dy * lastDy < 0) {
                dx = -dx; dy = -dy;
            }

            // 경로 설정해주기
            dx = fc * dx + (1 - fc) * lastDx;
            dy = fc * dy + (1 - fc) * lastDy;
            {
                float m = sqrt(dx * dx + dy * dy);
                dx /= m; 
                dy /= m;
            }
            pt.x = (int)(pt.x + R * dx + 0.5f);
            pt.y = (int)(pt.y + R * dy + 0.5f);
            // 붓 경로가 이미지 밖에 있으면 탈출
            if (pt.x < 0 || pt.x >= width ||
                pt.y < 0 || pt.y >= height)
                break;

            // 이동 경로 추가
            s[i].route[routeLen++] = pt;
            lastDx = dx; 
            lastDy = dy;
        }

        // 그려주기
        for (int j = 0; j + 1 < routeLen; j++) {
            cvLine(dst, s[i].route[j], s[i].route[j + 1], color, R);
        }
        delete[] s[i].route;
    }
}

// 기울기 구하기(Luminace 기준)
float getGradient(CvScalar a, CvScalar b) {
    return getLuminace(a) - getLuminace(b);
}

// 논문에 적혀있는 비율로 luminance값 구하기
float getLuminace(CvScalar f) {
    return 0.3f * f.val[2] + 0.59f * f.val[1] + f.val[0];
}
