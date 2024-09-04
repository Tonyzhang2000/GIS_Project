#include "drawroad.h"

OSMway::OSMway() {
    
}

OSMway::~OSMway() {
    
}

void OSMway::OSMway_insert(int wayIdx) {
     OSMID OSMid = getWayByIndex(wayIdx)->id();
     std::string temp;
     for(unsigned tag_id = 0; tag_id < getTagCount(getWayByIndex(wayIdx)); ++tag_id) {
         auto temp1 = getTagPair(getWayByIndex(wayIdx), tag_id);
         if (temp1.first == "highway" && (temp1.second=="motorway" || temp1.second=="motorway_link" || temp1.second=="trunk" || temp1.second == "primary" || temp1.second == "secondary")){
            temp = temp1.second;
            break;
         }
     } 
     roads.insert(make_pair(OSMid,temp));
}
//
//bool OSMway::OSMway_in(std::string string1, std::string string2, OSMID x) {
//    std::vector<std::pair<std::string, std::string>> temp = roads[x];
//    for (int id = 0;id < temp.size(); ++id) {      
//        if((temp[id].first == string1) && (temp[id].second == string2)) return true;
//    }
//    return false;
//}