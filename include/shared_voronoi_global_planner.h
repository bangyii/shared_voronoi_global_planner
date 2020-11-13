#include <ros/ros.h>
#include <nav_msgs/OccupancyGrid.h>
#include <map_msgs/OccupancyGridUpdate.h>
#include "voronoi_path.h"

#include <costmap_2d/costmap_2d_ros.h>
#include <costmap_2d/costmap_2d.h>
#include <nav_core/base_global_planner.h>
#include <geometry_msgs/PoseStamped.h>
#include <actionlib_msgs/GoalStatusArray.h>
#include <geometry_msgs/Twist.h>
#include <angles/angles.h>
#include <base_local_planner/world_model.h>
#include <base_local_planner/costmap_model.h>
#include <chrono>
#include <memory>
#include <mutex>

#ifndef SHARED_VORONOI_GLOBAL_PLANNER_H
#define SHARED_VORONOI_GLOBAL_PLANNER_H

namespace shared_voronoi_global_planner
{
    class SharedVoronoiGlobalPlanner : public nav_core::BaseGlobalPlanner
    {
    public:
        SharedVoronoiGlobalPlanner();
        SharedVoronoiGlobalPlanner(std::string name, costmap_2d::Costmap2DROS *costmap_ros);
        /** overridden classes from interface nav_core::BaseGlobalPlanner **/
        void initialize(std::string name, costmap_2d::Costmap2DROS *costmap_ros);
        bool makePlan(const geometry_msgs::PoseStamped &start,
                      const geometry_msgs::PoseStamped &goal,
                      std::vector<geometry_msgs::PoseStamped> &plan);

    private:
        /**
         * Internal copy of local costmap from ROS
         **/
        nav_msgs::OccupancyGrid local_costmap;

        /** 
         * Internal map which merges global and local costmap from ros
         **/
        voronoi_path::Map map;

        /**
         * Voronoi path object which is used for planning
         **/
        voronoi_path::voronoi_path voronoi_path;

        /**
         * Flag indicating whether the planner has been initialized
         **/ 
        bool initialized_ = false;

        /**
         * Param to indicate number of paths to find
         **/
        int num_paths = 2;

        /**
         * Rate at which to update the voronoi diagram, Hz
         **/
        double update_voronoi_rate = 0.3;

        /**
         * Flag indicating whether to print timings, used for debugging
         **/
        bool print_timings = true;

        /**
         * If there is a node within this threshold away from a node that only has 1 connection, they will both be connected
         **/
        int node_connection_threshold_pix = 1;

        /**
         * Pixel resolution to increment when checking if an edge collision occurs. Value of 0.1 means the edge will
         * be checked at every 0.1 pixel intervals
         **/
        double line_check_resolution = 0.1;

        /**
         * Threhsold before a pixel is considered occupied. If pixel value is < occupancy_threshold, it is considered free
         **/
        int occupancy_threshold = 100;

        /**
         * Threshold before a pixel is considered occupied during collision checking, this is same as occupancy_threshold but 
         * collision_threshold is used when checking if an edge collides with obstacles. Can be used in conjunction with
         * ROS's costmap inflation to prevent planner from planning in between narrow spaces
         **/
        int collision_threshold = 85;

        /**
         * Radius to search around robot location to try and find an empty cell to connect to start of previous path, meters
         **/
        double search_radius = 1.0;

        /**
         * Threshold used for trimming paths, should be smaller than collision_threshold to prevent robot from getting stuck
         **/
        int trimming_collision_threshold = 75;

        /**
         * Pixels to skip during the reading of map to generate voronoi graph. Increasing pixels to skip reduces computation time
         * of voronoi generation, but also reduces voronoi diagram density, likely causing path finding issues
         **/
        int pixels_to_skip = 0;

        /**
         * Downscale factor used for scaling map before finding contours. Smaller values increase speed (possibly marginal)
         * but may decrease the accuracy of the centroids found
         **/
        double open_cv_scale = 0.25;

        /**
         * Threshold to classify a homotopy class as same or different. Ideally, same homotopy classes should have identical 
         * compelx values, but since "double" representation is used, some difference might be present for same homotopy classes
         **/
        double h_class_threshold = 0.2;

        /**
         * Minimum separation between nodes. If nodes are less than this value (m^2) apart, they will be cleaned up
         **/
        double min_node_sep_sq = 1.0;

        /**
         * Distance (m) to put the extra point which is used to ensure continuity
         **/
        double extra_point_distance = 1.0;

        /**
         * Joystick maximum linear velocity, to normalize for joystick direction
         **/
        double joy_max_lin = 1.0;

        /**
         * Joystick maximum angular velocity, to normalize for joystick direction
         **/
        double joy_max_ang = 1.0;

        /**
         * Whether or not to trim beggining of paths generated
         **/
        bool trim_path_beginning = true;

        double forward_sim_time = 1.0;       //s
        double forward_sim_resolution = 0.1; //m
        double near_goal_threshold = 1.0;
        double selection_threshold = 1.2;
        bool add_local_costmap_corners = false;
        bool publish_all_path_markers = false;
        std::string joystick_topic = "/joy_vel";
        bool visualize_edges = false;
        bool subscribe_local_costmap = true;
        int preferred_path = 0;
        voronoi_path::GraphNode prev_goal;
        
        std::vector<std::pair<int, int>> map_pixels_backup;

        ros::NodeHandle nh;
        ros::Subscriber local_costmap_sub;
        ros::Subscriber global_costmap_sub;
        ros::Subscriber global_update_sub;
        ros::Subscriber user_vel_sub;
        ros::Subscriber move_base_stat_sub;

        ros::Publisher global_path_pub;
        ros::Publisher all_paths_pub;
        ros::Publisher user_direction_pub;
        ros::Publisher edges_viz_pub;

        ros::WallTimer voronoi_update_timer;
        ros::WallTimer map_update_timer;
        geometry_msgs::Twist cmd_vel;

        ros::Publisher centroid_pub;

        void localCostmapCB(const nav_msgs::OccupancyGrid::ConstPtr &msg);
        void moveBaseStatusCB(const actionlib_msgs::GoalStatusArray::ConstPtr &msg);
        void globalCostmapCB(const nav_msgs::OccupancyGrid::ConstPtr &msg);
        void globalCostmapUpdateCB(const map_msgs::OccupancyGridUpdate::ConstPtr &msg);
        void cmdVelCB(const geometry_msgs::Twist::ConstPtr &msg);

        /**
         * Callback for timer event to periodically update the voronoi diagram, if update_voronoi_rate is > 0
         **/
        void updateVoronoiCB(const ros::WallTimerEvent &e);

        /**
         * Get the closes matching path to the users' current joystick direction
         * @param curr_pose the current pose of the robot
         * @param plans_ all the plans to try to match the user's direction to
         * @return integer index of the path that is the closes match in plans_ vector
         **/
        int getMatchedPath(const geometry_msgs::PoseStamped &curr_pose, const std::vector<std::vector<geometry_msgs::PoseStamped>> &plans_);

        /**
         * Returns the minimum angle between two vectors
         * @param vec1 an array of 2 doubles, x and y, of the first vector
         * @param vec2 an array of 2 doubles, x and y, of the second vector
         * @return minimum angle between the 2 vectors in rads
         **/
        double vectorAngle(const double vec1[2], const double vec2[2]);
    };
}; // namespace shared_voronoi_global_planner

#endif