/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   node.h
 * Author: zhan7388
 *
 * Created on March 11, 2021, 12:33 AM
 */

#pragma once 

#include <unordered_map>
#include "m2.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

class RealOSMnode{
    public:
        RealOSMnode();
        ~RealOSMnode();
        void RealOSMnode_insert(int nodeIdx);  // Mutator
        int RealOSMnode_getvalue(OSMID key);   //Accessor
        LatLon RealOSMnode_getLatLon(OSMID key); //Accessor
    private:
        //connect OSMID(64-bit number) with the NODE index
        std::unordered_map <OSMID, int> OSMnodes;      
};

