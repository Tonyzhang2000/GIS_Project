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
 
#include "m4.h"
#include "m3.h"
#include "m1.h"
#include "m2.h"

#include <cstring>
#include <chrono>
#include <cstdlib>
#include <omp.h>
#include <unordered_map>
#include <queue>
#include <cmath> 
#include <ctime> 

#define MAXPOINTS 700
#define TEND 1e-4 // The end temperature of the SA algo 
#define A     0.9999
#define ACCEPT_BAD 1


//struct defines 
typedef struct info {
    bool inopenset;
    bool incloseset;
    double cost;
    IntersectionIdx came_from;
    StreetSegmentIdx came_via;
} Info;

 
typedef struct pdpoint{
    int index;
    int package; 
    IntersectionIdx pos; 
    bool is_pickup; 
    bool is_visited; 
} PDpoint; 

//function defines
void findPaths(std::vector<pdpoint>, int, double);
int get_nearest_legal_move(int, const std::vector<DeliveryInf>&, int*);
int get_nearest_depot(const std::vector<int>&, int, double);
int get_nearest_depot_time(int, double);
int get_time(std::vector<pdpoint>, std::vector<int>, double);
std::vector<pdpoint> generate_new_path(std::vector<pdpoint>, int, const std::vector<int>&, float);


//variables defines
using namespace std; 
int edges[MAXPOINTS+1][MAXPOINTS+1];
std::vector<PDpoint> nodess; 
int deliv_num; 
bool not_possiable; 


//Use single-start, multiple-dest to generate the adj matrix , slightly changed from M3
void findPaths(const std::vector<PDpoint>nodes, const int source, const double turn_penalty){ 
    std::unordered_map<IntersectionIdx, bool> in_nodes; 
    auto intersect_id_start = nodes[source].pos; 
    std::vector<StreetSegmentIdx> ret_path;
    std::unordered_map<IntersectionIdx,Info > inter_info; 
    std::priority_queue<std::pair<double, IntersectionIdx>,std::vector<std::pair<double, IntersectionIdx>>, std::greater<std::pair<double, IntersectionIdx>> >  openset;
    openset.push(std::make_pair(0,intersect_id_start));
    Info start_node ;
    start_node.inopenset = true;
    start_node.cost = 0;
    inter_info[intersect_id_start] = start_node ; 
    int visited_points  = 0; 
    for (int i = 0 ; i<nodes.size() ; i++){
        in_nodes[nodes[i].pos] = true; 
     }
    while (!openset.empty()){

        auto curr(openset.top());
        openset.pop();
        auto curidx = curr.second; 
        Info curinfo = inter_info[curidx];
        if (visited_points == nodes.size()){
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
                    double priority = toptinfo.cost ; 
                    openset.push(std::make_pair(priority,topt));
                    toptinfo.inopenset = true;
                    toptinfo.came_from = curidx;
                    toptinfo.came_via = anedge;
                    inter_info[topt] = toptinfo;
                }else{
                    if (toptinfo.cost > cost_update){
                        toptinfo.cost = cost_update;
                        double priority = toptinfo.cost ; 
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
        if (in_nodes[curidx]){
                
            ret_path.clear();
            auto last = curidx;
            while(last != intersect_id_start){
                ret_path.push_back(inter_info[last].came_via);
                last = inter_info[last].came_from;
            }
            int time_to_a_node = computePathTravelTime(ret_path, turn_penalty);
            int kk = 0; 
            for(kk = 0 ; kk <= nodes.size() ; kk++){
                if (kk == nodes.size() ) break ; 
                if (curidx == nodes[kk].pos){
                    if (kk != nodes.size()){
                        if (edges[source][kk] == 2139062143){
                            edges[source][kk] = time_to_a_node;
                            visited_points += 1; 
                        }else {
                            if (edges[source][kk] > time_to_a_node){
                                edges[source][kk] = time_to_a_node;
                            }
                        }
                    }
                }
            }
            
        }

    }
}


// get nearest legal move for the inital path generate
int get_nearest_legal_move(int current, const std::vector<DeliveryInf>&, int packages[]){
    int next_node = -1; 
    int shortest_next = 0x7f7f7f7f; 
    for (int i = 0 ; i < deliv_num ; i++){
        if (i==current) continue;
        if ( ( ( (packages[nodess[i].package] == 1) && (!nodess[i].is_pickup) ) || ((packages[nodess[i].package] == 0) && (nodess[i].is_pickup)) ) ){
                    if (edges[current][i]  < shortest_next){
                        next_node = i;
                        shortest_next = edges[current][i];
                    }
        }
    }
    return next_node;
}

//find the nearest depot to  a spectfic node
int get_nearest_depot(const std::vector<int>& depots, int currpos, const double turn_penalty){
    int nearest_depot = -1 ; 
    int shortest_time = 0x7f7f7f7f; 
    for (int i = 0 ; i < depots.size() ; i++){
        auto temp_path = findPathBetweenIntersections(currpos, depots[i], turn_penalty);
        int temp_time = computePathTravelTime(temp_path, turn_penalty);
        if (temp_time < shortest_time){
            nearest_depot = i;
            shortest_time = temp_time ; 
        }
    }
    return nearest_depot; 
}

//get the time to the nearest depot (optimized on time)
int get_nearest_depot_time(int curidx, const double){
    int nearest_time = 0x7f7f7f7f; 
    for(int i = deliv_num ; i < nodess.size() ; i++){
        if (edges[curidx][i] < nearest_time){
            nearest_time = edges[curidx][i];
        }
    }
    return nearest_time; 
}

//given a route, calc the time need on that route. 
int get_time(const std::vector<PDpoint>route, const std::vector<int> , double turn_penalty){
    if(route.size() == 1) return 0x7f7f7f7f; 
    int ret_time = 0 ; 
    for (int i = 0 ; i < route.size()-1 ; i++){
        ret_time += edges[route[i].index][route[i+1].index];
    }
    ret_time += get_nearest_depot_time(route[0].index, turn_penalty);  
    ret_time += get_nearest_depot_time(route[route.size()-1].index, turn_penalty);  
    return ret_time; 
} 

//rotate the given path to find a simllar new path, using "3-opt" -> change three connections 
std::vector<PDpoint>  generate_new_path(const std::vector<PDpoint> path, int rt ,const std::vector<int>& depots,const float turn_penalty ){
    if (rt > path.size()) rt%=path.size() ;  // if rt is too big only rotate one node. 
    int start_op_pt = rand()%(path.size()-rt + 1);
    std::vector<int> lbnd, rbnd;
    std::vector <PDpoint> temp;
    std::vector <PDpoint> tempgood;
    tempgood = path;
    double besttime = 0x7f7f7f7f;
    
    //pre exclusive the illegal path, save the time to verify those
    for(int k =0 ; k < rt ; k++){
        if (path[k+start_op_pt].is_pickup) rbnd.push_back(path[k+start_op_pt].index+1);
        else if (!path[k+start_op_pt].is_pickup) lbnd.push_back(path[k+start_op_pt].index-1); 
    }


    // rotate left  
    int curpos = start_op_pt -1;
    bool die = false; 
    while(curpos >= 0 ){
        for (auto & key : lbnd){
            if (path[curpos].index == key){
                die = true;
                break;
            }
        }
        if (die) break;  
        double at = 0 ,bt =0 ;

        //calculate the time difference 
        if (curpos == 0){
            at += get_nearest_depot_time(path[curpos].index, turn_penalty);
            bt += get_nearest_depot_time(path[start_op_pt].index, turn_penalty);
        }
        else {
            at +=  edges[path[curpos-1].index][path[curpos].index]; 
            bt +=  edges[path[curpos-1].index][path[start_op_pt].index];
        }
        if (start_op_pt + rt == path.size()) {
            at += get_nearest_depot_time(path[path.size() - 1].index , turn_penalty);
            bt += get_nearest_depot_time(path[start_op_pt - 1].index, turn_penalty);
        }
        else {
            at += edges[path[start_op_pt+rt-1].index][path[start_op_pt+rt].index];
            bt += edges[path[start_op_pt -1 ].index][path[start_op_pt+rt].index];
        }
        at += edges[path[start_op_pt -1 ].index][path[start_op_pt].index];
        bt += edges[path[start_op_pt + rt - 1].index ][path[curpos].index];

        //approx the time diff, and decided if choose that soluation or not. 
        if ( 1.5*at > bt  ){
            temp = path; 
            for(int i = curpos ; i < curpos + rt  ; i++ ){
                temp[i] = path[start_op_pt + i -curpos];
            }
            for (int i = curpos + rt ;  i < start_op_pt + rt ; i++  ){
                temp[i] = path[i - rt ];
            }
            int temptime =  get_time(temp,depots,turn_penalty);
            if (temptime < besttime){
                tempgood = temp;
                besttime = temptime;
            }           
        }
        curpos -- ; 
    }



    // rotate right , same thing as above 
    
    die = false; 
    curpos = start_op_pt + rt ; 
    while(curpos < path.size() - rt ){
        for (auto & key : rbnd){
            if (path[curpos].index == key){
                die = true;
                break;
            }
        }
        if (die) break;  
        double at = 0 , bt =0 ; 
        if (start_op_pt == 0){
            at += get_nearest_depot_time(path[start_op_pt].index, turn_penalty);
            bt += get_nearest_depot_time(path[rt].index, turn_penalty);
        }
        else {
            at +=  edges[path[start_op_pt-1].index][path[start_op_pt].index]; 
            bt +=  edges[path[start_op_pt-1].index][path[start_op_pt+rt].index];
        }
        if (curpos+rt == path.size()) {
            at += get_nearest_depot_time(path[path.size() - 1].index , turn_penalty);
            bt += get_nearest_depot_time(path[curpos -1 ].index, turn_penalty);
        }
        else {
            at += edges[path[curpos+rt-1].index][path[curpos+rt].index];
            bt += edges[path[start_op_pt+rt-1].index ][path[curpos+rt].index];
        }
        at += edges[path[start_op_pt+rt-1].index][path[start_op_pt+rt].index]; 
        bt += edges[path[curpos + rt -1].index][path[start_op_pt].index]; 
        if (1.5*at > bt ){
            temp = path; 
            for(int i = start_op_pt ; i < curpos - rt  ; i++ ){
                temp[i] = path[rt + i];
            }
            for (int i = curpos - rt ; i < curpos ; i++  ){
                temp[i] = path[i-curpos+start_op_pt+rt];
            }
            int temptime = get_time(temp, depots, turn_penalty);
            if (temptime < besttime) {
                tempgood = temp;
                besttime = temptime;
            }
        }
        curpos ++ ; 
    }
    return tempgood;     
}

std::vector<CourierSubPath> travelingCourier(const std::vector<DeliveryInf>& deliveries,const std::vector<int>& depots,const float turn_penalty){ 
    auto start_jiujiu = std::chrono::high_resolution_clock::now();
    bool timeout = false;
    memset(edges,0x7f, sizeof(edges));//edges[i][j] = 0x7f7f7f7f
    nodess.clear() ;
    int indexx = 0; 
    for (int i = 0 ; i < deliveries.size() ; i++){
        nodess.push_back({indexx++, i,deliveries[i].pickUp,true,false});
        nodess.push_back({indexx++, i,deliveries[i].dropOff,false,false });
    }
    deliv_num = nodess.size(); 
    for (int i = 0 ; i < depots.size() ; i ++){
        nodess.push_back({indexx++, -1, depots[i], false, true}); 
    }


    //pre-calculation all the distance between nodes 
    #pragma omp parallel for
    for (int i  = 0 ; i < nodess.size(); i++){
        findPaths(nodess, i, turn_penalty);
    }



    //handle the situation of has no possible solutations. 

    not_possiable = false ; 

    for (int i  = 0 ; i < deliv_num ; i++){
        for(int j = 0 ; j < deliv_num ; j++){
            if (edges[i][j] == 0x7f7f7f7f) not_possiable = true; 
            break;
        }
        if (not_possiable) break ; 
    }

    if (!not_possiable){
        bool works = false; 
        for (int i = deliv_num ; nodess.size() ; i++){
            for (int j = deliv_num ; nodess.size() ; j++){
                if (edges[i][j] != 0x7f7f7f7f) works = true;
                break; 
            }
            if (works) break; 
        }
        if (!works) not_possiable = false; 
    }

    if (not_possiable) return {} ; 


    std::vector<PDpoint> all_path[MAXPOINTS + 1]; //Only even index is valid 
    std::vector<PDpoint> all_best; 
    int all_best_time = 0x7f7f7f7f; 


    //parallel start from all the (in practice only the first few) nodes. 
    #pragma omp parallel for 
    for (int starting_point = 0; starting_point < deliv_num ; starting_point+=2){
        if (!timeout){
            std::vector<PDpoint> path; 
            int packages[MAXPOINTS + 1];
            memset(packages,0x0, sizeof(packages));
            int curr = starting_point;
            bool done = false;
            path.push_back(nodess[curr]);
            packages[nodess[curr].package] = 1; 
            while(!done){ 
                bool alldelivered = true;
                for (int item = 0 ; item < deliveries.size() ; item ++ ){
                    if (packages[item] != 2 ){
                        alldelivered = false;
                        break;
                    }
                }
                if (alldelivered) break ; 
                auto next = get_nearest_legal_move(curr, deliveries,packages);
                if(nodess[next].is_pickup){
                    packages[nodess[next].package] = 1;
                }else {
                    packages[nodess[next].package] = 2; 
                }
                nodess[next].is_visited = true; 
                path.push_back(nodess[next]);
                curr = next;

            }
            
            // SA process, calling the get_new_path to generate a new path and 
            // base on the SA algo to determin if we should accept that soluation             
            auto best = path; 
            auto best_tot_time = get_time(path, depots, turn_penalty);
            
            auto path_time = best_tot_time;
            double temp, LIMIT;
            
            //constants for different size of problem
            if (deliveries.size()>=150){
                temp = 200;
                LIMIT = 20;
            } else{
                temp = 2000;
                LIMIT = 100;
            }

            while(true){
                auto current_jiujiu = std::chrono::high_resolution_clock::now();
                auto wall_jiujiu = std::chrono::duration_cast<chrono::duration<double>> (current_jiujiu - start_jiujiu);
                if(wall_jiujiu.count() > 46) {timeout = true;break;} 
                if (temp < TEND) 
                    break;
                for (int rt = 1 ; rt < LIMIT ; rt++){
                    auto new_path = generate_new_path(path, rt, depots, turn_penalty);
                    int new_path_time = get_time(new_path, depots, turn_penalty);
                    double delta = new_path_time - path_time;
                    
                    //if has better soluation, accept it, otherwise accept is by this equation: exp(-(delta)/(15*temp));
                    if (delta < 0){
                        path = new_path;
                        path_time = new_path_time;
                        if (path_time < best_tot_time){
                            best = path; 
                            best_tot_time = path_time;
                        }
                        break ; 
                    }else{
                        double p = exp(-(delta)/(temp));
                        p = ACCEPT_BAD *p; 
                        // if (fabs(delta) < 1) continue;
                        double temp12 = rand() / (double) RAND_MAX;
                        
                        if ((temp12) < p){
                            path = new_path;
                            path_time = new_path_time;  
                            break ; 
                        }
                    }
                    
                }
                temp = temp * A;
            }
            
            //update the soluation to the global variable. 
            #pragma omp critical 
            if (best_tot_time < all_best_time){
                all_best_time = best_tot_time;
                all_best = best; 
            }
        }
    }

    

    //generate the final answer, all calculation should be done before this
    //Note, using findpathbetweenIntersections here, could be optimized!
    auto path = all_best; 
    std::vector<CourierSubPath> ret_path ;
    //cout << path.size()<< endl; 
    int start = get_nearest_depot(depots, path[0].pos, turn_penalty);
    //cout << "Finishe  d"<<endl; 
    ret_path.push_back({depots[start], path[0].pos, findPathBetweenIntersections(depots[start], path[0].pos,turn_penalty)});
    //cout << "Finished"<<endl;
    for (int i = 1 ; i < path.size(); i++){
        ret_path.push_back({path[i-1].pos, path[i].pos, findPathBetweenIntersections(path[i-1].pos, path[i].pos,turn_penalty)});
    }
//cout << "Finished1"<<endl;
    int end = get_nearest_depot(depots, path[path.size()-1].pos, turn_penalty);
    ret_path.push_back({ path[path.size()-1].pos,depots[end], findPathBetweenIntersections(path[path.size()-1].pos,depots[end],turn_penalty)});
    return ret_path; 
}

