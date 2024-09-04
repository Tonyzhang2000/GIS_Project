#include "m1.h"
#include "adjlink.h"

adjlink::adjlink(int n_nodes, int n_edges){ // Constructor 
    this->N = n_nodes;
    this->E = n_edges;
    this->adj_link.resize(N);
}
adjlink::~adjlink(){ // Destructor


}
void adjlink::adj_insert(IntersectionIdx from, IntersectionIdx to , StreetSegmentIdx edge){ // Insert an edge to this adjlink
    adj_link[from].push_back(std::make_pair(to,edge));

}


std::vector<StreetSegmentIdx> adjlink::find_edge_of_a_node(IntersectionIdx x){ // Return a vector includes ss_id connected to an intersection
    std::vector<StreetSegmentIdx> ret;
    ret.clear();
    std::vector<std::pair<IntersectionIdx,StreetSegmentIdx> >::iterator it;
    for(it = adj_link[x].begin() ; it != adj_link[x].end()  ; ++it  ){
        ret.push_back(it->second);
    }
    return ret; 
}
std::vector<IntersectionIdx>  adjlink::find_node_of_a_node(IntersectionIdx x){ // Return a vector includes intersection_id connected to an intersection
    std::vector<IntersectionIdx> ret;
    ret.clear();
    std::vector<std::pair<IntersectionIdx,StreetSegmentIdx> >::iterator it;
    for(it = adj_link[x].begin() ; it != adj_link[x].end()  ; ++it  ){
            bool repeated = false;
	    for(std::vector<StreetSegmentIdx>::iterator itt = ret.begin(); itt != ret.end() ; itt ++)
		    if (it->first == *itt ){
			repeated = true;
			break;
		    } 
	    if (!repeated){
		    StreetSegmentInfo ss_info = getStreetSegmentInfo(it->second);
	    	    if (ss_info.from != x && ss_info.oneWay)
			    continue;
		    ret.push_back(it->first);
	    }
    }
    return ret; 
}

int adjlink::get_number_of_nodes(){
    return this->N;
}
