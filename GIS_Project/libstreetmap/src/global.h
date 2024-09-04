/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   global.h
 * Author: zhan7388, dingguoz, hankemen
 *
 * Created on March 7, 2021, 4:39 AM
 */

#ifndef GLOBAL_H
#define GLOBAL_H
#include "streetsuper.h"
#include "my_m2.h"
#include <unordered_map>
#include "drawroad.h"

extern streetsuper* Hashtable_for_s_id;
extern OSMway* Hashtable_for_roads;
extern std::string inputSt1;
extern std::string inputSt2;
extern StreetIdx sel1, sel2; 
extern ezgl::application* applic; 
extern std::vector<struct intersection_data> intersections;
extern std::unordered_map<int, bool> poi_light;
extern std::unordered_map<std::string, bool> POIstatus;
extern std::string current_city;
extern bool night_bool;
extern bool subway_bool;
extern bool POImode; 
extern bool Find_path_mode;
extern bool Intersection_mode;
extern std::vector<std::vector<std::pair<std::string, LatLon>>> subway_stations;
extern IntersectionIdx start;
extern IntersectionIdx end;

extern double max_spd;
#endif /* GLOBAL_H */

