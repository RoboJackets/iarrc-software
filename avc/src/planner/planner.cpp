#include "planner.h"
#include <cmath>

using namespace std;

planner::planner() {
	ros::NodeHandle nh;
	ros::NodeHandle pnh("~");

	pnh.getParam("speed_increment", SPEED_INCREMENT);
	pnh.getParam("angle_increment", ANGLE_INCREMENT);
	pnh.getParam("time_increment", TIME_INCREMENT);
	pnh.getParam("search_radius", SEARCH_RADIUS);
	pnh.getParam("distance_increment", DISTANCE_INCREMENT);
	map_sub = nh.subscribe("map", 1, &planner::mapCb, this);
	speed_pub = nh.advertise<rr_platform::speed>("speed", 1);
	steer_pub = nh.advertise<rr_platform::steering>("steering", 1);
	path_pub = nh.advertise<nav_msgs::Path>("path", 1);
}

planner::pose planner::calculateStep(double x, double y, double theta, double velocity, double steer_angle, double timestep) {
	if (abs(steer_angle) < 1e-6) {
		deltaX = velocity * timestep;
		deltaY = 0;
		deltaTheta = 0;
	} else {
		double turn_radius = constants::wheel_base / sin(abs(steer_angle) * PI / 180.0);
		double temp_theta = velocity * timestep / turn_radius;
		deltaX = turn_radius * cos(PI / 2 - temp_theta);
		deltaY;
		if (steer_angle < 0) {
			deltaY = turn_radius - turn_radius * sin(PI / 2 - temp_theta);
		} else {
			deltaY = -(turn_radius - turn_radius * sin(PI / 2 - temp_theta));
		}
		deltaTheta = velocity / constants::wheel_base * sin(-steer_angle * PI / 180.0) * 180 / PI * timestep;
	}
	pose p;
	p.x = x + (deltaX * cos(theta * PI / 180.0) - deltaY * sin(theta * PI / 180.0));
	p.y = y + (deltaX * sin(theta * PI / 180.0) + deltaY * cos(theta * PI / 180.0));
	p.theta = theta + deltaTheta;
	return p;
}

double planner::calculatePathCost(double velocity, double steer_angle, 
								  pcl::KdTreeFLANN<pcl::PointXYZ> kdtree) {
	double cost = 0.0;
	double length = velocity * TIMESTEP;
	//int nSteps = 0;
	for(double t = 0; t < length; t += DISTANCE_INCREMENT) {
		pose step = calculateStep(0, 0, 0, velocity, steer_angle, t / velocity);

		cost += costAtPose(step, kdtree);
		//nSteps += 1;
	}
	// ROS_INFO("path used %d steps", nSteps);
	return cost * max(1.0, log(abs(steer_angle)));
}

double planner::costAtPose(pose step, pcl::KdTreeFLANN<pcl::PointXYZ> kdtree) {
	pcl::PointXYZ searchPoint(step.x , step.y ,0);
	vector<int> pointIdxRadiusSearch(1);
	vector<float> pointRadiusSquaredDistance(1);
	int nResults = kdtree.nearestKSearch(searchPoint, 1, pointIdxRadiusSearch, 
										 pointRadiusSquaredDistance);

	double distSqr = pointRadiusSquaredDistance[0];
	double dist = sqrt(distSqr);

	if(nResults == 0) return 0; //is blind
	if(dist < SEARCH_RADIUS) return 100.0; //collision
	return (1.0 / distSqr);
}

void planner::mapCb(const sensor_msgs::PointCloud2ConstPtr& map) {
	pcl::PCLPointCloud2 pcl_pc2;
	pcl_conversions::toPCL(*map, pcl_pc2);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::fromPCLPointCloud2(pcl_pc2, *cloud);

	pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;
	kdtree.setInputCloud(cloud);

	double lowest_cost = numeric_limits<double>::max();
	double best_path_speed = 0;
	double best_path_angle = 0;
	double cost = 0;
	//int nTraj = 0;
	for (double speed = MIN_SPEED; speed <= MAX_SPEED; speed += SPEED_INCREMENT) {
		for (double angle = MAX_STEER_ANGLE; angle >= -MAX_STEER_ANGLE; angle -= ANGLE_INCREMENT) {
			if(cloud->empty()) cost = 0;
			else cost = calculatePathCost(speed, angle, kdtree);

			if(cost < lowest_cost) {
				best_path_speed = speed;
				best_path_angle = angle;
				lowest_cost = cost;
			} else if (cost == lowest_cost) {

				if(speed > best_path_speed) {
					best_path_speed = speed;
					best_path_angle = angle;
				} else if(speed == best_path_speed && abs(angle) < abs(best_path_angle)) {
					best_path_speed = speed;
					best_path_angle = angle;
				}

				/*if (std::abs(angle) < std::abs(best_path_angle)) {
					best_path_speed = speed;
					best_path_angle = angle;
				} else if (speed > best_path_speed) {
					best_path_speed = speed;
					best_path_angle = angle;
				}*/
			}
			//nTraj += 1;
		}
	}
	rr_platform::speedPtr speedMSG(new rr_platform::speed);
	rr_platform::steeringPtr steerMSG(new rr_platform::steering);
	speedMSG->speed = best_path_speed;
	steerMSG->angle = best_path_angle;
	speed_pub.publish(speedMSG);
	steer_pub.publish(steerMSG);
	nav_msgs::Path path;
	for(double t = 0; t < best_path_speed * TIMESTEP; t += DISTANCE_INCREMENT) {
		pose step = calculateStep(0, 0, 0, best_path_speed, best_path_angle, t / best_path_speed);
		geometry_msgs::PoseStamped p;
		p.pose.position.x = step.x;
		p.pose.position.y = step.y;
		path.poses.push_back(p);
	}
	path.header.frame_id = "ground";
	//ROS_INFO_STREAM(path.poses.size());
	path_pub.publish(path);

	//ROS_INFO("tried %d trajectories", nTraj);
}


int main(int argc, char** argv) {
	ros::init(argc, argv, "planner");
	//ROS_INFO("hi");
	planner plan;
	//ROS_INFO("bye");
	ros::spin();
	return 0;
}