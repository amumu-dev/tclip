#include "cv.h"  
#include "opencv2/core/core.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"

#include <iostream>
#include <map>
#include <math.h>
#include <time.h>

using namespace cv;
using namespace std;

bool debug = false;

#define show_debug(tip,message) if(debug){ cout << tip << message << endl;}

int detectFace( Mat img , string face_cascade_name){
	CascadeClassifier face_cascade;
	std::vector<Rect> faces;
    Mat img_gray;
	int face_size;
	int Y;

	if( !face_cascade.load( face_cascade_name ) ){ 
        printf("[error] can not load classifier file！[use -H for help]\n");
        return -1; 
    }
    cvtColor( img, img_gray, CV_BGR2GRAY );
    equalizeHist( img_gray, img_gray );
    face_cascade.detectMultiScale( img_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );
	face_size = faces.size();
	
	show_debug("detectFace:face size is ", face_size);
	
	if ( face_size > 0)
	{
		show_debug("detectFace:faces[0].y is ", faces[0].y);
		show_debug("detectFace:faces[0].height is ", faces[0].height);
		show_debug("detectFace:faces[0].width is ", faces[0].width);
		show_debug("detectFace:faces[face_size -1].y is ", faces[face_size -1].y);
		show_debug("detectFace:faces[face_size -1].height is ", faces[face_size -1].height);
		show_debug("detectFace:faces[face_size -1].width is ", faces[face_size -1].height);
		Y = faces[face_size -1].y - faces[face_size -1].height / 2;
		return Y < 0 ? 0 : Y;
	} else {
		return -1;
	}
}

int detectCharacter( Mat img ){
	int start_x = 0; //特征点X坐标开始位置 
	int end_x = 0; //特征点X坐标结束位置
	int section_index = 0; //Y坐标段数字索引
	map<int,int> section_num; //每个Y坐标段中特征点的数量
	int total = 0; //总共特征点数量
	int avg = 0; //每个Y坐标段的平均特征点数量
	int con_num = 4; //需要连续的阀值 
	int flag = 0;
	int counter = 0;
	int Y = 0;

	vector<KeyPoint> keypoints;

	cv::initModule_nonfree();//使用SIFT/SURF create之前，必须先initModule_<modulename>(); 

	Ptr<FeatureDetector> detector = FeatureDetector::create( "SURF" );

	if( detector.empty())
	{
		cout << "Can not create detector or descriptor exstractor or descriptor matcher of given types" << endl;
		return -1;
	}

	start_x = img.size().width / 5;
	end_x = start_x * 4;

	detector->detect( img, keypoints );
	for (vector<KeyPoint>::iterator i = keypoints.begin(); i != keypoints.end(); i++)
	{
		if (i->pt.x > start_x && i->pt.x < end_x)
		{
			section_index = (int)ceil(i->pt.y / 10);
			section_num[section_index] = section_num[section_index] + 1;
			total = total + 1;
		}
	}
	avg = total / section_num.size();

	/*
	for (map<int,int>::iterator i = section_num.begin(); i != section_num.end(); i++)
	{
		cout << i->first << "->" << i->second << endl;
	}
	*/

	show_debug("detectCharacter:avg is ", avg);

	//检测特征点分布是否均匀
	int slice_total = 10 ; 
	int slice_num = section_num.size() / slice_total;
	int slice_counter = 0;
	for (int m = 0; m < slice_total; m++)
	{
		for (int n = m * slice_num; n < (m+1) * slice_num; n++)
		{
			if ( section_num[n] >= avg )
			{
				slice_counter++;
				break;
			}
		}
	}
	if (slice_counter >= slice_total)
	{
		return -1;
	}

	//检测特征点主要分布区域[找最开始连续大于avg的Y]
	for (map<int,int>::iterator i = section_num.begin(); i != section_num.end(); i++)
	{
		if (i->second >= avg && flag == 0)
		{
			counter++;
		} else {
			counter = 0;
		}
		if (counter >= con_num && flag == 0)
		{
			Y = i->first;
			flag = 1;
		}
	}
	if (Y > con_num)
	{
		return (Y - con_num) * 10;
	}
	return Y * 10;
}

int main(int argc, char** argv)
{
	const double CLK_TCK = 1000.0;
	Mat image;
	Mat dest_image;
	Size tmp_size;
	int dest_width = 300;//用户输入
	int dest_height = 180;//用户输入
	float ratio_width = 0;
	float ratio_height = 0;
	float ratio = 0;
	int clip_top = 0;
	int clip_bottom = 0;
	int clip_left = 0;
	int clip_right = 0;
	string config_path = "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml";
	string source_path = "";
	string dest_path = "";
	int result = 0;
	clock_t start;
	clock_t clt;
	int param;

	while( (param = getopt(argc, argv, "Hms:d:c:w:h:")) != -1 )
	{
		if ( param == 's' ){
			source_path = optarg;
		} else if ( param == 'd' ) {
			dest_path = optarg;
		} else if ( param == 'm' ) {
			debug = true;
		}else if ( param == 'c' ) {
			config_path = optarg;
		}else if ( param == 'w' ) {
			sscanf (optarg, "%d", &dest_width);
		}else if ( param == 'h' ) {
			sscanf (optarg, "%d", &dest_height);
		}else if ( param == 'H')
		{
			cout << "Usage: exclip [options] [-s] <source_file> [--] [args...]" << endl;
			cout << "-s<path>	the path of source file" << endl;
			cout << "-d<path>	the path of destination file" << endl;
			cout << "-w<int>		the width of destination file. default value is 300" << endl;
			cout << "-h<int>		the height of destination file. default value is 180" << endl;
			cout << "-c<path>	the path of config file." << endl;
			cout << "		default path is /usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml" << endl;
			cout << "-m		open debug model" << endl;
			return 0;
		}
	}

	if ( source_path == "" )
	{
		cerr << "you should specify the path of source file.[use -H for help]" << endl;
		return 1;
	}
	if ( dest_path == "" )
	{
		cerr << "you should specify the path of destination file.[use -H for help]" << endl;
		return 1;
	}

    image = imread( source_path );
    if( !image.data ){
        printf("[error] do not load pic \n");
        return 1;
    }

	show_debug("start to resize ", "");
	
	ratio_width = (float)dest_width / image.size().width;
	ratio_height = (float)dest_height / image.size().height;

	show_debug("width of dest image ", dest_width);
	show_debug("height of dest image ", dest_height);
	show_debug("width of origin image ", image.size().width);
	show_debug("height of origin image ", image.size().height);
	show_debug("ratio of width ", ratio_width);
	show_debug("ratio of height ", ratio_height);

	if (ratio_width > ratio_height)
	{
		ratio = ratio_width;
	}
	else
	{
		ratio = ratio_height;
	}
	tmp_size = Size((int)(image.size().width * ratio), (int)(image.size().height * ratio));
	dest_image = Mat(tmp_size, CV_32S);
	resize(image, dest_image, tmp_size);

	show_debug("width of resize image ", dest_image.size().width);
	show_debug("height of resize image ", dest_image.size().height);

	show_debug("start to detectFace ", "");
	start = clock();

    result = detectFace( dest_image, config_path);

	clt = clock() - start;
	show_debug("detectFace cost time ", double(clt/CLK_TCK));
	show_debug("detectFace Y is ", result);
	show_debug("detectFace end", "");

	if (result == -1)
	{
		cout << "start to detectCahracter " << endl;
		show_debug("start to detectCahracter ", "");
		start = clock();

    	result = detectCharacter( dest_image );

		clt = clock() - start;
		show_debug("detectCharacter cost time ", double(clt/CLK_TCK));
		show_debug("detectCharacter Y is ", result);
		show_debug("detectCharacter end", "");
	}

	if (ratio_width > ratio_height) //原图片 宽度小于高度
	{
		if (result == -1)
		{
			clip_top = -((dest_image.size().height - dest_height) / 2);
			clip_bottom = clip_top;
		}else {
			if (dest_image.size().height - result >= dest_height)
			{
				clip_top = -result;
				clip_bottom = -(dest_image.size().height - result - dest_height);
			} else {
				clip_top = -(dest_image.size().height - dest_height);
			}
		}
	} else {
		clip_left = -((dest_image.size().width - dest_width) / 2);
		clip_right = clip_left;
	}

	show_debug("clip_top ", clip_top);
	show_debug("clip_bottom ", clip_bottom);
	show_debug("clip_left ", clip_left);
	show_debug("clip_right ", clip_right);
	dest_image.adjustROI(clip_top, clip_bottom, clip_left, clip_right); //Mat& Mat::adjustROI(int dtop, int dbottom, int dleft, int dright)

	imwrite(dest_path, dest_image);
	show_debug("write to file ", dest_path);
}
