#pragma once 

#include <unordered_map>
#include "m2.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

class OSMnode{
    public:
        OSMnode();
        ~OSMnode();
        void OSMnode_insert(int nodeIdx);// member function to inital thid datastructure. 
        // Link the OSMID with POIIdx, to enhance the search speed
        std::unordered_map <std::string , std::vector< std::pair<OSMID, POIIdx> > > nodess;
        
};
