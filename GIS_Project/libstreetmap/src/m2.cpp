#include <curl/curl.h>
#include <cstdlib>

#include "m2.h"
#include "m1.h"
#include "my_m2.h"
#include "drawroad.h"
#include "streetsuper.h"
#include "global.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "algorithm"
#include "nodes.h"
#include "rapidjson/rapidjson.h" // An external json parsing library 
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"
#include "m3.h"



float minlong, minlat,  maxlong,  maxlat;
float avg_lat_of_this_map = (minlat + maxlat)/2;
bool first_draw = true ;
std::vector<struct intersection_data> intersections;
std::unordered_map<int, bool> poi_light; // if a poi should be labelled
ezgl::application* applic;               // global locator for the application 
std::vector<std::string> streetnames;  
std::unordered_map<std::string, bool> POIstatus;  //if a poi is being set to displayed
std::string current_city = "toronto_canada";       // by default the opening city is toronto
bool night_bool = false; // if in night mode 
bool subway_bool = false; // show subway info?
bool POImode = false;  // clicking for POI?
bool Intersection_mode = false;
bool Find_path_mode = true;
std::vector<std::pair<ezgl::point2d, std::string>> lake_text, park_text ;  // Caching lake and park info 
IntersectionIdx start = -1;
IntersectionIdx end = -1;

void show_message (GtkWindow *parent,const gchar *title , const gchar *message){
 GtkWidget *dialog, *label, *content_area;
 GtkDialogFlags flags;
 flags = GTK_DIALOG_DESTROY_WITH_PARENT;
 dialog = gtk_dialog_new_with_buttons (title,
                                       parent,
                                       flags,
                                       ("OK"),
                                       GTK_RESPONSE_NONE,
                                       NULL);
 content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
 label = gtk_label_new (message);

 g_signal_connect_swapped (dialog,
                           "response",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog);

 gtk_container_add (GTK_CONTAINER (content_area), label);
 gtk_widget_show_all (dialog);
}


void set_min_max_long_lat(){        // setting the max/min lat.lon for this whole map 
    
    intersections.resize(getNumIntersections());

    for (int id = 0 ; id < getNumIntersections(); id++){
        float x = (getIntersectionPosition(id).longitude());
        float y = (getIntersectionPosition(id).latitude()); 
        
        intersections[id].position = getIntersectionPosition(id);
        intersections[id].name = getIntersectionName(id);
        
        if (!id){
            minlong = x ;
            minlat = y ;
            maxlong = x ;
            maxlat = y ;
        }
        minlong = x < minlong ? x : minlong;
        minlat = y < minlat ? y : minlat;
        maxlong = x > minlong ? x : maxlong;
        maxlat = y > minlat ? y : maxlat;
    }

    avg_lat_of_this_map = (minlat + maxlat)*kDegreeToRadian/2;
    minlat *= kDegreeToRadian * kEarthRadiusInMeters ;
    maxlat *= kDegreeToRadian * kEarthRadiusInMeters;
    minlong *= kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
    maxlong *= kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
    return ; 
}

double lat_from_y(double y) {
    return y/(kDegreeToRadian * kEarthRadiusInMeters);
}

double lon_from_x(double x) {
    return x/(kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters);
}

double y_from_lat(double lat) {
    return lat * kDegreeToRadian * kEarthRadiusInMeters;
}

double x_from_lon(double lon) {
    return lon * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
}

//Call back function for curl to write the socket package to a buffer 
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

//main function for drawing the main canvas
void draw_main_canvas (ezgl::renderer *g){ 
    std::cout  <<"[DEBUG] Number of Nodes: " << getNumberOfNodes() << std::endl;
    std::cout  <<"[DEBUG] visible world: " << g->get_visible_world().width() << std::endl;
    
    streetnames.clear();
    
    // opening the PNG images 
    ezgl::surface *red_pin = ezgl::renderer::load_png("libstreetmap/resources/map-pin-red.png");
    ezgl::surface *poi = ezgl::renderer::load_png("libstreetmap/resources/map-pin-fill.png");
    ezgl::surface *subway_symbol = ezgl::renderer::load_png("libstreetmap/resources/subway-fill.png");
    ezgl::surface *compass = ezgl::renderer::load_png("libstreetmap/resources/compass-3-line.png");
    
    if(first_draw){     //set the window to full screen. 
        GObject* window = applic->get_object("MainWindow");
        gtk_window_fullscreen((GtkWindow*)window);
        first_draw = false;
        ezgl::renderer::free_surface(compass);
        ezgl::renderer::free_surface(red_pin);
        ezgl::renderer::free_surface(poi);
        ezgl::renderer::free_surface(subway_symbol);
        return; 
    }
       
    if (night_bool){       // using night mode?
        g->set_color(ezgl::color(83,83,83,255));
        g->fill_rectangle(g->get_visible_world().top_left(), g->get_visible_world().bottom_right());
    }
    
    //draw features amd save the overlaping strings for future use. 
    lake_text.clear();
    park_text.clear();
    for(int id = 0; id < getNumFeatures(); ++id) {
        if (getNumFeaturePoints(id) == 1) continue;
        if (!(getFeaturePoint(id, 0) == getFeaturePoint(id, getNumFeaturePoints(id)-1))) continue;
        std::vector<ezgl::point2d> points;
        points.clear();
        double xxx=0, yyy =0; 
        for(int featureId = 0; featureId < getNumFeaturePoints(id); ++featureId) {
            
            double x = x_from_lon(getFeaturePoint(id, featureId).longitude());
            double y = y_from_lat(getFeaturePoint(id, featureId).latitude());
            xxx += x;
            yyy += y;
            points.push_back({x, y});
        }
        xxx /= getNumFeaturePoints(id);
        yyy /= getNumFeaturePoints(id);
        
        if (!night_bool){
            if(getFeatureType(id) == UNKNOWN) {             //Setting colors for daylight mode 
               g->set_color(ezgl::BLACK);
            } else if(getFeatureType(id) == PARK) {
               g->set_color(ezgl::color(171,218,185, 255));
            } else if(getFeatureType(id) == BEACH) {
               g->set_color(ezgl::color(238,238,194, 255));
            } else if(getFeatureType(id) == LAKE) {
               g->set_color(ezgl::color(147,183,247, 255));
            } else if(getFeatureType(id) == RIVER) {
               g->set_color(ezgl::color(147,183,247, 255));
            } else if(getFeatureType(id) == ISLAND) {
               g->set_color(ezgl::color(171,218,185, 255));
            } else if(getFeatureType(id) == BUILDING) {
                if(g->get_visible_world().width()>3000) continue;
               g->set_color(ezgl::color((unsigned char)'\277', (unsigned char)'\277', (unsigned char)'\277', (unsigned char)'\277'));
            } else if(getFeatureType(id) == GREENSPACE) {
               g->set_color(ezgl::color(171,218,185, 255));
            } else if(getFeatureType(id) == GOLFCOURSE) {
               g->set_color(ezgl::color(46,158,77, 255));
            } else if(getFeatureType(id) == STREAM) {
               g->set_color(ezgl::color(147,183,247, 255));
            } 
        } else {
            if(getFeatureType(id) == UNKNOWN) {                 //setting colors for dark mode
               g->set_color(ezgl::BLACK);
            } else if(getFeatureType(id) == PARK) {
               g->set_color(ezgl::color(171,218,185, 255));
            } else if(getFeatureType(id) == BEACH) {
               g->set_color(ezgl::color(238,238,194, 255));
            } else if(getFeatureType(id) == LAKE) {
               g->set_color(ezgl::color(0,98,149, 255));
            } else if(getFeatureType(id) == RIVER) {
               g->set_color(ezgl::color(0,98,149, 255));
            } else if(getFeatureType(id) == ISLAND) {
               g->set_color(ezgl::color(171,218,185, 255));
            } else if(getFeatureType(id) == BUILDING) {
                if(g->get_visible_world().width()>3000) continue;
               g->set_color(ezgl::color((unsigned char)'\277', (unsigned char)'\277', (unsigned char)'\277', (unsigned char)'\277'));
            } else if(getFeatureType(id) == GREENSPACE) {
               g->set_color(ezgl::color(171,218,185, 255));
            } else if(getFeatureType(id) == GOLFCOURSE) {
               g->set_color(ezgl::color(46,158,77, 255));
            } else if(getFeatureType(id) == STREAM) {
               g->set_color(ezgl::color(147,183,247, 255));
            } 
        }

        g->fill_poly(points);
        std::string name = getFeatureName(id);

        if ((getFeatureType(id) == PARK || getFeatureType(id) == LAKE) && name != "<noname>" && findFeatureArea(id) > 200000){
            if(getFeatureType(id) == PARK) {
                g->set_font_size(10);
                ezgl::point2d temp = {xxx,yyy};
                park_text.push_back(std::make_pair(temp,name));
            } 
            if(getFeatureType(id) == LAKE) {
                g->set_font_size(10);
                ezgl::point2d temp = {xxx,yyy};
                lake_text.push_back(std::make_pair(temp,name));
            } 
        }
    }

    
    
    
    //draw streetsegments
    std::cout << "[DEBUG] draw streetsegments #1" <<std::endl;
    g->set_line_cap(ezgl::line_cap::round);
    for(int id = 0; id < getNumStreetSegments(); ++id) {
        double x1 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).from).longitude());
        double y1 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).from).latitude());
        double x2 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).to).longitude());
        double y2 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).to).latitude());
        if (!((x1 > g->get_visible_world().left()-1000)&&(x2 < g->get_visible_world().right()+1000+200)&&(y1 > g->get_visible_world().bottom()-1000)&&(y2 < g->get_visible_world().top()+1000))){
            continue;
        }
        if(getStreetSegmentInfo(id).numCurvePoints == 0) {
            if(Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway"          // drawing lower level streets. 
                    || Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                continue;
            } else if((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway_link")){
                continue; 
            } else if ((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "primary") || 
                        (Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "secondary")){
                if(g->get_visible_world().width()>35000) continue;
              
                g->set_color(ezgl::color(255, 255, 204, 255));
                if(g->get_visible_world().width()>20000) {
                g->set_line_width(35000.00/g->get_visible_world().width());
                } else if(g->get_visible_world().width()<5000) g->set_line_width(4+2000/g->get_visible_world().width());
                    else g->set_line_width(20000.00/g->get_visible_world().width());
                g->draw_line({x1, y1}, {x2, y2});
            } else {
                    if(g->get_visible_world().width()>5000) continue;
                    g->set_color(ezgl::color(0xFF, 0xFF, 0xFF,255));
                    g->set_line_width(2 + 5000/g->get_visible_world().width());
                    g->draw_line({x1, y1}, {x2, y2});
                } 
        } else {                                                                                       // draw curve  roads 
            for(int curveId = 0; curveId < getStreetSegmentInfo(id).numCurvePoints; ++curveId ) {
                x2 = x_from_lon(getStreetSegmentCurvePoint(id, curveId).longitude());
                y2 = y_from_lat(getStreetSegmentCurvePoint(id, curveId).latitude());
            if(Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway" || 
                        Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                continue; 
            } else if((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway_link") && 
                        (g->get_visible_world().width()<5000)){
                continue;
            } else if ((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "primary") || 
                        (Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "secondary")){
                if(g->get_visible_world().width()>35000) continue;
              
                g->set_color(ezgl::color(255, 255, 204, 255));
                if(g->get_visible_world().width()>20000) {
                g->set_line_width(35000.00/g->get_visible_world().width());
                } else if(g->get_visible_world().width()<5000) g->set_line_width(4+2000/g->get_visible_world().width());
                    else g->set_line_width(20000.00/g->get_visible_world().width());
                g->draw_line({x1, y1}, {x2, y2});
            } else {
                    if(g->get_visible_world().width()>5000) continue;
                    g->set_color(ezgl::color(0xFF, 0xFF, 0xFF,255));
                    g->set_line_width(2 + 5000/g->get_visible_world().width());
                    g->draw_line({x1, y1}, {x2, y2});
                } 
                    x1 = x2;
                    y1 = y2;
            }
            x2 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).to).longitude());
            y2 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).to).latitude());
            if(Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway" || 
                        Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                continue; 
            } else if((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway_link") ){
                continue; 
            } else if ((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "primary") || 
                        (Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "secondary")){
                if(g->get_visible_world().width()>35000) continue;
                
                g->set_color(ezgl::color(255, 255, 204, 255));
                if(g->get_visible_world().width()>20000) {
                g->set_line_width(35000.00/g->get_visible_world().width());
                } else if(g->get_visible_world().width()<5000) g->set_line_width(4+2000/g->get_visible_world().width());
                    else g->set_line_width(20000.00/g->get_visible_world().width());
                g->draw_line({x1, y1}, {x2, y2});
                
            } else {
                    if(g->get_visible_world().width()>5000) continue;
                    g->set_color(ezgl::color(0xFF, 0xFF, 0xFF,255));
                    g->set_line_width(2 + 5000/g->get_visible_world().width());
                    g->draw_line({x1, y1}, {x2, y2});
                } 
        }
    }
    
    
    std::cout << "[DEBUG] draw streetsegments #2" <<std::endl;                                      // draw high level roads. 
    for(int id = 0; id < getNumStreetSegments(); ++id) {
        double x1 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).from).longitude());
        double y1 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).from).latitude());
        double x2 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).to).longitude());
        double y2 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).to).latitude());
        if (!((x1 > g->get_visible_world().left()-1000)&&(x2 < g->get_visible_world().right()+1000)&&(y1 > g->get_visible_world().bottom()-1000)&&(y2 < g->get_visible_world().top()+1000))){
            continue;
        }
        if(getStreetSegmentInfo(id).numCurvePoints == 0) {
            if((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway") || 
                        Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                g->set_color(ezgl::color(255, 204, 0,255));
                if(g->get_visible_world().width()>20000) {
                g->set_line_width(50000.00/g->get_visible_world().width());
                } else if(g->get_visible_world().width()<5000) g->set_line_width(8+2000/g->get_visible_world().width());
                    else g->set_line_width(30000.00/g->get_visible_world().width());
                g->draw_line({x1, y1}, {x2, y2});
            } else if((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway_link") && 
                        (g->get_visible_world().width()<5000)){
                g->set_color(ezgl::color(255, 204, 0,255));
                g->set_line_width(5+2000.00/g->get_visible_world().width());
                g->draw_line({x1, y1}, {x2, y2});
            } 
        } else {
            x1 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).from).longitude());
            y1 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).from).latitude());
            for(int curveId = 0; curveId < getStreetSegmentInfo(id).numCurvePoints; ++curveId ) {
                x2 = x_from_lon(getStreetSegmentCurvePoint(id, curveId).longitude());
                y2 = y_from_lat(getStreetSegmentCurvePoint(id, curveId).latitude());
                if(Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway" || 
                            Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                    g->set_color(ezgl::color(255, 204, 0,255));
                    if(g->get_visible_world().width()>20000) {
                g->set_line_width(50000.00/g->get_visible_world().width());
                } else if(g->get_visible_world().width()<5000) g->set_line_width(8+2000/g->get_visible_world().width());
                    else g->set_line_width(30000.00/g->get_visible_world().width());
                    g->draw_line({x1, y1}, {x2, y2});
                } else if((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway_link") && 
                        (g->get_visible_world().width()<5000)){
                g->set_color(ezgl::color(255, 204, 0,255));
                g->set_line_width(5+2000.00/g->get_visible_world().width());
                g->draw_line({x1, y1}, {x2, y2});
            } 
                
                    x1 = x2;
                    y1 = y2;
            }
            x2 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).to).longitude());
            y2 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).to).latitude());
                if(Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway" || 
                        Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                g->set_color(ezgl::color(255, 204, 0,255));
                if(g->get_visible_world().width()>20000) {
                g->set_line_width(50000.00/g->get_visible_world().width());
                } else if(g->get_visible_world().width()<5000) g->set_line_width(8+2000/g->get_visible_world().width());
                    else g->set_line_width(30000.00/g->get_visible_world().width());
                g->draw_line({x1, y1}, {x2, y2});
            } else if((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway_link") && 
                        (g->get_visible_world().width()<5000)){
                g->set_color(ezgl::color(255, 204, 0,255));
                g->set_line_width(5+2000.00/g->get_visible_world().width());
                g->draw_line({x1, y1}, {x2, y2});
            } 
        }
    }
   
   
    std::cout << "[DEBUG] draw streetsegments #3" <<std::endl;                                      // put text and arrow on roads. 
    if (current_city != "tehran_iran")
        g->format_font("Noto Sans CJK SC",ezgl::font_slant::normal, ezgl::font_weight::normal, 10);
    if (current_city == "tehran_iran")
        g->format_font("monospace",ezgl::font_slant::normal, ezgl::font_weight::normal, 10);
    for(int id = 0; id < getNumStreetSegments(); id+=3) {
        if(getStreetName(getStreetSegmentInfo(id).streetID) == "<unknown>") continue;
        if(getStreetSegmentInfo(id).numCurvePoints == 0) {
            double x1 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).from).longitude());
            double y1 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).from).latitude());
            double x2 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).to).longitude());
            double y2 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).to).latitude());
            if(g->get_visible_world().width()<2000) {
                double angle;
                    if(x1-x2 == 0) angle = 90;
                    else angle = atan((y1-y2)/(x1-x2))/kDegreeToRadian;
                    g->set_font_size(10);
                    g->set_text_rotation(angle);
                    g->set_color(ezgl::BLACK);
                    g->set_horiz_text_just(ezgl::text_just::center);
                    g->draw_text({(x1+x2)/2, (y1+y2)/2}, getStreetName(getStreetSegmentInfo(id).streetID));
                    //std::cout << getStreetName(getStreetSegmentInfo(id).streetID) << std::endl;
            }
            if(Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway" || 
                        Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                if((getStreetName(getStreetSegmentInfo(id).streetID) != "<unknown>")&&((g->get_visible_world().width()<2000))) {
                    
                   double angle;
                    if(x1-x2 == 0) angle = 90;
                    else angle = atan((y1-y2)/(x1-x2))/kDegreeToRadian;
                   
                    g->set_text_rotation(angle);
                    g->set_color(ezgl::BLACK);
                    g->draw_text({x1,y1}, getStreetName(getStreetSegmentInfo(id).streetID),findStreetSegmentLength(id)*2, findStreetSegmentLength(id)*2);
                }
            }
        } else {
            LatLon point1 = getIntersectionPosition(getStreetSegmentInfo(id).from);
            double x1 = x_from_lon(point1.longitude());
            double y1 = y_from_lat(point1.latitude());
            for(int curveId = 0; curveId < getStreetSegmentInfo(id).numCurvePoints; ++curveId ) {
                if (curveId % 2 == 0) continue;
                LatLon point2 = getStreetSegmentCurvePoint(id, curveId);
                double x2 = x_from_lon(point2.longitude());
                double y2 = y_from_lat(point2.latitude());
            if(Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway" || 
                        Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                    if((getStreetName(getStreetSegmentInfo(id).streetID) != "<unknown>")&&((g->get_visible_world().width()<2000))) {
                    double angle;
                    if(x1-x2 == 0) angle = 90;
                    else angle = atan((y1-y2)/(x1-x2))/kDegreeToRadian;
                    
                    double strlen = 2 * findDistanceBetweenTwoPoints(std::make_pair(point1, point2));
                    g->set_text_rotation(angle);
                    g->set_color(ezgl::BLACK);
                    g->draw_text({x1,y1}, getStreetName(getStreetSegmentInfo(id).streetID),strlen, strlen);
                    }
                } else if((Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway_link") &&   
                        (g->get_visible_world().width()<2000) && curveId == getStreetSegmentInfo(id).numCurvePoints /2  ){
                   
                    double angle;
                    if(x1-x2 == 0) angle = 90;
                    else angle = atan((y1-y2)/(x1-x2))/kDegreeToRadian;
          
                    g->set_text_rotation(angle);
                    g->set_color(ezgl::BLACK);
                    g->draw_text({x1,y1}, "→");
                    
            } 
                
                    x1 = x2;
                    y1 = y2;
            }
            
                double x2 = x_from_lon(getIntersectionPosition(getStreetSegmentInfo(id).to).longitude());
                double y2 = y_from_lat(getIntersectionPosition(getStreetSegmentInfo(id).to).latitude());
                if((getStreetSegmentInfo(id).oneWay == true) && ((g->get_visible_world().width()<2000))) {
                double angle;
                    if(x1-x2 == 0) angle = 90;
                    else angle = atan((y1-y2)/(x1-x2))/kDegreeToRadian;
                if(angle < 0) angle = angle + 180;
                    g->set_text_rotation(angle);
                    g->set_color(ezgl::BLACK);
                    g->draw_text({x1,y1}, "→");
            }
            if(Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "motorway" || 
                        Hashtable_for_roads->roads[getStreetSegmentInfo(id).wayOSMID] == "trunk") {
                    if((getStreetName(getStreetSegmentInfo(id).streetID) != "<unknown>")&&((g->get_visible_world().width()<2000))) {
                    double angle;
                    if(x1-x2 == 0) angle = 90;
                    else angle = atan((y1-y2)/(x1-x2))/kDegreeToRadian;
                    if(angle < 0) angle = angle + 180;                    
                    g->set_text_rotation(angle);
                    g->set_color(ezgl::BLACK);
                }
                }
        }
    }
    
   //draw feature text 
    if (g->get_visible_world().width()<25000){
        for (int i = 0; i< lake_text.size() ; i++ ){
            g->set_color(ezgl::DARK_SLATE_BLUE);
            g->draw_text(lake_text[i].first,lake_text[i].second);
        }
        for (int i = 0; i< park_text.size() ; i++){
            g->set_color(ezgl::DARK_GREEN);
            g->draw_text(park_text[i].first,park_text[i].second);
        }
    }
    
    // draw intersections 
    std::cout << "[DEBUG] draw intersection" <<std::endl;
    for (size_t id = 0; id < getNumIntersections(); ++id) {
        float x = getIntersectionPosition(id).longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
        float y = getIntersectionPosition(id).latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
        if(intersections[id].highlight) {
            g->draw_surface(red_pin, {x-(g->get_visible_world().width())/100, y+(g->get_visible_world().width())/100});
        } else if (intersections[id].result){
            g->draw_surface(poi, {x-(g->get_visible_world().width())/100, y+(g->get_visible_world().width())/100});
        }
    }

    //draw path
    if (Find_path_mode){
        if(start!=-1&&end!=-1 ) {
            std::vector <StreetSegmentIdx> path = findPathBetweenIntersections(start, end, 15 ); //185496, 43413, 15 )
            if(path.size() == 0) 
                    show_message((GtkWindow*)applic->get_object("MainWindow"), (const gchar*)"Error", (const gchar*)"Error: No legal path!");
            if (path.size() != 0 ){
                double time = computePathTravelTime(path, 15);
                std::string Pathinfo = "Path From: ";
                Pathinfo += intersections[start].name;
                Pathinfo += " to ";
                Pathinfo += intersections[end].name;
                Pathinfo += ". Time: ";
                if  ((int)time/60 > 0 ) {
                    Pathinfo += std::to_string((int)time/60);
                    if (time/60 < 1) 
                        Pathinfo += " min ";
                    else 
                        Pathinfo += " mins ";
                }
                Pathinfo += std::to_string((int)(time - 60*((int)time/60)));
                Pathinfo += " seconds";
                GObject * bar = applic->get_object("StatusBar");
                gtk_statusbar_pop((GtkStatusbar *)bar , 0);
                gtk_statusbar_push((GtkStatusbar *)bar , 0 , Pathinfo.c_str());
                std::string guide="Starting From: ";
                if (intersections[start].name != "<unknown>")     guide += intersections[start].name;
                else guide+="Unknown Road";
                guide += "\n";
                std::string last_street =  getStreetName(getStreetSegmentInfo(path[0]).streetID);
                double this_time = 0; 
                for (int k = 0 ; k < path.size() ; k++ ){
                    if (getStreetName(getStreetSegmentInfo(path[k]).streetID) == last_street && k!=path.size()-1){
                        this_time += findStreetSegmentLength(path[k]);
                        
                    }else{
                        if (last_street == "<unknown>") last_street = "Unknown Road";
                        guide += "Driving along ";
                        this_time += findStreetSegmentLength(path[k]);
                        guide += last_street;
                        guide += " for ";
                        if (this_time > 2000){
                            guide += std::to_string((int)this_time/1000);
                            guide += " Kilometers\n";
                        }else {
                            guide += std::to_string((int)this_time);
                            guide += " meters\n";
                        }
                        if ( k < path.size()){
                            auto thisst = getStreetSegmentInfo(path[k-1]);
                            auto nextst = getStreetSegmentInfo(path[k]);
                            
                            IntersectionIdx com = -1 ; //com is the middle point
                            if(thisst.from == nextst.from || thisst.from == nextst.to){
                                com = thisst.from; 
                            }else if (thisst.to == nextst.to || thisst.to == nextst.from ){
                                com = thisst.to; 
                            } 
                            auto lastint = (thisst.from == com) ? thisst.to : thisst.from; 
                            auto nextint = (nextst.from == com) ? nextst.to : nextst.from; 
                            if (nextint == end) continue ; 
                            std::cout << getIntersectionName(lastint) << " " << getIntersectionName(com) <<" " << getIntersectionName(nextint) << std::endl ;
                            // double k1 = -fabs(y_from_lat(getIntersectionPosition(lastint).latitude()) - y_from_lat(getIntersectionPosition(com).latitude()))/ (x_from_lon(getIntersectionPosition(lastint).longitude()) - x_from_lon(getIntersectionPosition(com).longitude()));
                            // double k2 = -fabs(y_from_lat(getIntersectionPosition(com).latitude()) - y_from_lat(getIntersectionPosition(nextint).latitude())) / (x_from_lon(getIntersectionPosition(com).longitude()) - x_from_lon(getIntersectionPosition(nextint).longitude()));
                            double y1 = y_from_lat(getIntersectionPosition(lastint).latitude());
                            double y2 = y_from_lat(getIntersectionPosition(com).latitude());
                            double y3 = y_from_lat(getIntersectionPosition(nextint).latitude());
                            double x1 = x_from_lon(getIntersectionPosition(lastint).longitude());
                            double x2 = x_from_lon(getIntersectionPosition(com).longitude());
                            double x3 = x_from_lon(getIntersectionPosition(nextint).longitude()); 
                            double ans = (x2-x1)*(y3-y1)-(y2-y1)*(x3-x1);
                            printf("%lf,%lf,%lf,%lf,%lf,%lf\n",x1,y1,x2,y2,x3,y3);
                            if(ans>0) 
                                guide += "Turn Left\n";  
                            if(ans<0) 
                                guide += "Turn Right\n"; 
                            if(ans==0)
                                guide += "Go stright\n";
                        

                        }
                        this_time = 0 ; 
                        last_street = getStreetName(getStreetSegmentInfo(path[k]).streetID);
                        
                    }
                    
                }
                guide += "Arrived at: ";
                guide += intersections[end].name;
                auto currwindow = applic -> get_object("MainWindow");
                show_message((GtkWindow*)currwindow,(const gchar*) "Navigation", (const gchar*)guide.c_str());
                
                g->set_line_cap(ezgl::line_cap::round);
                for(int i = 0;i < path.size();i++) {
                    StreetSegmentInfo street = getStreetSegmentInfo(path[i]);
                    LatLon beginpt = getIntersectionPosition(street.from);
                    LatLon endpt = getIntersectionPosition(street.to);
                    float x1 = x_from_lon(beginpt.longitude());
                    float y1 = y_from_lat(beginpt.latitude());
                    float x2 = x_from_lon(endpt.longitude());
                    float y2 = y_from_lat(endpt.latitude());
                    g->set_color(ezgl::color(85, 131, 253, 0xff));
                    g->set_line_width(10);
                    if (street.numCurvePoints == 0){
                        //No curve points -- just calculate from start to end
                        g->draw_line({x1, y1}, {x2, y2});
                        
                    } else{
                        //if Curve points -- calculate by single segments
                        LatLon curvePt1, curvePt2;
                        curvePt1 = beginpt;
                        std::pair<LatLon, LatLon> curve_seg;

                        for (int j = 0; j< street.numCurvePoints;j++) {

                            curvePt2 =  getStreetSegmentCurvePoint(path[i],j);
                            x1 = x_from_lon(curvePt1.longitude());
                            y1 = y_from_lat(curvePt1.latitude());
                            x2 = x_from_lon(curvePt2.longitude());
                            y2 = y_from_lat(curvePt2.latitude());
                            g->draw_line({x1, y1}, {x2, y2});
                            curvePt1 = curvePt2;
                        }
                        x1 = x_from_lon(curvePt1.longitude());
                        y1 = y_from_lat(curvePt1.latitude());
                        x2 = x_from_lon(endpt.longitude());
                        y2 = y_from_lat(endpt.latitude());
                        g->draw_line({x1, y1}, {x2, y2});
                    } 
                }

                //draw path direction
                if(true) {
            double length = 0;
            for(int i = 0;i < path.size();i++) {
                if(i == path.size()-1) continue;
                StreetSegmentInfo street = getStreetSegmentInfo(path[i]);
                StreetSegmentInfo street_next = getStreetSegmentInfo(path[i+1]);
                LatLon beginpt, endpt;
                if(street_next.from == street.from||street_next.to == street.from) {
                    beginpt = getIntersectionPosition(street.to);
                    endpt = getIntersectionPosition(street.from); 
                } else {
                    beginpt = getIntersectionPosition(street.from);
                    endpt = getIntersectionPosition(street.to);
                    }
                length += findDistanceBetweenTwoPoints(std::make_pair(beginpt, endpt))/g->get_visible_world().width();
                if(length < 0.05) {
                    continue;
                }
                length = 0;
                
                
                g->set_color(ezgl::WHITE);
                g->set_line_width(3);
                if (street.numCurvePoints == 0){
                    //No curve points -- just calculate from start to end
                    float x1 = x_from_lon(beginpt.longitude());
                    float y1 = y_from_lat(beginpt.latitude());
                    float x2 = x_from_lon(endpt.longitude());
                    float y2 = y_from_lat(endpt.latitude());
                    double angle;
                    if(x1-x2 == 0) angle = 90 * kDegreeToRadian;
                    else angle = abs(atan((y1-y2)/(x1-x2)));
                    angle = angle -45 * kDegreeToRadian;
                    
                    ezgl::rectangle box = ezgl::rectangle({x1, y1}, {x2, y2});
                    ezgl::rectangle screen = g->world_to_screen(box);
                    g->set_coordinate_system(ezgl::SCREEN);
                    double start_x = (screen.right()+screen.left())/2;
                    double start_y = (screen.top()+screen.bottom())/2;
                    double end1_x, end1_y, end2_x, end2_y;
                    
                    if(angle >= 0) {
                    if(x2-x1 >= 0 && y2-y1 >= 0) { //1
                    end1_x = start_x - 5*sqrt(2)*cos(angle);
                    end1_y = start_y + 5*sqrt(2)*sin(angle);
                    end2_x = start_x + 5*sqrt(2)*sin(angle);
                    end2_y = start_y + 5*sqrt(2)*cos(angle);
                    } else if(x2-x1 < 0 && y2 -y1 > 0) { //2
                        end1_x = start_x + 5*sqrt(2)*cos(angle);
                        end1_y = start_y + 5*sqrt(2)*sin(angle);
                        end2_x = start_x - 5*sqrt(2)*sin(angle);
                        end2_y = start_y + 5*sqrt(2)*cos(angle);
                    } else if(x2 -x1 < 0 && y2 - y1 < 0) { //3
                        end1_x = start_x + 5*sqrt(2)*cos(angle);
                        end1_y = start_y - 5*sqrt(2)*sin(angle);
                        end2_x = start_x - 5*sqrt(2)*sin(angle);
                        end2_y = start_y - 5*sqrt(2)*cos(angle);
                    } else { //4
                        end1_x = start_x - 5*sqrt(2)*cos(angle);
                        end1_y = start_y - 5*sqrt(2)*sin(angle);
                        end2_x = start_x + 5*sqrt(2)*sin(angle);
                        end2_y = start_y - 5*sqrt(2)*cos(angle);
                    }
                    } else {
                        double angle1 = -angle;
                       if(x2-x1 >= 0 && y2-y1 >= 0) { //1
                        end1_x = start_x - 5*sqrt(2)*sin(angle1);
                        end1_y = start_y + 5*sqrt(2)*cos(angle1);
                        end2_x = start_x - 5*sqrt(2)*cos(angle1);
                        end2_y = start_y - 5*sqrt(2)*sin(angle1);
                    } else if(x2-x1 < 0 && y2 -y1 > 0) { //2
                        end1_x = start_x + 5*sqrt(2)*cos(angle);
                        end1_y = start_y + 5*sqrt(2)*sin(angle);
                        end2_x = start_x - 5*sqrt(2)*sin(angle);
                        end2_y = start_y + 5*sqrt(2)*cos(angle);
                    } else if(x2 -x1 < 0 && y2 - y1 < 0) { //3
                        end1_x = start_x + 5*sqrt(2)*sin(angle1);
                        end1_y = start_y - 5*sqrt(2)*cos(angle1);
                        end2_x = start_x + 5*sqrt(2)*cos(angle1);
                        end2_y = start_y + 5*sqrt(2)*sin(angle1);
                    } else { //4
                        end1_x = start_x - 5*sqrt(2)*cos(angle);
                        end1_y = start_y - 5*sqrt(2)*sin(angle);
                        end2_x = start_x + 5*sqrt(2)*sin(angle);
                        end2_y = start_y - 5*sqrt(2)*cos(angle);
                    } 
                    }
                    
                    g->set_coordinate_system(ezgl::SCREEN);
                    g->draw_line({start_x, start_y}, {end1_x, end1_y}); 
                    g->draw_line({start_x, start_y}, {end2_x, end2_y});  
                    
                    g->set_coordinate_system(ezgl::WORLD);
                    
                } else {
                    int num_curve = street.numCurvePoints;
                    if(beginpt == getIntersectionPosition(street.from)) {
                        endpt = getStreetSegmentCurvePoint(path[i], 0);
                    } else {
                        endpt = getStreetSegmentCurvePoint(path[i], num_curve-1);
                    } 
                    float x1 = x_from_lon(beginpt.longitude());
                    float y1 = y_from_lat(beginpt.latitude());
                    float x2 = x_from_lon(endpt.longitude());
                    float y2 = y_from_lat(endpt.latitude());

                    //find angle
                    double angle;
                    if(x1-x2 == 0) angle = 90 * kDegreeToRadian;
                    else angle = abs(atan((y1-y2)/(x1-x2)));
                    angle = angle -45 * kDegreeToRadian;
                    
                    ezgl::rectangle box = ezgl::rectangle({x1, y1}, {x2, y2});
                    ezgl::rectangle screen = g->world_to_screen(box);
                    g->set_coordinate_system(ezgl::SCREEN);
                    double start_x = (screen.right()+screen.left())/2;
                    double start_y = (screen.top()+screen.bottom())/2;
                    double end1_x, end1_y, end2_x, end2_y;
                    
                    if(angle >= 0) {
                    if(x2-x1 >= 0 && y2-y1 >= 0) { //1
                    end1_x = start_x - 5*sqrt(2)*cos(angle);
                    end1_y = start_y + 5*sqrt(2)*sin(angle);
                    end2_x = start_x + 5*sqrt(2)*sin(angle);
                    end2_y = start_y + 5*sqrt(2)*cos(angle);
                    } else if(x2-x1 < 0 && y2 -y1 > 0) { //2
                        end1_x = start_x + 5*sqrt(2)*cos(angle);
                        end1_y = start_y + 5*sqrt(2)*sin(angle);
                        end2_x = start_x - 5*sqrt(2)*sin(angle);
                        end2_y = start_y + 5*sqrt(2)*cos(angle);
                    } else if(x2 -x1 < 0 && y2 - y1 < 0) { //3
                        end1_x = start_x + 5*sqrt(2)*cos(angle);
                        end1_y = start_y - 5*sqrt(2)*sin(angle);
                        end2_x = start_x - 5*sqrt(2)*sin(angle);
                        end2_y = start_y - 5*sqrt(2)*cos(angle);
                    } else { //4
                        end1_x = start_x - 5*sqrt(2)*cos(angle);
                        end1_y = start_y - 5*sqrt(2)*sin(angle);
                        end2_x = start_x + 5*sqrt(2)*sin(angle);
                        end2_y = start_y - 5*sqrt(2)*cos(angle);
                    }
                    } else {
                        double angle1 = -angle;
                       if(x2-x1 >= 0 && y2-y1 >= 0) { //1
                        end1_x = start_x - 5*sqrt(2)*sin(angle1);
                        end1_y = start_y + 5*sqrt(2)*cos(angle1);
                        end2_x = start_x - 5*sqrt(2)*cos(angle1);
                        end2_y = start_y - 5*sqrt(2)*sin(angle1);
                    } else if(x2-x1 < 0 && y2 -y1 > 0) { //2
                        end1_x = start_x + 5*sqrt(2)*cos(angle);
                        end1_y = start_y + 5*sqrt(2)*sin(angle);
                        end2_x = start_x - 5*sqrt(2)*sin(angle);
                        end2_y = start_y + 5*sqrt(2)*cos(angle);
                    } else if(x2 -x1 < 0 && y2 - y1 < 0) { //3
                        end1_x = start_x + 5*sqrt(2)*sin(angle1);
                        end1_y = start_y - 5*sqrt(2)*cos(angle1);
                        end2_x = start_x + 5*sqrt(2)*cos(angle1);
                        end2_y = start_y + 5*sqrt(2)*sin(angle1);
                    } else { //4
                        end1_x = start_x - 5*sqrt(2)*cos(angle);
                        end1_y = start_y - 5*sqrt(2)*sin(angle);
                        end2_x = start_x + 5*sqrt(2)*sin(angle);
                        end2_y = start_y - 5*sqrt(2)*cos(angle);
                    } 
                    }
                    
                    g->set_coordinate_system(ezgl::SCREEN);
                    g->draw_line({start_x, start_y}, {end1_x, end1_y}); 
                    g->draw_line({start_x, start_y}, {end2_x, end2_y});  
                    
                    g->set_coordinate_system(ezgl::WORLD);
                    
                     
                }
            }
            }
        }   
    }
                    
    //draw subway stations && subwaylines 
    if (subway_bool){
        std::cout << "[DEBUG] draw subway stations & lines" << std::endl;
        for(int line_id = 0;line_id < subway_stations.size();++line_id) {
            if(subway_stations[line_id].size() == 0) continue;
            LatLon station_position = (subway_stations[line_id])[0].second;
            float x1 = x_from_lon(station_position.longitude());
            float y1 = y_from_lat(station_position.latitude());
            for(int station_id = 1;station_id <= subway_stations[line_id].size()/2;station_id++) {
               station_position = (subway_stations[line_id])[station_id].second;
               float x2 = x_from_lon(station_position.longitude());
               float y2 = y_from_lat(station_position.latitude());
               g->set_color(ezgl::RED);
               g->draw_line({x1,y1}, {x2,y2});
               if(g->get_visible_world().width() < 20000) g->draw_surface(subway_symbol, {x1, y1});
               x1 = x2;
               y1 = y2;
            }
        }
    }
    
    //draw developers' home
    std::cout << "[DEBUG] draw Home " <<std::endl ;
    auto house = ezgl::renderer::load_png("libstreetmap/resources/government-line.png");
    g->draw_surface(house, {-6.39071e+06,4.85947e+06});
    g->draw_surface(house, {-6.39019e+06,4.85651e+06});    
    g->draw_surface(house, {9.93489e+06,4.44407e+06});
    ezgl::renderer::free_surface(house);
    
    
    //Draw POI as user needed 
    std::cout << "[DEBUG] draw POI " <<std::endl ;
    OSMnode * poidata = new OSMnode; 
    std::string zoomnotify = "Please ZOOM IN for detailed POI display." ; 
    if (g->get_visible_world().width()<6000 && POIstatus["Restaurant"]){
        std::cout << "[DEBUG]\t drawing fast_food" << std::endl;
        ezgl::surface *fast_food_png = ezgl::renderer::load_png("libstreetmap/resources/restaurant-line.png");
        for(auto & poipoi : poidata->nodess["fast_food"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(fast_food_png, {x, y});
        }
        ezgl::renderer::free_surface(fast_food_png);
    } else if (POIstatus["Restaurant"]){
        GObject * bar = applic->get_object("StatusBar");
        gtk_statusbar_pop((GtkStatusbar*)bar, 0);
        gtk_statusbar_push((GtkStatusbar*)bar, 0, zoomnotify.c_str());
    }

    if (g->get_visible_world().width()<5000 && POIstatus["Hospital"]){
        std::cout << "[DEBUG]\t drawing pharmacy" << std::endl;
        ezgl::surface *hospital_png = ezgl::renderer::load_png("libstreetmap/resources/hospital-line.png");
        for(auto & poipoi : poidata->nodess["pharmacy"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(hospital_png, {x, y});
        }
        for(auto & poipoi : poidata->nodess["clinic"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(hospital_png, {x, y});
        }
        for(auto & poipoi : poidata->nodess["hospital"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(hospital_png, {x, y});
        }
        ezgl::renderer::free_surface(hospital_png);
    } else if (POIstatus["Hospital"]){
        GObject * bar = applic->get_object("StatusBar");
        gtk_statusbar_pop((GtkStatusbar*)bar, 0);
        gtk_statusbar_push((GtkStatusbar*)bar, 0, zoomnotify.c_str());
    }
    
    if (g->get_visible_world().width()<8000 && POIstatus["School"]){
        std::cout << "[DEBUG]\t drawing school" << std::endl;
        ezgl::surface *school_png = ezgl::renderer::load_png("libstreetmap/resources/book-open-line.png");
        for(auto & poipoi : poidata->nodess["school"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(school_png, {x, y});
        }
        for(auto & poipoi : poidata->nodess["college"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(school_png, {x, y});
        }
        for(auto & poipoi : poidata->nodess["university"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(school_png, {x, y});
        }
        ezgl::renderer::free_surface(school_png);
    } else if (POIstatus["School"]){
        GObject * bar = applic->get_object("StatusBar");
        gtk_statusbar_pop((GtkStatusbar*)bar, 0);
        gtk_statusbar_push((GtkStatusbar*)bar, 0, zoomnotify.c_str());
    }
    
    if (g->get_visible_world().width()<8000 && POIstatus["Bank"]){
        std::cout << "[DEBUG]\t drawing bank" << std::endl;
        ezgl::surface *bank_png = ezgl::renderer::load_png("libstreetmap/resources/bank-line.png");
        for(auto & poipoi : poidata->nodess["bank"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(bank_png, {x, y});
        }
        ezgl::renderer::free_surface(bank_png);
    } else if (POIstatus["Bank"]){
        GObject * bar = applic->get_object("StatusBar");
        gtk_statusbar_pop((GtkStatusbar*)bar, 0);
        gtk_statusbar_push((GtkStatusbar*)bar, 0, zoomnotify.c_str());
    }
    
    if (g->get_visible_world().width()<8000 && POIstatus["Cup"]){
        std::cout << "[DEBUG]\t drawing cafe" << std::endl;
        ezgl::surface *cup_png = ezgl::renderer::load_png("libstreetmap/resources/cup-line.png");
        for(auto & poipoi : poidata->nodess["cafe"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(cup_png, {x, y});
        }
        ezgl::renderer::free_surface(cup_png);
    } else if (POIstatus["Cup"]){
        GObject * bar = applic->get_object("StatusBar");
        gtk_statusbar_pop((GtkStatusbar*)bar, 0);
        gtk_statusbar_push((GtkStatusbar*)bar, 0, zoomnotify.c_str());
    }
    
    if (g->get_visible_world().width()<10000 && POIstatus["Bar"]){
        std::cout << "[DEBUG]\t drawing bank" << std::endl;
        ezgl::surface *bank_png = ezgl::renderer::load_png("libstreetmap/resources/bar-line.png");
        for(auto & poipoi : poidata->nodess["bar"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(bank_png, {x, y});
        }
        ezgl::renderer::free_surface(bank_png);
    } else if (POIstatus["Bar"]){
        GObject * bar = applic->get_object("StatusBar");
        gtk_statusbar_pop((GtkStatusbar*)bar, 0);
        gtk_statusbar_push((GtkStatusbar*)bar, 0, zoomnotify.c_str());
    }
    if (POIstatus["Movie"]){
        std::cout << "[DEBUG]\t drawing bank" << std::endl;
        ezgl::surface *movie_png = ezgl::renderer::load_png("libstreetmap/resources/movie-line.png");
        for(auto & poipoi : poidata->nodess["cinema"]){
            auto poilocation = getPOIPosition(poipoi.second);
            float x = poilocation.longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = poilocation.latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(movie_png, {x, y});
        }
        ezgl::renderer::free_surface(movie_png);
    }
    if (POImode){
        
        for (int i = 0 ; i < getNumPointsOfInterest() ; i+= 5){
            if (poi_light[i]){
                LatLon res = getPOIPosition(i);
                double x = x_from_lon(res.longitude());
                double y = y_from_lat(res.latitude());
                g->draw_surface(red_pin, {x,y});
            }
        }
    }
    
    //inserting the weather info
    std::string readBuffer;
    CURL *curl = curl_easy_init();
    if(curl) {
        g->set_color(ezgl::color(128,141,184),100);
        g->fill_rectangle({g->get_visible_world().left(),g->get_visible_world().top()},{g->get_visible_world().left()+0.16 * g->get_visible_world().width() , g->get_visible_world().top()-0.06 * g->get_visible_world().height()});
        try{                                    // in case network not stable we can show network error
            std::string api = "http://api.weatherapi.com/v1/current.json?key=dcbcea1ec69d497a8be71231211103&q=";   //weather query api 
            api += current_city.substr(0, current_city.find('_'));
            api += "&aqi=no";
            std::cout <<"[DEBUG]Getting weather info from API: " << api <<std::endl; 
            curl_easy_setopt(curl, CURLOPT_URL, api.c_str());
            readBuffer.clear();
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer); 
            curl_easy_perform(curl);
            rapidjson::Document document;
            document.Parse(readBuffer.c_str());
            std::string icon_str = "http:";
            icon_str += document["current"]["condition"]["icon"].GetString();   // URL to the image, which will be download later
            std::string cmd = "wget ";
            cmd += icon_str;
            system("rm *.png");                                                 //remove the png from last call 
            system(cmd.c_str());                                                //use a syscall to download pictures. 
            std::string filename = icon_str.substr(icon_str.rfind('/')+1,icon_str.length());
            ezgl::surface *weather_png = ezgl::renderer::load_png(filename.c_str());// draw the downloaded png 
            g->draw_surface(weather_png, {g->get_visible_world().left() , g->get_visible_world().top()});
            ezgl::renderer::free_surface(weather_png);
            std::string strr = document["current"]["condition"]["text"].GetString(); // draw the weather text 
            std::cout << strr.length() << std::endl;
            if (strr.length() > 10){                                            // set the size according to the length of the string 
                g->set_font_size(12); 
            }
            else {
                g->set_font_size(18);
            }
            g->set_color(ezgl::BLACK);
            g->set_text_rotation(0);
            g->draw_text({g->get_visible_world().left()+0.08 * g->get_visible_world().width() , g->get_visible_world().top()-0.035 * g->get_visible_world().height()},document["current"]["condition"]["text"].GetString());
            curl_easy_cleanup(curl);
        } catch(...){                                                              // in case networking error 
            g->set_font_size(18);
            g->set_color(ezgl::BLACK);
            g->set_text_rotation(0);
            g->draw_text({g->get_visible_world().left()+0.08 * g->get_visible_world().width() , g->get_visible_world().top()-0.035 * g->get_visible_world().height()},"Network Error");

        }
       
    }
    //draw compass, ruler and corresponding text
    ezgl::surface *ruler = ezgl::renderer::load_png("libstreetmap/resources/ruler.png");
    g->draw_surface(compass, {g->get_visible_world().right()-0.02 * g->get_visible_world().width() , g->get_visible_world().bottom()+0.03*g->get_visible_world().height()});
    g->draw_surface(ruler, {g->get_visible_world().right()-0.1 * g->get_visible_world().width() , g->get_visible_world().bottom()+0.03*g->get_visible_world().height()});
    int ruler_length = g->get_visible_world().width() * 0.04 * 4 / 3; 
    std::string ruler_string = std::to_string(ruler_length);
    ruler_string += " m";
    g->set_font_size(10);
    g->set_color(ezgl::BLACK);
    g->set_text_rotation(0);
    g->draw_text({g->get_visible_world().right()-0.07 * g->get_visible_world().width() , g->get_visible_world().bottom()+0.03*g->get_visible_world().height()},ruler_string.c_str());

    ezgl::renderer::free_surface(ruler);
    ezgl::renderer::free_surface(compass);
    ezgl::renderer::free_surface(red_pin);
    ezgl::renderer::free_surface(poi);
    ezgl::renderer::free_surface(subway_symbol);
    delete poidata;
    
    //draw start and destination of path find
    if (Find_path_mode){
        if (start != -1){
            ezgl::surface * start1 = ezgl::renderer::load_png("libstreetmap/resources/red_flag.png");
            float x = getIntersectionPosition(start).longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = getIntersectionPosition(start).latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(start1, {x, y+(g->get_visible_world().width())/100});
            ezgl::renderer::free_surface(start1);
        }
        if (end != -1){
            ezgl::surface * end1 = ezgl::renderer::load_png("libstreetmap/resources/blue_flag.png");
            float x = getIntersectionPosition(end).longitude() * kDegreeToRadian * cos(avg_lat_of_this_map) * kEarthRadiusInMeters;
            float y = getIntersectionPosition(end).latitude()* kDegreeToRadian * kEarthRadiusInMeters  ;
            g->draw_surface(end1, {x, y+(g->get_visible_world().width())/100});
            ezgl::renderer::free_surface(end1);
        }
    }
}
}

void act_on_mouse_click(ezgl::application* app, GdkEventButton*, double x, double y) {
    std::cout << "[DEBUG]Mouse clicked at (" << x << "," << y << ")\n";
    LatLon pos = LatLon(lat_from_y(y), lon_from_x(x));
    
    if (Intersection_mode){                      // find the nearest intersection 
        int id = findClosestIntersection(pos);
        std::cout << "[DEBUG]Closet intersection: " << intersections[id].name << "\n";
        intersections[id].highlight = !intersections[id].highlight;
        std::string POIinfo = "Intersection Name: ";
        POIinfo += intersections[id].name;
        GObject * bar = applic->get_object("StatusBar");
        gtk_statusbar_pop((GtkStatusbar *)bar , 0);
        gtk_statusbar_push((GtkStatusbar *)bar , 0 , POIinfo.c_str());
    } 
    else if(Find_path_mode) {
        std::cout << "[DEBUG]Find Path Mode" << "\n";
        int id = findClosestIntersection(pos);
        
        if(start==-1&&end==-1) start = id;
        else if(start!=-1&&end==-1) end = id;
        else if(start!=-1&&end!=-1){
            start = id;
            end = -1;
        }
    }else if (POImode){                            // in POI mode, find nearest POI 
        int id = findClosestPOI(pos,"ECE297will_get4.0");
        std::cout << "[DEBUG]Closet POI: " << getPOIName(id) << "\n";
        poi_light[id] = !poi_light[id];
        std::string POIinfo = "POI Name: ";
        POIinfo += getPOIName(id);
        POIinfo += "               POI Type: ";
        POIinfo += getPOIType(id);
        GObject * bar = applic->get_object("StatusBar");
        gtk_statusbar_pop((GtkStatusbar *)bar , 0);
        gtk_statusbar_push((GtkStatusbar *)bar , 0 , POIinfo.c_str());
    }
    
    app->refresh_drawing();
}

void drawMap(){
    int x_offset = 0, y_offset = 0 ;          // derfault offests to make the map looks in the center of the application
    if (current_city == "toronto_canada"){
        x_offset = 10000;
        y_offset = 0; 
    } else if (current_city == "beijing_china"){
     x_offset = 20000;
     y_offset = 0;
    } else if (current_city == "cairo_egypt"){
     x_offset = 5000;
     y_offset = 0;
    } else if (current_city == "cape-town_south-africa"){
     x_offset = 20000;
     y_offset = 25000;
    } else if (current_city == "golden-horseshoe_canada"){
     x_offset = 30000;
     y_offset = 0;
    } else if (current_city == "hamilton_canada"){
     x_offset = 5000;
     y_offset = 0;
    } else if (current_city == "hong-kong_china"){
     x_offset = 60000;
     y_offset = 60000;
    } else if (current_city == "iceland"){
     x_offset = 25000;
     y_offset = 25000;
    } else if (current_city == "interlaken_switzerland"){
     x_offset = 2000;
     y_offset = 1500;
    } else if (current_city == "london_england"){
     x_offset = 0;
     y_offset = 20000;
    } else if (current_city == "moscow_russia"){
     x_offset = 20000;
     y_offset = 5000;
    } else if (current_city == "new-delhi_india"){
     x_offset = 20000;
     y_offset = 0;
    } else if (current_city == "new-york_usa"){
     x_offset = 20000;
     y_offset = 6000;
    } else if (current_city == "rio-de-janeiro_brazil"){
     x_offset = 0;
     y_offset = 0;
    } else if (current_city == "saint-helena"){
     x_offset = 0;
     y_offset = 0;
    } else if (current_city == "singapore"){
     x_offset = 0;
     y_offset = 6000;
    } else if (current_city == "sydney_australia"){
     x_offset = 0;
     y_offset = 6000;
    } else if (current_city == "tehran_iran"){
     x_offset = 40000;
     y_offset = 25000;
    } else if (current_city == "tokyo_japan"){
     x_offset = 0;
     y_offset = 0;
    }
    if (first_draw){
        ezgl::application::settings settings;

        // Path to the "main.ui" file that contains an XML description of the UI.
        settings.main_ui_resource = "./libstreetmap/resources/main.ui";

        // Note: the "main.ui" file has a GtkWindow called "MainWindow".
        settings.window_identifier = "MainWindow";

        // Note: the "main.ui" file has a GtkDrawingArea called "MainCanvas".
        settings.canvas_identifier = "MainCanvas";

        // Create our EZGL application.
        ezgl::application application(settings);                                                                                                                                                                                 
        set_min_max_long_lat();
        ezgl::rectangle initial_world({minlong + x_offset ,minlat + y_offset},{maxlong+x_offset,maxlat+y_offset}); 
        std::cout <<  minlong << " " <<  minlat <<" " << maxlong <<" " << maxlat<<std::endl ;
        // set background in night mode 
        application.add_canvas("MainCanvas", draw_main_canvas, initial_world, ezgl::color(232, 232, 232, 10));
        applic = &application; 
        application.run(0, act_on_mouse_click, 0, 0);
    } else {
        
        set_min_max_long_lat();
        ezgl::rectangle changing_w({minlong + x_offset ,minlat + y_offset},{maxlong+x_offset,maxlat+y_offset}); 
        applic->change_canvas_world_coordinates("MainCanvas",changing_w);
        applic->refresh_drawing(); 
    }
   }
    


 