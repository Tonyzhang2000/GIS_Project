/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "node.h"

RealOSMnode::RealOSMnode() {
    
}

RealOSMnode::~RealOSMnode() {
    
}

void RealOSMnode::RealOSMnode_insert(int nodeIdx) {
    OSMID key = getNodeByIndex(nodeIdx)->id();
    OSMnodes.insert(std::make_pair(key, nodeIdx));
}

int RealOSMnode::RealOSMnode_getvalue(OSMID key) {
    return OSMnodes[key];
}
       
LatLon RealOSMnode::RealOSMnode_getLatLon(OSMID key) {   
    return getNodeCoords(getNodeByIndex(OSMnodes[key]));
}
