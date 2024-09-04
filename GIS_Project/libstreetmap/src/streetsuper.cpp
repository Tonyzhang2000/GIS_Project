#include "streetsuper.h"

streetsuper::streetsuper(){

}
streetsuper::~streetsuper(){

}
void streetsuper::ss_insert(StreetSegmentIdx x){
    StreetSegmentInfo ss_info = getStreetSegmentInfo(x);
    StreetIdx s_id = ss_info.streetID;
    street_super_int[s_id].push(ss_info.from);
    street_super_int[s_id].push(ss_info.to);
    street_super_ss[s_id].push_back(x);
}
std::vector<IntersectionIdx> streetsuper::get_int(StreetIdx x){
    std::vector<IntersectionIdx> ret;
    std::priority_queue<StreetSegmentIdx> pq = street_super_int[x];//tmp->second;
    IntersectionIdx check_duplex = -1;
    while(!pq.empty()){
        IntersectionIdx temp;
        temp = pq.top();
        if (temp == check_duplex){
            pq.pop();
            continue; 
        }
        check_duplex = temp;
        ret.push_back(temp);
        pq.pop();
    }
    return ret; 
}

std::vector<StreetSegmentIdx> streetsuper::get_ss(StreetIdx x){
    return street_super_ss[x];
}
