#include <ros/ros.h>
#include <nav_msgs/Path.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl/conversions.h>
#include <pcl_ros/transforms.h>
#include <geometry_msgs/PoseStamped.h>

#include <rr_platform/speed.h>
#include <rr_platform/steering.h>

#include "random_sample_planner.h"
#include "annealing_planner.h"


std::unique_ptr<rr::Planner> planner;
std::unique_ptr<rr::DistanceChecker> distance_checker;

ros::Publisher speed_pub;
ros::Publisher steer_pub;
ros::Publisher path_pub;

sensor_msgs::PointCloud2ConstPtr last_map_msg;
bool is_new_msg;

void mapCallback(const sensor_msgs::PointCloud2ConstPtr& map) {
  last_map_msg = map;
  is_new_msg = true;
}

void processMap(const sensor_msgs::PointCloud2ConstPtr& map) {
  pcl::PCLPointCloud2 pcl_pc2;
  pcl_conversions::toPCL(*map, pcl_pc2);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::fromPCLPointCloud2(pcl_pc2, *cloud);

  for(auto point_it = cloud->begin(); point_it != cloud->end();) {
    if (distance_checker->GetCollision(*point_it)) {
      point_it = cloud->erase(point_it);
    } else {
      point_it++;
    }
  }

  if (cloud->empty()) {
    // Do not publish new commands
    ROS_WARN("environment map pointcloud is empty");
    return;
  }

  pcl::KdTreeFLANN<pcl::PointXYZ> kdtree(false);
  kdtree.setInputCloud(cloud);

  rr::PlannedPath plan = planner->Plan(kdtree);

  ROS_INFO_STREAM("Best path cost is " << plan.cost);

  rr_platform::speedPtr speedMSG(new rr_platform::speed);
  rr_platform::steeringPtr steerMSG(new rr_platform::steering);
  steerMSG->angle = plan.path[0].steer * 1.1;
  speedMSG->speed = plan.path[0].speed;
  steerMSG->header.stamp = ros::Time::now();
  speedMSG->header.stamp = ros::Time::now();
  speed_pub.publish(speedMSG);
  steer_pub.publish(steerMSG);

  if(path_pub.getNumSubscribers() > 0) {
    nav_msgs::Path pathMsg;
    
    for (auto path_point : plan.path) {
      geometry_msgs::PoseStamped ps;
      ps.pose.position.x = path_point.pose.x;
      ps.pose.position.y = path_point.pose.y;
      pathMsg.poses.push_back(ps);
    }

    pathMsg.header.frame_id = "base_footprint";
    path_pub.publish(pathMsg);
  }
}

template <typename T>
T getParamAssert(const ros::NodeHandle& nhp, const std::string& name) {
  T out;
  if (!nhp.getParam(name, out)) {
    ROS_ERROR_STREAM("[Planner] Param name " << name << " needs to be defined");
    std::exit(-1);
  }
  return out;
}

template <typename T>
std::vector<T> getNumericListParam(const ros::NodeHandle& nhp, const std::string& name, char delim) {
  auto listAsString = getParamAssert<std::string>(nhp, name);
  std::vector<T> out;

  std::stringstream ss(listAsString);
  std::string s;
  while (std::getline(ss, s, delim)) {
    out.push_back(static_cast<T>(std::stod(s)));
  }

  return out;
}

rr::RandomSamplePlanner::Params getRandomSampleParams(const ros::NodeHandle& nhp) {
  rr::RandomSamplePlanner::Params params;

  params.n_path_segments = getParamAssert<int>(nhp, "n_path_segments");
  params.steer_limits = getNumericListParam<double>(nhp, "steer_limits", ' ');
  params.steer_stddevs = getNumericListParam<double>(nhp, "steer_stddevs", ' ');

  params.path_similarity_cutoff = getParamAssert<double>(nhp, "path_similarity_cutoff");
  params.max_relative_cost = getParamAssert<double>(nhp, "max_relative_cost");
  params.k_dist = getParamAssert<double>(nhp, "k_dist");
  params.k_speed = getParamAssert<double>(nhp, "k_speed");

  params.n_control_samples = getParamAssert<int>(nhp, "n_control_samples");

  params.smoothing_array_size = getParamAssert<int>(nhp, "smoothing_array_size");

  params.obs_dist_slow_thresh = getParamAssert<double>(nhp, "obs_dist_slow_thresh");
  params.obs_dist_slow_ratio = getParamAssert<double>(nhp, "obs_dist_slow_ratio");

  return params;
}

rr::AnnealingPlanner::Params getAnnealingParams(const ros::NodeHandle& nhp) {
  rr::AnnealingPlanner::Params params;

  params.n_path_segments = static_cast<unsigned int>(getParamAssert<int>(nhp, "n_path_segments"));
  params.annealing_steps = static_cast<unsigned int>(getParamAssert<int>(nhp, "annealing_steps"));

  params.k_dist = getParamAssert<double>(nhp, "k_dist");
  params.k_speed = getParamAssert<double>(nhp, "k_speed");
  params.k_final_pose = getParamAssert<double>(nhp, "k_final_pose");
  params.backwards_penalty = getParamAssert<double>(nhp, "backwards_penalty");
  params.collision_penalty = getParamAssert<double>(nhp, "collision_penalty");
  params.max_steering = getParamAssert<double>(nhp, "max_steering");
  params.acceptance_scale = getParamAssert<double>(nhp, "acceptance_scale");

  params.temperature_start = getParamAssert<double>(nhp, "temperature_start");
  params.temperature_end = getParamAssert<double>(nhp, "temperature_end");

  return params;
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "planner");

  ros::NodeHandle nh;
  ros::NodeHandle nhp("~");

  rr::CenteredBox box;
  box.length_front = getParamAssert<double>(nhp, "collision_dist_front");
  box.length_back = getParamAssert<double>(nhp, "collision_dist_back");
  box.width_left = box.width_right = getParamAssert<double>(nhp, "collision_dist_side");

  rr::CenteredBox map_dimensions;
  map_dimensions.length_front = 7;
  map_dimensions.length_back = 5;
  map_dimensions.width_right = map_dimensions.width_left = 6;

  distance_checker = std::make_unique<rr::DistanceChecker>(box, map_dimensions);

  auto wheel_base = getParamAssert<double>(nhp, "wheel_base");
  auto lateral_accel = getParamAssert<double>(nhp, "lateral_accel");
  auto distance_increment = getParamAssert<double>(nhp, "distance_increment");
  auto max_speed = getParamAssert<double>(nhp, "max_speed");
  auto steering_speed = getParamAssert<double>(nhp, "steering_speed");
  auto segment_sections = getNumericListParam<int>(nhp, "segment_sections", ' ');

  rr::BicycleModel model(wheel_base, lateral_accel, distance_increment, max_speed, steering_speed, segment_sections);

  auto obstacle_cloud_topic = getParamAssert<std::string>(nhp, "input_cloud_topic");
  auto planner_type = getParamAssert<std::string>(nhp, "planner_type");

  if (planner_type == "random_sample"){
    auto params = getRandomSampleParams(nhp);
    planner = std::make_unique<rr::RandomSamplePlanner>(*distance_checker, model, params);
  } else if (planner_type == "annealing") {
    auto params = getAnnealingParams(nhp);
    planner = std::make_unique<rr::AnnealingPlanner>(*distance_checker, model, params);
  } else {
    ROS_ERROR_STREAM("[Planner] Error: unknown planner type \"" << planner_type << "\"");
    std::exit(-2);
  }

  auto map_sub = nh.subscribe(obstacle_cloud_topic, 1, mapCallback);
  speed_pub = nh.advertise<rr_platform::speed>("plan/speed", 1);
  steer_pub = nh.advertise<rr_platform::steering>("plan/steering", 1);
  path_pub = nh.advertise<nav_msgs::Path>("plan/path", 1);

  ROS_INFO("Planner initialized");

  ros::Rate rate(30);
  is_new_msg = false;
  while (ros::ok()) {
    if (is_new_msg) {
      processMap(last_map_msg);
    }

    ros::spinOnce();
    rate.sleep();

    ROS_INFO_STREAM("Planner took " << rate.cycleTime() << " seconds");
  }

  return 0;
}
