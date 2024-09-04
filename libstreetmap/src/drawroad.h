#pragma once 

#include <unordered_map>
#include "m2.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

class OSMway{
    public:
        OSMway();
        ~OSMway();
        void OSMway_insert(int wayIdx);  //Mutator
        //Link OSMID with street type 
        std::unordered_map <OSMID, std::string > roads;
        
};