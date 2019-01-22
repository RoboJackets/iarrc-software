#include "color_detector.h"
#include <pluginlib/class_list_macros.h>
#include <rr_iarrc/hsv_tuned.h>

using namespace std;
using namespace ros;
using namespace cv;

namespace rr_iarrc {

    double white_h_low = 255; 
    double white_s_low = 255; 
    double white_v_low = 255; 
    double white_h_high = 255;    
    double white_s_high = 255;
    double white_v_high = 255;


    void color_detector::ImageCB(const sensor_msgs::ImageConstPtr &msg) {

        cv_bridge::CvImageConstPtr cv_ptr;

        try {
            cv_ptr = cv_bridge::toCvShare(msg, "bgr8");
        } catch (cv_bridge::Exception &e) {
            ROS_ERROR("CV-Bridge error: %s", e.what());
            return;
        }

        const Mat &frameBGR = cv_ptr->image;
        Mat frameBlurred;
        GaussianBlur(frameBGR, frameBlurred, Size{0,0}, 1);
        Mat frameHSV;
        cvtColor(frameBlurred, frameHSV, CV_BGR2HSV);

        const Mat frame_masked = frameHSV(mask);

        Mat output_white = Mat::zeros(mask.height, mask.width, CV_8UC1);

        auto white_low = Scalar(white_h_low, white_s_low, white_v_low);
        auto white_high = Scalar(white_h_high, white_s_high, white_v_high);
        inRange(frame_masked, white_low, white_high, output_white);

        erode(output_white, output_white, erosion_kernel_white);
        dilate(output_white, output_white, dilation_kernel_white);

        Mat output = Mat::zeros(frameHSV.rows, frameHSV.cols, CV_8UC1);
        Mat output_masked = output(mask);
        output_masked.setTo(255, output_white);

        img_pub.publish(cv_bridge::CvImage{std_msgs::Header(), "mono8", output}.toImageMsg());
    }

    void hsvTunedCallback(const rr_iarrc::hsv_tuned::ConstPtr &msg){
        white_h_low = msg->white_h_low;
        white_s_low = msg->white_s_low;
        white_v_low = msg->white_v_low;

        white_h_high = msg->white_h_high;
        white_s_high = msg->white_s_high;
        white_v_high = msg->white_v_high;

        ROS_INFO("Set HSV limits");
    }

    void color_detector::onInit() {
        NodeHandle nh = getNodeHandle();
        NodeHandle pnh = getPrivateNodeHandle();
        image_transport::ImageTransport it(nh);

        ros::Subscriber hsv_tuned_sub = nh.subscribe("/hsv_tuned", 1, hsvTunedCallback);

        mask = Rect(0, 482, 1280, 482); // bottom half of the image; x, y, w, h

        erosion_kernel_white = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        dilation_kernel_white = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));

        img_sub = it.subscribe("/camera/image_color_rect", 1, &color_detector::ImageCB, this);
        img_pub = it.advertise("/lines_detection_img", 1);

        ROS_INFO("Color Detector ready!");
        ros::spin();        

    }

}

PLUGINLIB_EXPORT_CLASS(rr_iarrc::color_detector, nodelet::Nodelet)