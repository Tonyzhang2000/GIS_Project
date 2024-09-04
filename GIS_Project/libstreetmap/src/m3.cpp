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

#include "StreetsDatabaseAPI.h"
#include "m3.h"
#include "global.h"

#include <vector>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <thread>




std::unordered_map<IntersectionIdx, double> cost;
typedef struct info {
    bool inopenset;
    bool incloseset;
    double cost;
    IntersectionIdx came_from;
    StreetSegmentIdx came_via;
} Info;


// g(n) = 
/*
    g[N]={+inf,}
    g[start] = 0
    on visit node n 
        g[n] = min (g[prev] + path time(path), g[n] )
        
*/
/* path_time(path){
    return changestreet?length/speed + turn_penalty:length/speed;
}
dx = x[n] - destx;
dy = y[n] - desty;
h[n] = sqrt(dx*dx + dy*dy)/speed 

f(n) = h[n] + g[n]
*/
// Returns the time required to travel along the path specified, in seconds.
// The path is given as a vector of street segment ids, and this function can
// assume the vector either forms a legal path or has size == 0.  The travel
// time is the sum of the length/speed-limit of each street segment, plus the
// given turn_penalty (in seconds) per turn implied by the path.  If there is
// no turn, then there is no penalty. Note that whenever the street id changes
// (e.g. going from Bloor Street West to Bloor Street East) we have a turn.
double computePathTravelTime(const std::vector<StreetSegmentIdx>& path, 
                                const double turn_penalty){
        if (path.size() == 0 ) return 0.0;
        double ret = findStreetSegmentTravelTime(path[0]);
        for (int i = 1 ;i < path.size() ; i++){
            ret += findStreetSegmentTravelTime(path[i]);
            ret += getStreetName(getStreetSegmentInfo(path[i-1]).streetID) != getStreetName(getStreetSegmentInfo(path[i]).streetID)? turn_penalty : 0;
        }
        return ret; 
}


// Returns a path (route) between the start intersection and the destination
// intersection, if one exists. This routine should return the shortest path
// between the given intersections, where the time penalty to turn right or
// left is given by turn_penalty (in seconds).  If no path exists, this routine
// returns an empty (size == 0) vector.  If more than one path exists, the path
// with the shortest travel time is returned. The path is returned as a vector
// of street segment ids; traversing these street segments, in the returned
// order, would take one from the start to the destination intersection.
double h(IntersectionIdx adjint ,IntersectionIdx dest); 
double h(IntersectionIdx adjint ,IntersectionIdx dest){
    auto adjintloc = getIntersectionPosition(adjint);
    auto destloc = getIntersectionPosition(dest);
    return findDistanceBetweenTwoPoints(std::make_pair(adjintloc, destloc))/max_spd;
}

std::vector<StreetSegmentIdx> findPathBetweenIntersections(
                  const IntersectionIdx intersect_id_start, 
                  const IntersectionIdx intersect_id_destination,
                  const double turn_penalty){ 

    std::vector<StreetSegmentIdx> ret_path;
    std::unordered_map<IntersectionIdx,Info > inter_info; 
    std::priority_queue<std::pair<double, IntersectionIdx>,std::vector<std::pair<double, IntersectionIdx>>, std::greater<std::pair<double, IntersectionIdx>> >  openset;
    openset.push(std::make_pair(0,intersect_id_start));
    Info start_node ;
    start_node.inopenset = true;
    start_node.cost = 0;
    inter_info[intersect_id_start] = start_node ; 
    if (intersect_id_start == intersect_id_destination){
        return ret_path ; 
    }
    while (!openset.empty()){
        auto curr(openset.top());
        openset.pop();
        auto curidx = curr.second; 
        Info curinfo = inter_info[curidx];
        if (curidx == intersect_id_destination){
            break ;
        }else {
            curinfo.inopenset = false;
            auto adjedge = findStreetSegmentsOfIntersection(curidx);
            for (auto &anedge : adjedge){
                
                auto edgeinfo = getStreetSegmentInfo(anedge);
                if (edgeinfo.to == curidx && edgeinfo.oneWay){
                    continue;
                }
                if (edgeinfo.from == edgeinfo.to || inter_info[edgeinfo.to].incloseset || inter_info[edgeinfo.from].incloseset ){
                    continue;
                }
                auto topt = (edgeinfo.to == curidx) ? edgeinfo.from : edgeinfo.to;
                bool penalty = (getStreetSegmentInfo(anedge).streetID != getStreetSegmentInfo(curinfo.came_via).streetID);
                double cost_update = curinfo.cost + findStreetSegmentTravelTime(anedge) + (penalty?turn_penalty:0);
                Info toptinfo = inter_info[topt];
                auto testforexistinopenset = toptinfo.inopenset;
                if (!testforexistinopenset){
                    toptinfo.cost = cost_update;
                    double priority = toptinfo.cost + h(topt,intersect_id_destination); 
                    openset.push(std::make_pair(priority,topt));
                    toptinfo.inopenset = true;
                    toptinfo.came_from = curidx;
                    toptinfo.came_via = anedge;
                    inter_info[topt] = toptinfo;
                }else{
                    if (toptinfo.cost > cost_update){
                        toptinfo.cost = cost_update;
                        double priority = toptinfo.cost + h(topt,intersect_id_destination); 
                        openset.push(std::make_pair(priority,topt));
                        toptinfo.inopenset = true;
                        toptinfo.came_from = curidx;
                        toptinfo.came_via = anedge;
                        inter_info[topt] = toptinfo;
                    }
                }
                
            }
            curinfo.incloseset = true;
            inter_info[curidx] = curinfo;
            
        }
        

    }
    
    if (!inter_info[intersect_id_destination].inopenset){
        return ret_path;
    }
    auto last = intersect_id_destination;
    while(last != intersect_id_start){
        ret_path.push_back(inter_info[last].came_via);
        last = inter_info[last].came_from;
    }
    std::reverse(ret_path.begin(), ret_path.end());
    return ret_path;
    
}
