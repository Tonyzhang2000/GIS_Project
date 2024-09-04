/* 
 * Copyright 2021 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <iostream>
#include <cmath>
#include <iomanip> 
#include <algorithm>
#include <limits>
#include <cstddef>

#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "adjlink.h"
#include "streetsuper.h"
#include "trie_tree.h"
#include "drawroad.h"
#include "global.h"
#include "node.h"






// loadMap will be called with the name of the file that stores the "layer-2"
// map data accessed through StreetsDatabaseAPI: the street and intersection 
// data that is higher-level than the raw OSM data). 
// This file name will always end in ".streets.bin" and you 
// can call loadStreetsDatabaseBIN with this filename to initialize the
// layer 2 (StreetsDatabase) API.
// If you need data from the lower level, layer 1, API that provides raw OSM
// data (nodes, ways, etc.) you will also need to initialize the layer 1 
// OSMDatabaseAPI by calling loadOSMDatabaseBIN. That function needs the 
// name of the ".osm.bin" file that matches your map -- just change 
// ".streets" to ".osm" in the map_streets_database_filename to get the proper
// name.

//global variable

//store all street-segment indexes of a list of intersectionss
std::vector<std::vector<StreetSegmentIdx>> intersection_street_segments; 

//store all street-segment names of a list of intersectionss
std::vector<std::vector<std::string>> intersection_street_segments_name; 

//store the adjacent intersection indexes of a list of intersectionss
std::vector<std::vector<IntersectionIdx>> intersection_adjacents;

//pointer to a double vector of street segment travel time for caching
std::vector<double> * ss_time_cache;

//pointer to a adjlink object storing the graph
adjlink* Graph;

//pointer to a hash table for street id & corresponding street segment id
streetsuper* Hashtable_for_s_id;

//vector to pre-calculate and store all the intersectionss of a street
std::vector<std::vector<IntersectionIdx>> streetIntercCache; 

//Modified Trie Tree to store the street names
TrieNode* tt_root;

//Caching OSMway information
OSMway* Hashtable_for_roads;

//Caching subway information
std::vector<const OSMRelation*> subways;

//Caching subway station information
std::vector<std::vector<OSMID>> subway_nodes;

//Caching subway station information
std::vector<std::vector<std::pair<std::string, LatLon>>> subway_stations;

//Storing the OSM nodes for searching subway station.
RealOSMnode* Hashtable_for_nodes;

double max_spd; 

bool loadMap(std::string map_streets_database_filename) {
    //Indicates whether the map has loaded successfully
    max_spd = 0 ; 
    bool load_successful = loadStreetsDatabaseBIN(map_streets_database_filename); 
    int position1  = map_streets_database_filename.find_last_of(".");
    int position2 = map_streets_database_filename.substr(0,position1).find_last_of(".");
    std::string OSM_string;
    OSM_string = map_streets_database_filename.substr(0,position2);
    OSM_string += ".osm.";
    OSM_string += map_streets_database_filename.substr(position1 + 1 );
    std::cout << OSM_string << std::endl;

    loadOSMDatabaseBIN(OSM_string); 

    if (load_successful){
        std::cout << "loadMap: " << map_streets_database_filename << std::endl;
        std::cout << "[DEBUG]POI numbers: " << getNumPointsOfInterest() << std::endl;
        //set the size of the global variables
        intersection_street_segments.resize(getNumIntersections());
        intersection_street_segments_name.resize(getNumIntersections());
        intersection_adjacents.resize(getNumIntersections());

        //initialize the data structures 
        Graph = new adjlink(getNumIntersections(),getNumStreetSegments());
        Hashtable_for_s_id = new streetsuper;
        tt_root = new TrieNode(); //Trie_tree for store street names
        ss_time_cache = new std::vector<double>(getNumStreetSegments(),-1);
        Hashtable_for_roads = new OSMway;
        Hashtable_for_nodes = new RealOSMnode;

        
        for(int nodeidx = 0;nodeidx < getNumberOfNodes();++nodeidx) {
            Hashtable_for_nodes->RealOSMnode_insert(nodeidx);
        }
        
        for(int id = 0;id < getNumberOfWays(); ++id) {
            Hashtable_for_roads->OSMway_insert(id);

        }
        
        for(int id = 0;id < getNumberOfRelations(); ++id) {
            const OSMRelation * cur = getRelationByIndex(id);
            for(int tagId = 0; tagId < getTagCount(cur); ++tagId) {
            std::pair<std::string, std::string> tag = getTagPair(cur, tagId);
                if(tag.first == "route" && tag.second == "subway") {
                subways.push_back(cur);
                break;
                }
            }
        }
        
        for(int id = 0;id < subways.size();++id) {    //each line 
            std::vector<std::pair<std::string, LatLon>> temp;
            std::vector<TypedOSMID> route_members = getRelationMembers(subways[id]);
            for(int memberid = 0; memberid < route_members.size(); ++memberid) {  //all the points 
                if(route_members[memberid].type() == TypedOSMID::Node) {
                    int nodeidx = Hashtable_for_nodes->RealOSMnode_getvalue(route_members[memberid]);
                    const OSMNode *curr = getNodeByIndex(nodeidx);
                    for(int nodeid = 0;nodeid < getTagCount(curr);++nodeid) {
                        if(getTagPair(curr, nodeid).first == "name") {
                            temp.push_back(std::make_pair(getTagPair(curr, nodeid).second, Hashtable_for_nodes->RealOSMnode_getLatLon(route_members[memberid])));
                        }
                    } 
                } 
            }
            subway_stations.push_back(temp);
            //subway_lines.push_back(std::make_pair(color, subway_lines_latlon)); 
        }
        
        for (StreetSegmentIdx ss_id = 0 ; ss_id < getNumStreetSegments() ; ss_id ++){
            StreetSegmentInfo ss_info = getStreetSegmentInfo(ss_id);
            max_spd = max_spd>ss_info.speedLimit? max_spd : ss_info.speedLimit;
            //add edges to the undirected graph
            Graph->adj_insert(ss_info.from, ss_info.to, ss_id);
            Graph->adj_insert(ss_info.to, ss_info.from, ss_id);

            //add street segments to the corresponding street index
            Hashtable_for_s_id->ss_insert(ss_id);
        }

        for (StreetSegmentIdx ss_id = 0 ; ss_id < getNumStreetSegments() ; ss_id ++){

            //pre-calculate all the intersectionss of a street
            streetIntercCache.push_back(Hashtable_for_s_id->get_int(ss_id));
        }

        for (StreetIdx s_id = 0 ; s_id < getNumStreets() ; s_id ++){
            //store the street name to the trie tree
            tt_root->tt_insert(getStreetName(s_id),s_id,true); 
        }

        //pre-calculate the used variables for following functions
        for (IntersectionIdx intersection_id = 0 ; intersection_id <getNumIntersections() ; intersection_id++ ){
            std::vector<StreetSegmentIdx> temp = Graph->find_edge_of_a_node(intersection_id);
            std::vector<std::string> names;

            for (std::vector<StreetSegmentIdx>::iterator it = temp.begin(); it != temp.end() ; it++ ){
                names.push_back(getStreetName(getStreetSegmentInfo(*it).streetID));
            }

            intersection_street_segments_name[intersection_id]=names;
            intersection_street_segments[intersection_id]=Graph->find_edge_of_a_node(intersection_id);
            intersection_adjacents[intersection_id]=Graph->find_node_of_a_node(intersection_id);
        }
    }
    //initialize POI status as false (not print)
    POIstatus["School"] = false;
    POIstatus["Cup"] = false;
    POIstatus["Restaurant"] = false;
    POIstatus["Bank"] = false;
    POIstatus["Hospital"] = false;
    POIstatus["Bar"] = false;
    POIstatus["Movie"] = false;
    
    return load_successful;
}

void closeMap() {
    //Clean-up your map related data structures here

    for (int i = 0 ; i < getNumPointsOfInterest() ; i+=5 ){
        poi_light[i] = false;
    }
    //reset the precalculated vectors for later use
    for(int i = 0 ; i < intersection_street_segments.size() ; i++ ){
        intersection_street_segments[i].clear();
        intersection_street_segments_name[i].clear();
        intersection_adjacents[i].clear();
    }
    
    for(int i = 0; i < subway_stations.size(); i++) {
        subway_stations[i].clear();
    }
    
    /*for(int i = 0; i < subway_lines.size(); i++) {
        ((subway_lines[i]).second).clear();
    }*/
    
    intersection_street_segments.clear();
    intersection_street_segments_name.clear();
    intersection_adjacents.clear();
    subway_stations.clear();
    subways.clear();
    poi_light.clear();
    intersections.clear();
    //subway_lines.clear();
    
   
    
    //free the memories to avoid leak
    delete Graph;
    delete Hashtable_for_s_id;
    delete tt_root;
    delete ss_time_cache;
    delete Hashtable_for_roads;
    delete Hashtable_for_nodes;
    //delete Hashtable_for_ways;
    closeStreetDatabase();
    closeOSMDatabase(); 
    return; 
}

// Returns the distance between two (lattitude,longitude) coordinates in meters
// Speed Requirement --> moderate

double findDistanceBetweenTwoPoints(std::pair<LatLon, LatLon> points){
    //convert the lat & lon value to radian
    double Lat1 = points.first.latitude()*kDegreeToRadian;
    double Lat2 = points.second.latitude()*kDegreeToRadian;
    double Lon1 = points.first.longitude()*kDegreeToRadian;
    double Lon2 = points.second.longitude()*kDegreeToRadian;
    
    double Lat_avg = (Lat1 + Lat2)/2;
    double x1,y1,x2,y2;
    
    x1 = Lon1 * cos(Lat_avg);
    x2 = Lon2 * cos(Lat_avg);
    y1 = Lat1;
    y2 = Lat2;
    return kEarthRadiusInMeters*sqrt((y2-y1)*(y2-y1)+(x2-x1)*(x2-x1));

}


// Returns the length of the given street segment in meters
// Speed Requirement --> moderate

double findStreetSegmentLength(StreetSegmentIdx street_segment_id){
    double distance = 0;
    StreetSegmentInfo street = getStreetSegmentInfo(street_segment_id);

    LatLon _start = getIntersectionPosition(street.from);
    LatLon _end = getIntersectionPosition(street.to);

    if (street.numCurvePoints == 0){
        //No curve points -- just calculate from start to end
        distance = findDistanceBetweenTwoPoints(std::make_pair(_start,_end));
        
    } else{
        
        //if Curve points -- calculate by single segments
        LatLon curvePt1, curvePt2;
        curvePt1 = _start;
        std::pair<LatLon, LatLon> curve_seg;
        for (int i = 0; i< street.numCurvePoints;i++) {
            curvePt2 =  getStreetSegmentCurvePoint(street_segment_id, i);
            distance += findDistanceBetweenTwoPoints(std::make_pair(curvePt1,curvePt2));
            curvePt1 = curvePt2;
        }
        
        distance += findDistanceBetweenTwoPoints(std::make_pair(curvePt2, _end));
    } 
    
    return distance;
}



// Returns the travel time to drive from one end of a street segment in 
// to the other, in seconds, when driving at the speed limit
// Note: (time = distance/speed_limit)
// Speed Requirement --> high 

double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id){
    double ret ; 
    
    //check if the travel time has been calculated
    //if so, return the pre-calculated value stored in global
    if((*ss_time_cache)[street_segment_id] != -1)
        ret = (*ss_time_cache)[street_segment_id];
    else {
        
        double distance = findStreetSegmentLength(street_segment_id);
        double speed = getStreetSegmentInfo(street_segment_id).speedLimit;
        ret =  distance/speed;
        (*ss_time_cache)[street_segment_id] = ret; 
    }
    return ret;
}


// Returns the nearest intersection to the given position
// Speed Requirement --> none

IntersectionIdx findClosestIntersection(LatLon my_position){
    int closest_Intersection_id = -1;
    //set a large number for initial value to be replaced
    double closest_Distance = std::numeric_limits<double>::max();
    
    //traverse every intersection and calculate the distance to decide the closest one
    for(int intersection = 0; intersection < getNumIntersections();++intersection) {

        LatLon intersection_position = getIntersectionPosition(intersection);
        std::pair<LatLon, LatLon> points(my_position, intersection_position);
        
        if(findDistanceBetweenTwoPoints(points) < closest_Distance) {
            closest_Intersection_id = intersection;
            closest_Distance = findDistanceBetweenTwoPoints(points);
        }
    }

    return closest_Intersection_id;

}


// Returns the street segments that connect to the given intersection 
// Speed Requirement --> high

std::vector<StreetSegmentIdx> findStreetSegmentsOfIntersection(IntersectionIdx intersection_id){

    return intersection_street_segments[intersection_id];

}


// Returns the street names at the given intersection (includes duplicate 
// street names in the returned vector)
// Speed Requirement --> high 

std::vector<std::string> findStreetNamesOfIntersection(IntersectionIdx intersection_id){

    return intersection_street_segments_name[intersection_id];

}


// Returns all intersectionss reachable by traveling down one street segment 
// from the given intersection (hint: you can't travel the wrong way on a 
// 1-way street)
// the returned vector should NOT contain duplicate intersectionss
// Speed Requirement --> high 

std::vector<IntersectionIdx> findAdjacentIntersections(IntersectionIdx intersection_id){
      
    return intersection_adjacents[intersection_id];

}


// Returns all intersectionss along the a given street
// Speed Requirement --> high

std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id){

    return Hashtable_for_s_id->get_int(street_id);

}


// Return all intersection ids at which the two given streets intersect
// This function will typically return one intersection id for streets
// that intersect and a length 0 vector for streets that do not. For unusual 
// curved streets it is possible to have more than one intersection at which 
// two streets cross.
// Speed Requirement --> high

std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(std::pair<StreetIdx, StreetIdx> street_ids){

    std::vector<IntersectionIdx> street1 = streetIntercCache[street_ids.first];
    std::vector<IntersectionIdx> street2 = streetIntercCache[street_ids.second];
    
    std::vector<IntersectionIdx>::iterator poiFir = street1.begin();
    std::vector<IntersectionIdx>::iterator poiSec = street2.begin();
    std::vector<IntersectionIdx> intersectionss;
    
    //find the common street segment index values for two streets
    while(poiFir != street1.end() && poiSec != street2.end()) {
        if(*poiFir > *poiSec) {
            poiFir++;
        } else if(*poiFir == *poiSec) {
            intersectionss.push_back(*poiFir);
            poiFir++;
            poiSec++;
        } else if(*poiFir < *poiSec) {
            poiSec++;
        }
    }
    return intersectionss;
}


// Returns all street ids corresponding to street names that start with the 
// given prefix 
// The function should be case-insensitive to the street prefix. 
// The function should ignore spaces.
//  For example, both "bloor " and "BloOrst" are prefixes to 
// "Bloor Street East".
// If no street names match the given prefix, this routine returns an empty 
// (length 0) vector.
// You can choose what to return if the street prefix passed in is an empty 
// (length 0) string, but your program must not crash if street_prefix is a 
// length 0 string.
// Speed Requirement --> high 

std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix){
    //trie tree for street names
    auto ret =  tt_root->tt_search(street_prefix,true);
    return ret;

}


// Returns the length of a given street in meters
// Speed Requirement --> high 

double findStreetLength(StreetIdx street_id){
    double ret = 0 ; 
    auto ss_s = Hashtable_for_s_id->get_ss(street_id);
    for (auto ss_si : ss_s){
        ret += findStreetSegmentLength(ss_si);
    }
    return ret;

}


// Return the smallest rectangle that contains all the intersectionss and 
// curve points of the given street (i.e. the min,max lattitude 
// and longitude bounds that can just contain all points of the street).
// Speed Requirement --> none 

LatLonBounds findStreetBoundingBox(StreetIdx street_id) {

    //initalization of the position value and required vector
    std::vector<IntersectionIdx> intersectionss = findIntersectionsOfStreet(street_id);
    double minlat = getIntersectionPosition(intersectionss[0]).latitude();
    double minlon = getIntersectionPosition(intersectionss[0]).longitude();
    double maxlat = getIntersectionPosition(intersectionss[0]).latitude();
    double maxlon = getIntersectionPosition(intersectionss[0]).longitude();

    //traverse all intersectionss and pick their max/min position value
    for(int i = 1;i < intersectionss.size();i++) {
        LatLon pos = getIntersectionPosition(intersectionss[i]);
        if(pos.latitude() < minlat) minlat = pos.latitude();
        if(pos.latitude() > maxlat) maxlat = pos.latitude();
        if(pos.longitude() < minlon) minlon = pos.longitude();
        if(pos.longitude() > maxlon) maxlon = pos.longitude();
    }

    //traverse all curve points and pick their max/min position value
    for(int i = 0;i < Hashtable_for_s_id->get_ss(street_id).size();i++) {
        int k = Hashtable_for_s_id->get_ss(street_id)[i];
        for(int j = 0;j < getStreetSegmentInfo(k).numCurvePoints;j++) {
            LatLon pos = getStreetSegmentCurvePoint(k, j);
            if(pos.latitude() < minlat) minlat = pos.latitude();
            if(pos.latitude() > maxlat) maxlat = pos.latitude();
            if(pos.longitude() < minlon) minlon = pos.longitude();
            if(pos.longitude() > maxlon) maxlon = pos.longitude();
        }
    }
    LatLonBounds box;
    box.max = LatLon(maxlat, maxlon);
    box.min = LatLon(minlat, minlon);
    return box;
}


// Returns the nearest point of interest of the given name to the given position
// Speed Requirement --> none 

POIIdx findClosestPOI(LatLon my_position, std::string POIname){

    //set a large number for replace
    double distance = std::numeric_limits<double>::max();
    POIname == "";  //meaningless code to avoid warning. 
    //initialize by -1 for error checking
    int poiIdx = -1;
    for(int i = 0;i <  getNumPointsOfInterest();i+=5) {

        //only calculate for the required POIname

        if(findDistanceBetweenTwoPoints(std::make_pair(my_position, getPOIPosition(i))) < distance) {
            poiIdx = i;
            distance = findDistanceBetweenTwoPoints(std::make_pair(my_position, getPOIPosition(i)));
        }
    }

    return poiIdx;

}


// Returns the area of the given closed feature in square meters
// Assume a non self-intersecting polygon (i.e. no holes)
// Return 0 if this feature is not a closed polygon.
// Speed Requirement --> moderate

double findFeatureArea(FeatureIdx feature_id){
    int numFeature = getNumFeaturePoints(feature_id);
    double area = 0;
    
    //only calculate closed feature area (area = 0 as initialization for open polygon)
    if(getFeaturePoint(feature_id, 0) == getFeaturePoint(feature_id, numFeature-1)) {
        double minLon = getFeaturePoint(feature_id, 0).longitude();
        
        for(int i = 1;i < numFeature; i++) {
            if(getFeaturePoint(feature_id, i).longitude() < minLon) 
                minLon = getFeaturePoint(feature_id, i).longitude();
        }

        //do calculation
        for(int i = 0;i < numFeature-1; i++) {
            LatLon helper1 = LatLon(getFeaturePoint(feature_id, i).latitude(), minLon);
            LatLon helper2 = LatLon(getFeaturePoint(feature_id, i+1).latitude(), minLon);
            double height = findDistanceBetweenTwoPoints(std::make_pair(helper1, helper2));
            double edge1 = findDistanceBetweenTwoPoints(std::make_pair(helper1, getFeaturePoint(feature_id, i)));
            double edge2 = findDistanceBetweenTwoPoints(std::make_pair(helper2, getFeaturePoint(feature_id, i+1)));
            
            if(getFeaturePoint(feature_id, i).latitude() < getFeaturePoint(feature_id, i+1).latitude())
                area = area + 0.5 * height * (edge1 + edge2);
            else area = area - 0.5 * height * (edge1 + edge2);
        }
    }
    return fabs(area);
    
}



