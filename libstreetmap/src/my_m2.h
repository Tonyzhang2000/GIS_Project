#pragma once 
#include "m2.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"
//constexpr double kDegreeToRadian = 0.017453292519943295769236907684886;
//constexpr double kEarthRadiusInMeters = 6372797.560856;
void set_min_max_long_lat ();
void draw_main_canvas (ezgl::renderer *g);
void act_on_mouse_click(ezgl::application* app, GdkEventButton* event, double x, double y);
double lat_from_y(double y);
double lon_from_x(double x);
double y_from_lat(double lat);
double x_from_lon(double lon);
struct intersection_data {
    LatLon position;
    std::string name;
    bool highlight = false;
    bool result = false;
};
void show_message (GtkWindow *parent,const gchar *title , const gchar *message);