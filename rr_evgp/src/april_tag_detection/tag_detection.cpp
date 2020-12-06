//
// Created by Charles Jenkins on 11/27/20.
//

#include "tag_detection.h"
#include "tf/LinearMath/Transform.h"

tag_detection::tag_detection(ros::NodeHandle *nh, std::string camera_frame, const std::string &pointcloud,
                             const std::string &tag_detections_topic, std::string destination_frame) : tfListener(tfBuffer) {
    this->camera_frame = std::move(camera_frame);
    this->destination_frame = std::move(destination_frame);
    sub_detections = nh->subscribe(tag_detections_topic, 1, &tag_detection::callback, this);
    pub_pointcloud = nh->advertise<sensor_msgs::PointCloud2>(pointcloud, 1);
    tf2_ros::TransformListener listener(tfBuffer);
}

void tag_detection::callback(const apriltag_ros::AprilTagDetectionArray::ConstPtr &msg) {
    auto msgs = msg->detections;
    draw_opponents(msg);
    for (const auto &message : msgs) {  // Iterate through all discovered April Tags
        ROS_INFO_STREAM((message.id[0]));
    }
}

void tag_detection::draw_opponents(const apriltag_ros::AprilTagDetectionArray::ConstPtr &msg) {
    opponent_cloud.clear();
    auto msgs = msg->detections;
    for (const auto &message : msgs) {  // Iterate through all discovered April Tags
        draw_opponent(message.id[0], message.pose.pose.pose);
    }
    publishPointCloud(opponent_cloud);
}

void tag_detection::publishPointCloud(pcl::PointCloud<pcl::PointXYZ> &cloud) {
    sensor_msgs::PointCloud2 outmsg;
    pcl::toROSMsg(cloud, outmsg);
    outmsg.header.frame_id = destination_frame;
    pub_pointcloud.publish(outmsg);
}

void tag_detection::draw_opponent(const int &id, geometry_msgs::Pose_<std::allocator<void>> april_tag_center) {
    int side_length = 2;
    int num_points = 10;
    double ratio = (double) side_length / num_points;
    double start_x = april_tag_center.position.x - side_length / 2.0;
    geometry_msgs::TransformStamped stamped =
            tfBuffer.lookupTransform(destination_frame, camera_frame, ros::Time(0));
    for (int x = 0; x < num_points; x++) {
        for (int z = 0; z < num_points; z++) {
            Eigen::Vector3d vector((float) (start_x + x * ratio), 0,
                                   (float) (april_tag_center.position.z + z * ratio));
            Eigen::Vector3d output;

            tf2::doTransform(vector, output, stamped);
            opponent_cloud.push_back(pcl::PointXYZ(output.x(), output.y(), 0));
        }
    }
}
