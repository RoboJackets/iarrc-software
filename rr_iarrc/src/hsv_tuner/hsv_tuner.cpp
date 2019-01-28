#include <ros/ros.h>
#include <ros/package.h>
#include <rr_iarrc/hsv_tuned.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>
#include <ctime>

using namespace cv;
using namespace std;

ros::Publisher hsv_pub;
rr_iarrc::hsv_tuned hsv_msg;

const int hsv_slider_max = 255;
const int saving_slider_max = 1;

int32_t white_h_low_slider;     
int32_t white_s_low_slider;
int32_t white_v_low_slider; 
int32_t white_h_high_slider;        
int32_t white_s_high_slider;
int32_t white_v_high_slider;

int32_t saving_slider =  0;

 std::string package_path = ros::package::getPath("rr_iarrc");

// Callback for trackbar
void on_trackbar( int, void* ){
    hsv_msg.white_h_low = white_h_low_slider;
    hsv_msg.white_s_low = white_s_low_slider;
    hsv_msg.white_v_low = white_v_low_slider;

    hsv_msg.white_h_high = white_h_high_slider;
    hsv_msg.white_s_high = white_s_high_slider;
    hsv_msg.white_v_high = white_v_high_slider;

    rr_iarrc::hsv_tuned publishable_copy = hsv_msg;
    publishable_copy.header.stamp = ros::Time::now();
    hsv_pub.publish(publishable_copy);
}


void loadValues(){
    string line;
    int value; 
    ifstream myfile (package_path + "/saved_hsv/example.txt");
    if (myfile.is_open()){
        ROS_INFO("Loading HSV values.");
        getline(myfile,line);
        value = stoi(line);
        white_h_low_slider = value;

        getline(myfile,line);
        value = stoi(line);
        white_s_low_slider = value;

        getline(myfile,line);
        value = stoi(line);
        white_v_low_slider = value;

        getline(myfile,line);
        value = stoi(line);
        white_h_high_slider = value;

        getline(myfile,line);
        value = stoi(line);
        white_s_high_slider = value;

        getline(myfile,line);
        value = stoi(line);
        white_v_high_slider = value;

        myfile.close();
    }
    else{
        cout << "Unable to load HSV values."; 
    }
}

void saveValues(int, void*){

    if(saving_slider == 1){
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];
        time (&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
        std::string date_time(buffer);

        ofstream myfile (package_path + "/saved_hsv/"+ date_time+".txt", ios::out); 

        if (myfile.is_open()){
            ROS_INFO("Saving HSV values");   
            myfile << white_h_low_slider << endl;
            myfile << white_s_low_slider << endl;
            myfile << white_v_low_slider << endl;
            myfile << white_h_high_slider << endl;
            myfile << white_s_high_slider << endl;  
            myfile << white_v_high_slider << endl;
            myfile.close();
            saving_slider = 0; 
        }else{
            ROS_INFO("Unable to open file");
        } 
    } 
}

    

int main(int argc, char **argv) {

    ros::init(argc, argv, "hsv_tuner");
    ros::NodeHandle handle;
    ros::NodeHandle private_handle("~");
    
    hsv_pub = handle.advertise<rr_iarrc::hsv_tuned>("/hsv_tuned", 1);

    rr_iarrc::hsv_tuned hsv_tuned_msg;
    hsv_tuned_msg.header.frame_id = "hsv_tuned";
    ROS_INFO("Running HSV tuner panel.");

    loadValues();

    hsv_msg.white_h_low = white_h_low_slider;
    hsv_msg.white_s_low = white_s_low_slider;
    hsv_msg.white_v_low = white_v_low_slider;
    hsv_msg.white_h_high = white_h_high_slider;
    hsv_msg.white_s_high = white_s_high_slider;
    hsv_msg.white_v_high = white_v_high_slider;

    /// Create Windows
    namedWindow("HSV Tuner", CV_GUI_NORMAL | CV_WINDOW_FREERATIO);

    /// Create Trackbars
    char white_h_low[50];
    sprintf( white_h_low, "White Hue Low: %d", hsv_slider_max);
    char white_s_low[50];
    sprintf( white_s_low, "White Saturation Low: %d", hsv_slider_max);
    char white_v_low[50];
    sprintf( white_v_low, "White Value Low: %d", hsv_slider_max);
    char white_h_high[50];
    sprintf( white_h_high, "White Hue High: %d", hsv_slider_max);
    char white_s_high[50];
    sprintf( white_s_high, "White Saturation High: %d", hsv_slider_max);
    char white_v_high[50];
    sprintf( white_v_high, "White Value High: %d", hsv_slider_max);
    char saving_slider_label[50];
    sprintf(saving_slider_label, "Save: %d", saving_slider_max);

    createTrackbar(white_h_low, "HSV Tuner", &white_h_low_slider, hsv_slider_max, on_trackbar);
    createTrackbar(white_s_low, "HSV Tuner", &white_s_low_slider, hsv_slider_max, on_trackbar);
    createTrackbar(white_v_low, "HSV Tuner", &white_v_low_slider, hsv_slider_max, on_trackbar);
    createTrackbar(white_h_high, "HSV Tuner", &white_h_high_slider, hsv_slider_max, on_trackbar);
    createTrackbar(white_s_high, "HSV Tuner", &white_s_high_slider, hsv_slider_max, on_trackbar);
    createTrackbar(white_v_high, "HSV Tuner", &white_v_high_slider, hsv_slider_max, on_trackbar);
    createTrackbar(saving_slider_label, "HSV Tuner", &saving_slider, saving_slider_max, saveValues);

    while(ros::ok()) {
        ros::spinOnce();       
        waitKey(1);
    }
    return 0;
}