#include <ros/ros.h>
#include <rr_platform/speed.h>
#include <rr_platform/steering.h>
#include <rr_platform/chassis_state.h>
#include <sensor_msgs/Imu.h>
#include <tf/transform_datatypes.h>
#include <math.h>

ros::Publisher speedPub;
ros::Publisher steeringPub;

double currentYaw = 0;
const double kP = 0;

bool go = false;

void imuCallback(const sensor_msgs::ImuConstPtr& msg) {
    auto orientation = msg->orientation;
    tf::Quaternion tf_quat;
    tf::quaternionMsgToTF(orientation, tf_quat);
    auto currentYaw = tf::getYaw(tf_quat);
    ROS_INFO_STREAM(currentYaw);
}

void chassisCB(const rr_platform::chassis_stateConstPtr& msg) {
    go = msg->mux_automatic;
}

void publishSpeed(const float &desiredSpeed) {
    rr_platform::speed speedMsg;
    speedMsg.header.stamp = ros::Time::now();
    speedMsg.speed = desiredSpeed;
    speedPub.publish(speedMsg);
    ROS_INFO("published");
    rr_platform::steering steeringMsg;
    steeringMsg.header.stamp = ros::Time::now();
    steeringMsg.angle = /*-currentYaw * kP*/0;
    steeringPub.publish(steeringMsg);
}

int main(int argc, char* argv[]) {
    ros::init(argc, argv, "qualify");
    ros::NodeHandle nh;
    speedPub = nh.advertise<rr_platform::speed>("/speed", 1);
    steeringPub = nh.advertise<rr_platform::steering>("/steering", 1);
    ros::Subscriber chassisSub = nh.subscribe("/chassis_state", 1, chassisCB);
    ros::Subscriber imuSub = nh.subscribe("/imu/data_raw", 1, imuCallback);
    while (!go && ros::ok()) {
        ros::spinOnce();
    }
    publishSpeed(2.0);
    ros::Duration(5).sleep();
    publishSpeed(0.0);
    return 0;
}
