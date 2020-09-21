#ifndef MAP2VORONOI_H
#define MAP2VORONOI_H

#define JC_VORONOI_IMPLEMENTATION
#define JCV_REAL_TYPE double
#define JCV_ATAN2 atan2
#define JCV_FLT_MAX 1.7976931348623157E+308

#include "jc_voronoi_clip.h"
#include <chrono>
#include <limits>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <atomic>

#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

//TODO: Determine if paths are homotopically different
//Check if there are any obstacles between paths?
//Sum all cells of area between paths?

namespace voronoi_path
{
    /**
     * Type used to store coordinates of nodes. Coordinates are pixels in the map
     **/
    struct GraphNode
    {
        double x;
        double y;
        GraphNode() : x(0), y(0) {}
        GraphNode(double _x, double _y) : x(_x), y(_y) {}
        GraphNode(std::pair<double, double> in_pair) : x(in_pair.first), y(in_pair.second) {}

        GraphNode operator*(const double &mult) const
        {
            return GraphNode(x * mult, y * mult);
        }

        GraphNode operator+(const double &incr) const
        {
            return GraphNode(x + incr, y + incr);
        }

        GraphNode operator-(const double &incr) const
        {
            return GraphNode(x - incr, y - incr);
        }

        GraphNode &operator+=(const GraphNode &incr)
        {
            x = x + incr.x;
            y = y + incr.y;
            return *this;
        }

        GraphNode operator+(const GraphNode &incr) const
        {
            return GraphNode(x + incr.x, y + incr.y);
        }

        GraphNode operator-(const GraphNode &incr) const
        {
            return GraphNode(x - incr.x, y - incr.y);
        }

        int getSquareMagnitude()
        {
            return pow(x, 2) + pow(y, 2);
        }
    };

    /**
     * Same structure as ROS's nav_msgs::OccupancyGrid type
     * Redefined here to decouple from ROS
     **/
    struct Map
    {
        std::string frame_id;
        double resolution;
        int width;
        int height;
        struct origin
        {
            struct position
            {
                double x;
                double y;
                double z;
            };

            struct orientation
            {
                double x;
                double y;
                double z;
                double w;
            };
        };

        std::vector<int> data;

        Map() {}
        Map(std::vector<int> in_data, int _width, int _height, double _resolution, std::string _frame_id)
        {
            data.insert(data.begin(), in_data.begin(), in_data.end());
            width = _width;
            height = _height;
            resolution = _resolution;
            frame_id = _frame_id;
        }
    };

    /**
     * Structure definition for use during A* search algorithm only
     * Contains the information required for path searching
     **/
    struct NodeInfo
    {
        int prevNode;
        double cost_upto_here;
        double cost_to_goal;
        double total_cost;

        void updateCost()
        {
            total_cost = cost_upto_here + cost_to_goal;
        }
    };

    /**
     * Struct used during the generation of Bezier curves using the shortest path's nodes
     **/
    struct FValue
    {
        double val;
        int r1, c1, r2, c2;

        FValue()
        {
            val = std::numeric_limits<double>::infinity();
            r1 = -1;
            c1 = -1;
            r2 = -1;
            c2 = -1;
        }
    };

    class voronoi_path
    {
    public:
        voronoi_path();
        bool mapToGraph(const Map &map_);
        std::vector<std::vector<int>> getAdjList();
        void printEdges();
        std::vector<std::vector<GraphNode>> getPath(const GraphNode &start, const GraphNode &end, const int &num_paths);
        void setLocalVertices(const std::vector<GraphNode> &vertices);
        bool isUpdatingVoronoi();

        double hash_resolution = 0.1;
        int hash_length = 6;
        double line_check_resolution = 0.1;
        bool print_timings = false;
        int occupancy_threshold = 100;
        int collision_threshold = 85;
        int pixels_to_skip = 0;
        double waypoint_sep = 2; //pixels
        bool findObstacleCentroids();

    private:
        Map map;
        std::vector<jcv_edge> edge_vector;
        std::vector<std::vector<int>> adj_list;
        std::vector<GraphNode> node_inf;
        double open_list_time = 0, closed_list_time = 0;
        double copy_path_time = 0, find_path_time = 0;
        double edge_collision_time = 0;
        std::vector<GraphNode> local_vertices;
        std::vector<int> factorials;
        std::atomic<bool> updating_voronoi;
        std::atomic<bool> is_planning;
        int num_nodes = 0;
        int bezier_max_n = 10;
        double min_node_sep_sq = 0.5;
        double extra_point_distance = 0.5;

        std::vector<jcv_point> fillOccupancyVector(const int &start_index, const int &num_pixels);
        std::string hash(const double &x, const double &y);
        std::vector<double> dehash(const std::string &str);
        bool getNearestNode(const GraphNode &start, const GraphNode &end, int &start_node, int &end_node);
        bool kthShortestPaths(const int &start_node, const int &end_node, const std::vector<int> &shortestPath, std::vector<std::vector<int>> &all_paths, const int &num_paths);
        bool findShortestPath(const int &start_node, const int &end_node, std::vector<int> &path, double &cost);
        void removeObstacleVertices();
        void removeCollisionEdges();
        double vectorAngle(const double vec1[2], const double vec2[2]);
        bool edgeCollides(const GraphNode &start, const GraphNode &end);
        double manhattanDist(const GraphNode &a, const GraphNode &b);
        double euclideanDist(const GraphNode &a, const GraphNode &b);
        int getNumberOfNodes();
        int factorial(int n);
        int combination(int n, int r);
        std::vector<GraphNode> bezierSubsection(std::vector<GraphNode> &points);
    };

} // namespace voronoi_path

#endif