#include <ros/ros.h>
#include <nav_msgs/OccupancyGrid.h>
#include <map_msgs/OccupancyGridUpdate.h>
#include "voronoi_path.h"

#include <costmap_2d/costmap_2d_ros.h>
#include <costmap_2d/costmap_2d.h>
#include <nav_core/base_global_planner.h>
#include <geometry_msgs/PoseStamped.h>
#include <angles/angles.h>
#include <base_local_planner/world_model.h>
#include <base_local_planner/costmap_model.h>
#include <chrono>

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
        nav_msgs::OccupancyGrid local_costmap;
        nav_msgs::OccupancyGrid merged_costmap;
        voronoi_path::Map map;
        voronoi_path::voronoi_path voronoi_path;
        bool initialized_ = false;
        int num_paths = 2;
        bool costmap_received = false;

        //Params
        double update_voronoi_rate = 0.3;
        double update_costmap_rate = 0.3;
        bool print_timings = true;

        /**
         * Number of decimals to use for hashing, separate from hash_length. 10 means 1 decimal, 1 means 0 decimals.
         * ie total length of string post-hash for 1151.345 if hash_resolution = 10.0 and hash_length = 6 
         * is "0011513"
         **/
        double hash_resolution = 10.0;

        /**
         * Number of digits to allow before decimal point. Number of digits should be greater than map size in pixels
         **/
        int hash_length = 6;

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

        ros::NodeHandle nh;
        ros::Subscriber local_costmap_sub;
        ros::Subscriber global_costmap_sub;
        ros::Subscriber global_update_sub;
        ros::Publisher merged_costmap_pub;
        ros::Publisher global_path_pub;
        ros::Publisher alternate_path_pub;
        ros::WallTimer voronoi_update_timer;
        ros::WallTimer map_update_timer;

        ros::Publisher centroid_pub;

        void localCostmapCB(const nav_msgs::OccupancyGrid::ConstPtr &msg);
        void globalCostmapCB(const nav_msgs::OccupancyGrid::ConstPtr &msg);
        void globalCostmapUpdateCB(const map_msgs::OccupancyGridUpdate::ConstPtr &msg);
        void updateVoronoiCB(const ros::WallTimerEvent &e);
        void updateVoronoiMapCB(const ros::WallTimerEvent &e);
        void threadedMapCleanup();
    };
}; // namespace shared_voronoi_global_planner

#endif