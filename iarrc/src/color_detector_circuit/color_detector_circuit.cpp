#include <ros/ros.h>
#include <std_msgs/Header.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <cv_bridge/cv_bridge.h>

using namespace std;
using namespace ros;
using namespace cv;

using uchar = unsigned char;

Publisher img_pub;
Publisher obstacle_pub;

//Mat mask;
//Mat mask_horizontal;

Mat findWhiteLines(const Mat& image) {
    Mat frame;
    image.copyTo(frame);
    frame *= 1.25;

    for(int r = 0; r < frame.rows; r++) {
        uchar* row = frame.ptr<uchar>(r);
        for(int c = 0; c < frame.cols * frame.channels(); c+= frame.channels()) {
            uchar& blue = row[c];
            uchar& green = row[c+1];
            uchar& red = row[c+2];

            if(blue > 220 && green > 220 && red > 220) {
                blue = green = red = 255;
            } else {
                blue = green = red = 0;
            }
        }
    }
    return frame;
}

Mat findYellowLines(const Mat& image) {
    Mat frame;
    image.copyTo(frame);
    
    cvtColor(frame, frame, CV_BGR2HSV);

    Mat output(image.rows, image.cols, CV_8UC3);

    for(int r = 0; r < frame.rows; r++) {
        uchar* row = frame.ptr<uchar>(r);
        uchar* out_row = output.ptr<uchar>(r);
        for(int c = 0; c < frame.cols * frame.channels(); c+= frame.channels()) {
            uchar& H = row[c];
            uchar& S = row[c+1];
            uchar& V = row[c+2];

            if(abs(H - 30) < 10 && S > 50 && V > 30) {
            //if(abs(H - 30) < 0 && S > 50 && V > 50) {       //dragrace_setting
                out_row[c] = 0;
                out_row[c+1] = out_row[c+2] = 255;
            } else {
                out_row[c] = out_row[c+1] = out_row[c+2] = 0;
            }
        }
    }

    auto kernel_size = 3;
    Mat erosion_kernel = getStructuringElement(MORPH_CROSS, Size(kernel_size, kernel_size));

    erode(output, output, erosion_kernel);

    return output;
}

Mat findOrange(const Mat& image) {
    Mat frame;
    image.copyTo(frame);

    cvtColor(frame, frame, CV_BGR2HSV);

    Mat output(image.rows, image.cols, CV_8UC3);

    for(int r = 0; r < frame.rows; r++) {
        uchar* row = frame.ptr<uchar>(r);
        uchar* out_row = output.ptr<uchar>(r);
        for(int c = 0; c < frame.cols * frame.channels(); c+= frame.channels()) {
            uchar& H = row[c];
            uchar& S = row[c+1];
            uchar& V = row[c+2];

            if(abs(H - 15) < 5 && S > 70 && V > 40) {
                out_row[c] = 0;
                out_row[c+1] = 127;
                out_row[c+2] = 255;
            } else {
                out_row[c] = out_row[c+1] = out_row[c+2] = 0;
            }
        }
    }

    auto kernel_size = 11;
    Mat erosion_kernel = getStructuringElement(MORPH_CROSS, Size(kernel_size, kernel_size));

    erode(output, output, erosion_kernel);

    return output;
}

Mat findBlueLines(const Mat& image) {
    Mat frame;
    image.copyTo(frame);

    image *= 1.2;

    cvtColor(frame, frame, CV_BGR2HSV);

    Mat output(image.rows, image.cols, CV_8UC3);

    for(int r = 0; r < frame.rows; r++) {
        uchar* row = frame.ptr<uchar>(r);
        uchar* out_row = output.ptr<uchar>(r);
        for(int c = 0; c < frame.cols * frame.channels(); c+= frame.channels()) {
            uchar& H = row[c];
            uchar& S = row[c+1];
            uchar& V = row[c+2];

            if(abs(H - 108) < 5 && S > 50) {
                out_row[c] = 255;
                out_row[c+1] = out_row[c+2] = 0;
            } else {
                out_row[c] = out_row[c+1] = out_row[c+2] = 0;
            }
        }
    }

    return output;
}

void ImageCB(const sensor_msgs::ImageConstPtr& msg) {

    cv_bridge::CvImagePtr cv_ptr;
	Mat frame;
	Mat output;
	
	try {
		cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
	} catch (cv_bridge::Exception& e) {
		ROS_ERROR("CV-Bridge error: %s", e.what());
		return;
	}
	
	frame = cv_ptr->image;

//    frame = frame.mul(mask);

    Mat white_lines = findWhiteLines(frame);

    Mat blue_lines = findBlueLines(frame);

    Mat yellow_lines = findYellowLines(frame);

    Mat orange_blobs = findOrange(frame);

    output = white_lines + blue_lines + yellow_lines + orange_blobs;

    sensor_msgs::Image outmsg;
    cv_ptr->image = output;
	cv_ptr->encoding = "bgr8";
	cv_ptr->toImageMsg(outmsg);
	img_pub.publish(outmsg);

    Mat output_gray;
    cvtColor(output, output_gray, CV_BGR2GRAY);
    sensor_msgs::Image outmsg2;
    cv_ptr->image = output_gray;
    cv_ptr->encoding = "mono8";
    cv_ptr->toImageMsg(outmsg2);
    obstacle_pub.publish(outmsg2);
}

int main(int argc, char** argv) {

    init(argc, argv, "color_detector_circuit");

    NodeHandle nh;

    //Blacks out front bumper from image
//    vector<Mat> mask_hsegments = {
//        Mat(40,205,CV_8UC3, CV_RGB(1,1,1)),
//        Mat::zeros(40,230,CV_8UC3),
//        Mat(40,205,CV_8UC3, CV_RGB(1,1,1))
//    };
//
//    hconcat(mask_hsegments, mask_horizontal);
//    vector<Mat> mask_segments = {
//        Mat::zeros(200,640,CV_8UC3),
//        Mat(200,640,CV_8UC3, CV_RGB(1,1,1)),
//        mask_horizontal,
//        Mat::zeros(40,640,CV_8UC3)
//    };
//
//    vconcat(mask_segments, mask);

    ros::Subscriber img_saver_sub = nh.subscribe("/camera/image_rect", 1, ImageCB);
	
	img_pub = nh.advertise<sensor_msgs::Image>(string("/colors_img"), 1);
    obstacle_pub = nh.advertise<sensor_msgs::Image>(string("/obstacles_img"), 1);
        
    spin();

    return 0;

}
