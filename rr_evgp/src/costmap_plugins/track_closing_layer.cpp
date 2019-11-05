#include <valarray>

#include <costmap_2d/GenericPluginConfig.h>
#include <costmap_2d/layer.h>
#include <dynamic_reconfigure/server.h>
#include <parameter_assertions/assertions.h>
#include <pluginlib/class_list_macros.h>
#include <ros/ros.h>
#include <opencv2/opencv.hpp>

#include "skeletonize.hpp"

namespace rr {

class TrackClosingLayer : public costmap_2d::Layer {
  public:
    TrackClosingLayer() = default;

    void updateBounds(double robot_x, double robot_y, double robot_yaw, double* min_x, double* min_y, double* max_x,
                      double* max_y) override;

    void updateCosts(costmap_2d::Costmap2D& master_grid, int min_cell_x, int min_cell_y, int max_cell_x,
                     int max_cell_y) override;

  protected:
    void onInitialize() override;

  private:
    std::unique_ptr<dynamic_reconfigure::Server<costmap_2d::GenericPluginConfig>> dsrv_;
    int branch_pruning_size_;
    int dilate_size_;
    double min_enclosing_radius_;
    int reg_bounding_box_size_;
    double extrapolate_distance_;
    double max_angle_between_lines_;
    double max_dist_between_lines_;

    void reconfigureCB(const costmap_2d::GenericPluginConfig& config, uint32_t level) {
        enabled_ = config.enabled;
    }

    using WallEndpointContext = std::tuple<cv::Point, cv::Point, double, double>;
    bool IsConnectable(const WallEndpointContext& current, const WallEndpointContext& prev);
};

PLUGINLIB_EXPORT_CLASS(rr::TrackClosingLayer, costmap_2d::Layer)

// ******************* Implementation ********************** //

void TrackClosingLayer::onInitialize() {
    dsrv_ = std::make_unique<dynamic_reconfigure::Server<costmap_2d::GenericPluginConfig>>();
    dsrv_->setCallback(boost::bind(&TrackClosingLayer::reconfigureCB, this, _1, _2));

    ros::NodeHandle private_nh("~" + getName());
    assertions::getParam(private_nh, "branch_pruning_size", branch_pruning_size_, { assertions::greater<int>(0) });
    assertions::getParam(private_nh, "dilate_size", dilate_size_, { assertions::greater<int>(0) });
    assertions::getParam(private_nh, "min_enclosing_radius", min_enclosing_radius_, { assertions::greater<double>(0) });
    assertions::getParam(private_nh, "reg_bounding_box_size", reg_bounding_box_size_, { assertions::greater<int>(0) });
    assertions::getParam(private_nh, "extrapolate_distance", extrapolate_distance_, { assertions::greater<double>(0) });
    assertions::getParam(private_nh, "max_angle_between_lines", max_angle_between_lines_,
                         { assertions::greater<double>(0) });
    assertions::getParam(private_nh, "max_dist_between_lines", max_dist_between_lines_,
                         { assertions::greater<double>(0) });
}

void TrackClosingLayer::updateBounds(double robot_x, double robot_y, double robot_yaw, double* min_x, double* min_y,
                                     double* max_x, double* max_y) {
    if (!enabled_) {
        return;
    }

    const costmap_2d::Costmap2D* costmap = layered_costmap_->getCostmap();
    *min_x = std::min(*min_x, costmap->getOriginX());
    *min_y = std::min(*min_y, costmap->getOriginY());
    *max_x = std::max(*max_x, costmap->getOriginX() + costmap->getSizeInMetersX());
    *max_y = std::max(*max_y, costmap->getOriginY() + costmap->getSizeInMetersY());
}

template <int N>
double norm(const cv::Vec<double, N>& x) {
    return std::sqrt(x.dot(x));
}

cv::Point2d intersect_point_line(const cv::Point2d& p, const cv::Point2d& l1, const cv::Point2d& l2, bool bound) {
    cv::Vec2d n = l2 - l1;
    n /= norm(n);
    double t = (p - l1).dot(n);
    cv::Vec2d x = cv::Vec2d(l1) + n * t;

    if (bound && (norm(x - cv::Vec2d(l1)) + norm(x - cv::Vec2d(l2)) > norm(l2 - l1))) {
        x = (norm(p - l1) < norm(p - l2)) ? l1 : l2;
    }

    return x;
}

bool TrackClosingLayer::IsConnectable(const WallEndpointContext& current, const WallEndpointContext& prev) {
    const auto& [endpntA, pntA, vxA, vyA] = current;
    const auto& [endpntB, pntB, vxB, vyB] = prev;

    ROS_INFO_STREAM("comparing " << endpntA << " with " << endpntB);

    double angle_between_lines = std::acos(vxA * vxB + vyA * vyB) * 180.0 / M_PI;
    angle_between_lines = std::abs(angle_between_lines - 180.0);
    ROS_INFO_STREAM("angle " << angle_between_lines);
    cv::Point closest_endB_to_lineA =
          intersect_point_line(endpntB, pntA, pntA + cv::Point(vxA * 1000, vyA * 1000), true);
    cv::Point closest_endA_to_lineB =
          intersect_point_line(endpntA, pntB, pntB + cv::Point(vxA * 1000, vyA * 1000), true);
    double dist_a = norm(closest_endA_to_lineB - endpntA);
    double dist_b = norm(closest_endB_to_lineA - endpntB);
    ROS_INFO_STREAM("dists " << dist_a << " " << dist_b);
    return angle_between_lines <= max_angle_between_lines_ && dist_a < max_dist_between_lines_ &&
           dist_b < max_dist_between_lines_;
}

void TrackClosingLayer::updateCosts(costmap_2d::Costmap2D& master_grid, int min_cell_x, int min_cell_y, int max_cell_x,
                                    int max_cell_y) {
    if (!enabled_) {
        return;
    }

    // populate OpenCV Mat as grid
    cv::Mat mat_grid(master_grid.getSizeInCellsX(), master_grid.getSizeInCellsY(), CV_8UC1);
    uint8_t* charMap = master_grid.getCharMap();
    for (int r = 0; r < mat_grid.rows; r++) {
        for (int c = 0; c < mat_grid.cols; c++) {
            mat_grid.at<uint8_t>(r, c) = (charMap[r * mat_grid.cols + c] == costmap_2d::LETHAL_OBSTACLE) ? 255 : 0;
        }
    }

    const int max_debug_img_slots = 4;
    cv::Mat display_img(mat_grid.rows, mat_grid.cols * max_debug_img_slots, CV_8UC1, cv::Scalar(127));

    auto blit_gray_debug_img = [&](const cv::Mat& m, int slot) {
        cv::Rect roi(0, 0, mat_grid.cols, mat_grid.rows);
        m.copyTo(display_img(roi));
    };

    blit_gray_debug_img(mat_grid, 0);

    auto dilate_kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(dilate_size_, dilate_size_));
    cv::morphologyEx(mat_grid, mat_grid, cv::MORPH_DILATE, dilate_kernel);
    blit_gray_debug_img(mat_grid, 1);

    auto skel = rr::thinObstacles(mat_grid);
    auto branches = rr::removeSmallBranches(skel, branch_pruning_size_);
    blit_gray_debug_img(skel, 2);
    blit_gray_debug_img(branches, 3);

    cv::Mat debug_img;
    cv::cvtColor(branches * 255, debug_img, CV_GRAY2BGR);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(branches, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
    std::vector<WallEndpointContext> endpnt_line;

    for (size_t contour_id = 0; contour_id < contours.size(); contour_id++) {
        const std::vector<cv::Point>& contour = contours[contour_id];

        cv::Point2f center;
        float radius = 0;
        cv::minEnclosingCircle(contour, center, radius);

        if (radius < min_enclosing_radius_)
            continue;

        cv::Mat contour_img(skel.rows, skel.cols, CV_8UC1, cv::Scalar(0));
        cv::drawContours(contour_img, contours, contour_id, cv::Scalar(1), 1);

        cv::Mat line_kernel(3, 3, CV_8UC1, cv::Scalar(1));
        line_kernel.at<uint8_t>(1, 1) = 10;
        cv::Mat filtered;
        cv::filter2D(contour_img, filtered, CV_16SC1, line_kernel);

        // np.where
        std::vector<cv::Point> endpoints;
        for (size_t r = 0; r < filtered.rows; r++) {
            for (size_t c = 0; c < filtered.cols; c++) {
                if (filtered.at<int16_t>(r, c) == 11) {
                    endpoints.emplace_back(c, r);
                }
            }
        }

        const size_t half_size = reg_bounding_box_size_ / 2;
        const cv::Point half_size_point(half_size, half_size);

        for (const cv::Point& endpnt : endpoints) {
            cv::Point tl = endpnt - half_size_point;
            cv::Point br = endpnt + half_size_point;
            if (tl.x < 0)
                tl.x = 0;
            if (tl.y < 0)
                tl.y = 0;
            if (br.x >= contour_img.cols)
                br.x = contour_img.cols - 1;
            if (br.y >= contour_img.rows)
                br.y = contour_img.rows - 1;
            cv::rectangle(debug_img, tl, br, cv::Scalar(255, 0, 0), 1);

            cv::Mat endpnt_box = contour_img(cv::Rect(tl, br));
            std::vector<cv::Point> box_points;
            cv::findNonZero(endpnt_box, box_points);

            cv::Vec4f line;
            cv::fitLine(box_points, line, CV_DIST_L2, 0, 0.01, 0.01);
            double vx = line(0);
            double vy = line(1);
            double x = line(2) + tl.x;
            double y = line(3) + tl.y;

            ROS_INFO_STREAM("line " << x << " " << y << " " << vx << " " << vy);

            cv::Point2d new_endpnt =
                  intersect_point_line(endpnt, cv::Point2d(x, y), cv::Point2d(x + vx, y + vy), false);

            cv::Vec2d avg_pnt;
            for (const cv::Point& pnt : box_points) {
                avg_pnt(0) += pnt.x + tl.x;
                avg_pnt(1) += pnt.y + tl.y;
            }
            avg_pnt /= static_cast<double>(box_points.size());

            cv::Vec2d v(vx, vy);
            double projection = (cv::Vec2d(new_endpnt) - avg_pnt).dot(v);
            if (projection < 0) {
                vx *= -1;
                vy *= -1;
            }

            cv::circle(debug_img, new_endpnt, 2, cv::Scalar(0, 255, 0));
            cv::Point2d point;
            point.x = new_endpnt.x + extrapolate_distance_ * vx;
            point.y = new_endpnt.y + extrapolate_distance_ * vy;
            endpnt_line.emplace_back(new_endpnt, point, vx, vy);
        }
    }

    std::valarray<bool> have_connection(false, endpnt_line.size());
    for (size_t i = 0; i < endpnt_line.size(); i++) {
        const auto& curr_line_context = endpnt_line.at(i);
        for (size_t j = i + 1; j < endpnt_line.size(); j++) {
            const auto& comp_line_context = endpnt_line.at(j);
            if (IsConnectable(curr_line_context, comp_line_context)) {
                have_connection[i] = true;
                have_connection[j] = true;
                const auto& point_a = std::get<0>(curr_line_context);
                const auto& point_b = std::get<0>(comp_line_context);
                cv::line(debug_img, point_a, point_b, cv::Scalar(255, 0, 255));
            }
        }

        if (!have_connection[i]) {
            const auto& endpnt = std::get<0>(curr_line_context);
            const auto& extrapolated_pnt = std::get<1>(curr_line_context);
            cv::line(debug_img, endpnt, extrapolated_pnt, cv::Scalar(0, 0, 255));
        }
    }

    // cv::imshow("closing", display_img);
    cv::imshow("closing", debug_img);
    cv::waitKey(1);
}

}  // namespace rr