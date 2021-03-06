// dnf-jigsaw.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Opencv2/OpenCv.hpp"
#include "windows.h"


void get(cv::Mat src);

inline unsigned int rgb(unsigned char r, unsigned char g, unsigned char b) {
	return (static_cast<unsigned int>(r) << 24) + (static_cast<unsigned int>(g) << 16) + (static_cast<unsigned int>(b));
}

enum block_state {
	in,
	none,
	out
};

struct block {
	block_state up;
	block_state down;
	block_state left;
	block_state right;
};

HBITMAP   GetCaptureBmp()
{
	HDC     hDC;
	HDC     MemDC;
	BYTE*   Data;
	HBITMAP   hBmp;
	BITMAPINFO   bi;

	memset(&bi, 0, sizeof(bi));
	bi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bi.bmiHeader.biWidth = GetSystemMetrics(SM_CXSCREEN);
	bi.bmiHeader.biHeight = GetSystemMetrics(SM_CYSCREEN);
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;

	hDC = GetDC(NULL);
	MemDC = CreateCompatibleDC(hDC);
	hBmp = CreateDIBSection(MemDC, &bi, DIB_RGB_COLORS, (void**)&Data, NULL, 0);
	SelectObject(MemDC, hBmp);
	BitBlt(MemDC, 0, 0, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, hDC, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hDC);
	DeleteDC(MemDC);
	return   hBmp;
}

int SaveBitmapToFile(HBITMAP hBitmap, LPCWSTR lpFileName)
{
	HDC            hDC; //设备描述表  
	int            iBits;//当前显示分辨率下每个像素所占字节数  
	WORD           wBitCount;//位图中每个像素所占字节数      
	DWORD          dwPaletteSize = 0;//定义调色板大小  
	DWORD          dwBmBitsSize;//位图中像素字节大小  
	DWORD          dwDIBSize;// 位图文件大小  
	DWORD          dwWritten;//写入文件字节数  
	BITMAP         Bitmap;//位图结构  
	BITMAPFILEHEADER   bmfHdr;   //位图属性结构     
	BITMAPINFOHEADER   bi;       //位图文件头结构  
	LPBITMAPINFOHEADER lpbi;     //位图信息头结构     指向位图信息头结构  
	HANDLE          fh;//定义文件句柄  
	HANDLE            hDib;//分配内存句柄  
	HANDLE            hPal;//分配内存句柄  
	HANDLE          hOldPal = NULL;//调色板句柄    

								   //计算位图文件每个像素所占字节数     
	hDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
	DeleteDC(hDC);

	if (iBits <= 1)
		wBitCount = 1;
	else if (iBits <= 4)
		wBitCount = 4;
	else if (iBits <= 8)
		wBitCount = 8;
	else if (iBits <= 24)
		wBitCount = 24;
	else if (iBits <= 32)
		wBitCount = 24;


	//计算调色板大小     
	if (wBitCount <= 8)
		dwPaletteSize = (1 << wBitCount) * sizeof(RGBQUAD);



	//设置位图信息头结构     
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	dwBmBitsSize = ((Bitmap.bmWidth *wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	//为位图内容分配内存     
	hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	if (lpbi == NULL)
	{
		return 0;
	}

	*lpbi = bi;
	// 处理调色板  
	hPal = GetStockObject(DEFAULT_PALETTE);
	if (hPal)
	{
		hDC = GetDC(NULL);
		hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE);
		RealizePalette(hDC);
	}
	// 获取该调色板下新的像素值     
	GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight,
		(LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize,
		(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
	//恢复调色板        
	if (hOldPal)
	{
		SelectPalette(hDC, (HPALETTE)hOldPal, TRUE);
		RealizePalette(hDC);
		ReleaseDC(NULL, hDC);
	}
	//创建位图文件         
	fh = CreateFile(lpFileName, GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;

	// 设置位图文件头     
	bmfHdr.bfType = 0x4D42;  // "BM"     
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;

	// 写入位图文件头     
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);

	// 写入位图文件其余内容     
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);

	//清除        
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(fh);

	return 1;
}


int main()
{
	using namespace std;
	using namespace cv;
	Sleep(5000);
	HBITMAP   hBmp;
	hBmp = GetCaptureBmp();
	SaveBitmapToFile(hBmp, L"11.bmp");
	Mat screen = imread("11.bmp");



	//742 473
	Mat pic = imread("up.png");
	Mat result;

	int result_cols = screen.cols - pic.cols + 1;
	int result_rows = screen.rows - pic.rows + 1;
	result.create(result_cols, result_rows, CV_32FC1);
	matchTemplate(screen, pic, result, CV_TM_SQDIFF_NORMED);//这里我们使用的匹配算法是标准平方差匹配 method=CV_TM_SQDIFF_NORMED，数值越小匹配度越好
	normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

	double minVal = -1;
	double maxVal;
	Point minLoc;
	Point maxLoc;
	Point matchLoc;
	cout << "匹配度：" << minVal << endl;
	minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
	cout << "匹配度：" << minVal << endl;
	matchLoc = minLoc;

	Mat src = Mat::Mat(screen, Range(matchLoc.y, matchLoc.y + 473), Range(matchLoc.x,matchLoc.x + 742));
	get(src);

	cin.get();
	cin.get();

    return 0;
}

void get(cv::Mat src) {
	using namespace std;
	using namespace cv;
	unsigned char a = 1;

	//int a = RGB(1, 2, 3);

	printf("%X\n", ((unsigned int)1 << 24) + ((unsigned int)2 << 16) + ((unsigned int)3));
	//cout << RGB(1, 2, 3) << endl;

	cv::namedWindow("dxf");
	//Mat src = imread("x.png");
	Mat pic = src;


	cv::cvtColor(src, src, CV_BGR2GRAY);

	//Mat element1 = getStructuringElement(MORPH_RECT, Size(20, 20)); //第一个参数MORPH_RECT表示矩形的卷积核，当然还可以选择椭圆形的、交叉型的
	//dilate(src, result, element);

	for (int i = 0; i < src.rows; i++) {
		for (int j = 0; j < src.cols; j++) {

			if (src.at<uchar>(i, j) != 55 && src.at<uchar>(i, j) != 56 && src.at<uchar>(i, j) != 57) {
				src.at<uchar>(i, j) = 0;

			}
			//cout << (int)src.at<uchar>(i, j) << endl;
		}
	}
	threshold(src, src, 1, 255, THRESH_BINARY);//通过阈值操作把灰度图变成二值图  
	Mat rows;
	Mat cols;

	reduce(src, rows, 1, CV_REDUCE_SUM, CV_32S);//列向量
	reduce(src, cols, 0, CV_REDUCE_SUM, CV_32S);//行向量


	int start = 0, end = 0, start1 = 0, end1 = 0;

	for (int i = 0; i < rows.rows; i++) {
		int a = rows.at<int>(i, 0);
		cout << a << endl;
		if (a > 50000) {
			start = i;
			break;
		}
	}

	for (int i = rows.rows - 1; i > 0; i--) {
		int a = rows.at<int>(i, 0);
		cout << a << endl;
		if (a > 50000) {
			end = i;
			break;
		}
	}
	for (int i = 0; i < cols.cols; i++) {
		int a = cols.at<int>(0, i);
		cout << a << endl;
		if (a > 50000) {
			start1 = i;
			break;
		}
	}

	for (int i = cols.cols - 1; i > 0; i--) {
		int a = cols.at<int>(0, i);
		cout << a << endl;
		if (a > 50000) {
			end1 = i;
			break;
		}
	}

	cout << "start:" << start << "   " << "end:" << end << endl;
	cout << "start1:" << start1 << "   " << "end1:" << end1 << endl;

	Mat element = getStructuringElement(MORPH_RECT, Size(15, 15)); //第一个参数MORPH_RECT表示矩形的卷积核，当然还可以选择椭圆形的、交叉型的
																   //dilate(src, src, element);
																   //erode(src, src, element);


	src = Mat::Mat(src, Range(start - 1, end + 1), Range(start1 - 1, end1 + 1));
	pic = Mat::Mat(pic, Range(start - 1, end + 1), Range(start1 - 1, end1 + 1));

	Mat mat_arry[5][6];

	int row_start_line[7];
	int row_end_line[7];
	int col_start_line[6];
	int col_end_line[6];

	row_start_line[0] = 0;
	row_end_line[0] = 0;

	col_start_line[0] = 0;
	col_end_line[0] = 0;

	row_start_line[6] = src.cols;
	row_end_line[6] = src.cols;

	col_start_line[5] = src.rows;
	col_end_line[5] = src.rows;

	//分割方块
	reduce(src, rows, 1, CV_REDUCE_SUM, CV_32S);//行向量
	reduce(src, cols, 0, CV_REDUCE_SUM, CV_32S);//列向量

	{
		int count = 1;
		bool flag = false;
		for (int i = 0; i < rows.rows; i++) {
			//小于25个点
			if (!flag && rows.at<int>(i, 0) < 10000) {
				col_start_line[count] = i;
				flag = true;
			}
			if (flag && rows.at<int>(i, 0) > 10000) {
				if (i - col_start_line[count] > 2) {
					col_end_line[count++] = i - 1;
				}
				flag = false;
			}
		}
	}

	{
		int count = 1;
		bool flag = false;
		for (int i = 0; i < cols.cols; i++) {
			//小于25个点
			if (!flag && cols.at<int>(0, i) < 10000) {
				row_start_line[count] = i;
				flag = true;
			}
			if (flag && cols.at<int>(0, i) > 10000) {
				if (i - row_start_line[count] > 2) {
					row_end_line[count++] = i - 1;
				}
				flag = false;
			}
		}
	}

	Rect all_rect[5][6];
	block result[5][6];

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 6; j++) {
			Rect& r = all_rect[i][j];


			r.y = col_end_line[i];
			r.x = row_end_line[j];

			r.height = col_start_line[i + 1] - col_end_line[i];
			r.width = row_start_line[i + 1] - row_end_line[i];

		}
	}
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 6; j++) {
			cout << "矩形(" << i << "," << j << ") : " << "x=" << all_rect[i][j].x << ",y=" << all_rect[i][j].y;
			cout << ",width=" << all_rect[i][j].width << ",heigh=" << all_rect[i][j].height << endl;
		}
	}

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 6; j++) {
			static int row_start, col_start;
			Rect& r = all_rect[i][j];

			int w = r.width / 3;
			int h = r.height / 3;

			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					row_start = r.x + w * j;
					col_start = r.y + h * i;
					int count = 0;

					for (int m = 0; m < h; m++) {
						for (int n = 0; n < w; n++) {
							if (pic.at<Vec3b>(col_start + m, row_start + n)[0] != pic.at<Vec3b>(col_start + m, row_start + n)[1]
								&& pic.at<Vec3b>(col_start + m, row_start + n)[1] != pic.at<Vec3b>(col_start + m, row_start + n)[2]) {
								count++;
							}
						}
					}
					if (count > 10) {
						for (int m = 0; m < h; m++) {
							for (int n = 0; n < w; n++) {
								src.at<uchar>(col_start + m, row_start + n) = 255;
							}
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 6; j++) {
			Rect& r = all_rect[i][j];

			int w = r.width / 3;
			int h = r.height / 3;

			auto getBlockState = [&src, &r, &w, &h](int c_start, int r_start) -> bool {
				int count = 0;
				r_start += 2;
				c_start += 4;
				for (int i = r_start; i < r_start + h - 4; i++) {
					for (int j = c_start; j < c_start + w - 8; j++) {
						if (src.at<uchar>(i, j) == 255)
							count++;
					}
				}

				if (count > 10)
					return true;
				return false;

			};

			//判断各块状态
			result[i][j].up = getBlockState(r.x + w, r.y) ? in : out;
			result[i][j].down = getBlockState(r.x + w, r.y + 2 * h) ? in : out;
			result[i][j].left = getBlockState(r.x, r.y + h) ? in : out;
			result[i][j].right = getBlockState(r.x + w * 2, r.y + h) ? in : out;

			if (j == 0)
				result[i][j].left = none;
			if (i == 0)
				result[i][j].up = none;
			if (j == 5)  result[i][j].right = none;
			if (i == 4) result[i][j].down = none;


		}
	}


	cout << "准备输出最终拼图:" << endl;

	auto print_result = [](block& b) {
		bool r[5][5];
		memset(r, 0, 25);

		r[1][1] = 1;
		r[3][1] = 1;
		r[1][3] = 1;
		r[3][3] = 1;
		r[2][2] = 1;
		if (b.up == out)
			r[1][2] = 1, r[0][2] = 1;
		if (b.down == out)
			r[3][2] = 1, r[4][2] = 1;
		if (b.left == out)
			r[2][0] = 1, r[2][1] = 1;
		if (b.right == out)
			r[2][3] = 1, r[2][4] = 1;

		if (b.up == none)
			r[1][2] = 1;
		if (b.down == none)
			r[3][2] = 1;
		if (b.left == none)
			r[2][1] = 1;
		if (b.right == none)
			r[2][3] = 1;

		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				if (r[i][j])
					cout << "■";
				else
					cout << "  ";
			}
			cout << endl;
		}
	};

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 6; j++) {
			cout << "第(" << i << "," << j << ")" << "块的需求方块:" << endl;
			print_result(result[i][j]);
		}
	}
	//dilate(src, src, element);

	//erode(src, src, element);

	//cout << (int)src.at<int>(1, 0) << endl;
	//cout << (int)src.at<int>(1, 0) << endl;
	//cout << (int)src.at<int>(1, 0) << endl;

	//threshold(src, src, 30, 255, CV_THRESH_BINARY);


	imshow("dxf", src);
	//imshow("dxf" ,pic);

	//cv::startWindowThread();
	//cv::updateWindow("dxf");
	cv::waitKey(1000000);

	//
	//cv::waitKey(0);
	//cvReleaseImage(&src);
	//cvReleaseImage(&result);
	cv::destroyAllWindows();


}