#pragma once
#include "m1.h"


class adjlink{
    public:
        adjlink(int n_nodes, int n_edges); // Constructor 
        ~adjlink(); // Destructor
        void adj_insert(IntersectionIdx from, IntersectionIdx to , StreetSegmentIdx edge); // Insert an edge to this adjlink
        std::vector<StreetSegmentIdx> find_edge_of_a_node(IntersectionIdx x); // Return a vector includes ss_id connected to an intersection
        std::vector<IntersectionIdx>  find_node_of_a_node(IntersectionIdx x); // Return a vector includes intersection_id connected to an intersection
        int get_number_of_nodes();
    private:
        int N;  // Number of nodes
        int E;  //Number of edges 
        std::vector <std::vector< std::pair<IntersectionIdx,StreetSegmentIdx> > > adj_link; // Data structure 
};


