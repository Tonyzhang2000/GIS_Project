#include "nodes.h"

OSMnode::OSMnode() {
    int numOPOI = getNumPointsOfInterest();
    for (int POIid = 0 ; POIid < numOPOI ; POIid+=5){
        nodess[getPOIType(POIid)].push_back(std::make_pair(getPOIOSMNodeID(POIid),POIid));
    }
}

OSMnode::~OSMnode() {
    
}
