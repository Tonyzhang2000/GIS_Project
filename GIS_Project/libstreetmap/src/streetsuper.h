#pragma once 

#include <unordered_map>
#include <queue>

#include "m1.h"
#include "m2.h"


class streetsuper{
    public:
        streetsuper();
        ~streetsuper();
        void ss_insert(StreetSegmentIdx x);
        std::vector<IntersectionIdx> get_int(StreetIdx x);
        std::vector<StreetSegmentIdx> get_ss(StreetIdx x);

    private:
        std::unordered_map<StreetIdx, std::priority_queue<IntersectionIdx> > street_super_int;
        std::unordered_map<StreetIdx, std::vector<StreetSegmentIdx> > street_super_ss;
};