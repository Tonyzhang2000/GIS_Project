/*
 * Copyright 2021 University of Toronto
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors: Mario Badr, Sameh Attia and Tanner Young-Schultz
 */
#include <cstdlib>
#include "drawroad.h"
#include "ezgl/callback.hpp"
#include "global.h"
#include "my_m2.h"
#include "m1.h"

std::string inputSt2, inputSt1;
StreetIdx   sel1, sel2;

namespace ezgl {

// File wide static variables to track whether the middle mouse
// button is currently pressed AND the old x and y positions of the mouse pointer
bool middle_mouse_button_pressed = false;
int last_panning_event_time = 0;
double prev_x = 0, prev_y = 0;

gboolean press_key(GtkWidget *, GdkEventKey *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  // Call the user-defined key press callback if defined
  if(application->key_press_callback != nullptr) {
    // see: https://developer.gnome.org/gdk3/stable/gdk3-Keyboard-Handling.html
    application->key_press_callback(application, event, gdk_keyval_name(event->keyval));
  }

  // Returning FALSE to indicate this event should be propagated on to other
  // gtk widgets. This is important since we're grabbing keyboard events
  // for the whole main window. It can have unexpected effects though, such
  // as Enter/Space being treated as press any highlighted button.
  // return TRUE (event consumed) if you want to avoid that, and don't have
  // any widgets that need keyboard events.
  return FALSE;
}

gboolean press_mouse(GtkWidget *, GdkEventButton *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  if(event->type == GDK_BUTTON_PRESS) {

    // Check for Middle mouse press to support dragging
    if(event->button == 2) {
      middle_mouse_button_pressed = true;
      prev_x = event->x;
      prev_y = event->y;
    }

    // Call the user-defined mouse press callback if defined
    if(application->mouse_press_callback != nullptr) {
      ezgl::point2d const widget_coordinates(event->x, event->y);

      std::string main_canvas_id = application->get_main_canvas_id();
      ezgl::canvas *canvas = application->get_canvas(main_canvas_id);

      ezgl::point2d const world = canvas->get_camera().widget_to_world(widget_coordinates);
      application->mouse_press_callback(application, event, world.x, world.y);
    }
  }

  return TRUE; // consume the event
}

gboolean release_mouse(GtkWidget *, GdkEventButton *event, gpointer )
{
  if(event->type == GDK_BUTTON_RELEASE) {
    // Check for Middle mouse release to support dragging
    if(event->button == 2) {
      middle_mouse_button_pressed = false;
    }
  }

  return TRUE; // consume the event
}

gboolean move_mouse(GtkWidget *, GdkEventButton *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  if(event->type == GDK_MOTION_NOTIFY) {

    // Check if the middle mouse is pressed to support dragging
    if(middle_mouse_button_pressed) {
      // drop this panning event if we have just served another one
      if(gtk_get_current_event_time() - last_panning_event_time < 100)
        return true;

      last_panning_event_time = gtk_get_current_event_time();

      GdkEventMotion *motion_event = (GdkEventMotion *)event;

      std::string main_canvas_id = application->get_main_canvas_id();
      auto canvas = application->get_canvas(main_canvas_id);

      point2d curr_trans = canvas->get_camera().widget_to_world({motion_event->x, motion_event->y});
      point2d prev_trans = canvas->get_camera().widget_to_world({prev_x, prev_y});

      double dx = curr_trans.x - prev_trans.x;
      double dy = curr_trans.y - prev_trans.y;

      prev_x = motion_event->x;
      prev_y = motion_event->y;

      // Flip the delta x to avoid inverted dragging
      translate(canvas, -dx, -dy);
    }
    // Else call the user-defined mouse move callback if defined
    else if(application->mouse_move_callback != nullptr) {
      ezgl::point2d const widget_coordinates(event->x, event->y);

      std::string main_canvas_id = application->get_main_canvas_id();
      ezgl::canvas *canvas = application->get_canvas(main_canvas_id);

      ezgl::point2d const world = canvas->get_camera().widget_to_world(widget_coordinates);
      application->mouse_move_callback(application, event, world.x, world.y);
    }
  }

  return TRUE; // consume the event
}

gboolean scroll_mouse(GtkWidget *, GdkEvent *event, gpointer data)
{

  if(event->type == GDK_SCROLL) {
    auto application = static_cast<ezgl::application *>(data);

    std::string main_canvas_id = application->get_main_canvas_id();
    auto canvas = application->get_canvas(main_canvas_id);

    GdkEventScroll *scroll_event = (GdkEventScroll *)event;

    ezgl::point2d scroll_point(scroll_event->x, scroll_event->y);

    if(scroll_event->direction == GDK_SCROLL_UP) {
      // Zoom in at the scroll point
      ezgl::zoom_in(canvas, scroll_point, 5.0 / 3.0);
    } else if(scroll_event->direction == GDK_SCROLL_DOWN) {
      // Zoom out at the scroll point
      ezgl::zoom_out(canvas, scroll_point, 5.0 / 3.0);
    } else if(scroll_event->direction == GDK_SCROLL_SMOOTH) {
      // Doesn't seem to be happening
    } // NOTE: We ignore scroll GDK_SCROLL_LEFT and GDK_SCROLL_RIGHT
  }
  return TRUE;
}

gboolean press_zoom_fit(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::zoom_fit(canvas, canvas->get_camera().get_initial_world());

  return TRUE;
}

gboolean press_zoom_in(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::zoom_in(canvas, 5.0 / 3.0);

  return TRUE;
}

gboolean press_zoom_out(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::zoom_out(canvas, 5.0 / 3.0);

  return TRUE;
}

gboolean press_up(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_up(canvas, 5.0);

  return TRUE;
}

gboolean press_down(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_down(canvas, 5.0);

  return TRUE;
}

gboolean press_left(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_left(canvas, 5.0);

  return TRUE;
}

gboolean press_right(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_right(canvas, 5.0);

  return TRUE;
}

gboolean press_proceed(GtkWidget *, gpointer data)
{
  auto ezgl_app = static_cast<ezgl::application *>(data);
  ezgl_app->quit();

  return TRUE;
}



gboolean press_loadMap(GtkWidget* , gpointer){
    /*std::cout << "[DEBUG] chooseMap successfully called." <<std::endl;
    GtkWidget *dia;                             
    ezgl::application* app= static_cast<ezgl::application *>(data);                                                                                                   
    dia = gtk_file_chooser_dialog_new("Choose Map",(GtkWindow*)app->get_object("MainWindow"),GTK_FILE_CHOOSER_ACTION_OPEN,"Cancel",GTK_RESPONSE_CANCEL,                                                                                    
                                    ("Open"),GTK_RESPONSE_ACCEPT,NULL);                                                                                                  
    auto res = gtk_dialog_run(GTK_DIALOG(dia));
    if (res == GTK_RESPONSE_ACCEPT){
      char *filename;
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (dia);
      filename = gtk_file_chooser_get_filename (chooser);
      std::cout << "[DEBUG] get file " << filename <<std::endl;
      g_free (filename);
    } else {
      ;
    }
    gtk_widget_destroy(dia);*/
    std::string binary = "/usr/lib/chromium/chromium ";
    std::string url = "\"https://www.guozhen.dev/ECE297\"";
    system((binary + url).c_str());
    return TRUE; 
}

gboolean on_mapList_changed(GtkEntry *widget, gpointer ){
    std::string res;
    res = gtk_entry_get_text((widget));
    if (res == "Beijing" ) {res = "beijing_china";}
    if (res == "Cario") {res = "cairo_egypt";}
    if (res == "Cape Town") {res = "cape-town_south-africa";}
    if (res == "Golden Horseshoe") {res = "golden-horseshoe_canada";}
    if (res == "Hamilton" ) {res = "hamilton_canada";}
    if (res == "Hong Kong" ) {res = "hong-kong_china";}
    if (res == "Iceland" ) {res = "iceland";}
    if (res == "Interlaken" ) {res = "interlaken_switzerland";}
    if (res == "London" ) {res = "london_england";}
    if (res == "Moscow" ) {res = "moscow_russia";}
    if (res == "New Delhi" ) {res = "new-delhi_india";}
    if (res == "New York" ) {res = "new-york_usa";}
    if (res == "Rio de Janeiro" ) {res = "rio-de-janeiro_brazil";}
    if (res == "Saint Helena" ) {res = "saint-helena";}
    if (res == "Singapore" ) {res = "singapore";}
    if (res == "Sydney" ) {res = "sydney_australia";}
    if (res == "Tehran" ) {res = "tehran_iran";}
    if (res == "Tokyo" ) {res = "tokyo_japan";}
    if (res == "Toronto" ) {res = "toronto_canada";}
    current_city = res ;
    std::cout << "[DEBUG] Get input city: " << res << std::endl;
    std::string map_path = "/nfs/ug/fast1/ece297s/public/maps/";
    map_path += res;
    map_path += ".streets.bin";
    closeMap();
    loadMap(map_path);
    drawMap();


    return TRUE; 
}
gboolean on_POI_changed(GtkWidget *widget, gpointer ){
    std::string res;
    res = gtk_button_get_label((GtkButton *)widget);
    std::cout << "[DEBUG] Get input POI: " << res << std::endl;
    //draw individual flags for each poi
    //flag global
    POIstatus[res] = !POIstatus[res];
   
    applic->refresh_drawing();
    return TRUE; 
}

void remove_an_entry(GtkWidget *widget, gpointer data){
    if (widget != data && widget!=NULL)
        gtk_widget_destroy(widget);
    return;
}

gboolean on_first_changed(GtkWidget *widget, gpointer data)
{
    std::cout << data <<std::endl ;
    if (data){
        std::cout <<"inputbox getdata";
    }
  ezgl::application* app = static_cast<ezgl::application *>(data);
  GObject * interlist = app->get_object("input1ListBox"); 
  gtk_container_foreach ((GtkContainer *)interlist, GtkCallback(remove_an_entry), NULL);


  inputSt1 = gtk_entry_get_text ((GtkEntry *)widget);
  if (inputSt1.length() < 4 ) return TRUE;
  std::cout << "[DEBUG] Get and Matching First street name " << inputSt1 << std::endl;
  std::vector<StreetIdx> result = findStreetIdsFromPartialStreetName(inputSt1);
  if (result.empty()) {
      GtkWidget * testInsert= gtk_widget_new (GTK_TYPE_LABEL, "label", "Please enter correct street name" ,"xalign", 0.0, NULL);
      gtk_list_box_insert((GtkListBox *)(interlist),testInsert,-1);
      gtk_widget_show_all ((GtkWidget *) testInsert);
      return TRUE;
  }
  for (auto it = result.begin() ; it != result.end() ; it++){    
        GtkWidget * testInsert= gtk_widget_new (GTK_TYPE_LABEL, "label", getStreetName(*it).c_str() ,"xalign", 0.0, NULL);
        gtk_list_box_insert((GtkListBox *)(interlist),testInsert,-1);
        gtk_widget_show_all ((GtkWidget *) testInsert);
  }
  std::cout << data <<std::endl ;
  return TRUE;
}

gboolean on_second_changed(GtkWidget *widget, gpointer data)
{
    std::cout << data <<std::endl ;
  ezgl::application* app = static_cast<ezgl::application *>(data);
  GObject * interlist = app->get_object("input2ListBox"); 
  gtk_container_foreach ((GtkContainer *)interlist, GtkCallback(remove_an_entry), NULL);

  inputSt2 = gtk_entry_get_text ((GtkEntry *)widget);
  if (inputSt2.length() < 4 ) return TRUE;
  std::cout << "[DEBUG] Get and Matching Second street name " << inputSt2 << std::endl;
  std::vector<StreetIdx> result = findStreetIdsFromPartialStreetName(inputSt2);
  if (result.empty()) {
      GtkWidget * testInsert= gtk_widget_new (GTK_TYPE_LABEL, "label", "Please enter correct street name" ,"xalign", 0.0, NULL);
      gtk_list_box_insert((GtkListBox *)(interlist),testInsert,-1);
      gtk_widget_show_all ((GtkWidget *) testInsert);
      return TRUE;
  }
  for (auto it = result.begin() ; it != result.end() ; it++){    
        GtkWidget * testInsert= gtk_widget_new (GTK_TYPE_LABEL, "label", getStreetName(*it).c_str() ,"xalign", 0.0, NULL);
        gtk_list_box_insert((GtkListBox *)(interlist),testInsert,-1);
        gtk_widget_show_all ((GtkWidget *)  testInsert);
  }

  return TRUE;
}

gboolean start_search(GtkWidget *, gpointer data)
{
  ezgl::application* app = static_cast<ezgl::application *>(applic);
  GObject * interlist = app->get_object("intersectionList"); 
  gtk_container_foreach ((GtkContainer *)interlist, GtkCallback(remove_an_entry), data);
  std::cout << "[DEBUG] Search First street Idx " << sel1 << std::endl;
  std::cout << "[DEBUG] Search Second street Idx " << sel2 << std::endl;
  if (sel1==sel2){
      GtkWidget * testInsert= gtk_widget_new (GTK_TYPE_LABEL, "label", "Please enter two different streets" ,"xalign", 0.0, NULL);
      gtk_list_box_insert((GtkListBox *)(interlist),testInsert,-1);
      gtk_widget_show_all ((GtkWidget *)  testInsert);
  } else{
      std::vector<IntersectionIdx> result = findIntersectionsOfTwoStreets(std::make_pair(sel1,sel2));
      if (result.empty()){
            GtkWidget * testInsert= gtk_widget_new (GTK_TYPE_LABEL, "label", "No intersetions" ,"xalign", 0.0, NULL);
            gtk_list_box_insert((GtkListBox *)(interlist),testInsert,-1);
            gtk_widget_show_all ((GtkWidget *)  testInsert);
       } else{
            for(auto &&item : result){
                intersections[item].result = true;
                GtkWidget * testInsert= gtk_widget_new (GTK_TYPE_LABEL, "label", getIntersectionName(item).c_str() ,"xalign", 0.0, NULL);
                gtk_list_box_insert((GtkListBox *)(interlist),testInsert,-1);
                gtk_widget_show_all ((GtkWidget *)  testInsert);
            }
       }    
  }
  return TRUE;
}

gboolean night_mode(GtkWidget *, gpointer ){
    night_bool = !night_bool;
    applic->refresh_drawing();
    return TRUE;
}

gboolean subway_display(GtkWidget *, gpointer ){
    subway_bool = !subway_bool;
    applic->refresh_drawing();
    return TRUE;
}

gboolean on_select_selSearchbox1(GtkWidget *widget, gpointer data){
    std::cout << data <<std::endl ;
    if (data==NULL){
     // auto currwindow = applic->get_object("MainWindow");
      //show_message((GtkWindow*)currwindow,(gchar*) "Error", (gchar*)"Please Re-enter Street 1");
      return TRUE;
    }
    gint seled_label = gtk_list_box_row_get_index(gtk_list_box_get_selected_row ((GtkListBox *)widget));
    std::vector<StreetIdx> result = findStreetIdsFromPartialStreetName(inputSt1);
    std::cout << "[DEBUG] box1 choose " << getStreetName(result[seled_label]) <<" StIdx = " << result[seled_label]<< std::endl;
    std::string name = getStreetName(result[seled_label]);
    sel1 = result[seled_label];
    ezgl::application* app = static_cast<ezgl::application *>(applic);
    GObject * interlist = app->get_object("input1ListBox"); 
    gtk_container_foreach ((GtkContainer *)interlist, GtkCallback(remove_an_entry), data);
    
    return TRUE;
}
gboolean on_select_selSearchbox2(GtkWidget *widget, gpointer data){
    std::cout << data <<std::endl ;
    if (data==NULL){
      //auto currwindow = applic->get_object("MainWindow");
      //show_message((GtkWindow*)currwindow,(gchar*) "Error", (gchar*)"Please Re-enter Street 2");
      return TRUE;
    }
    gint seled_label = gtk_list_box_row_get_index(gtk_list_box_get_selected_row ((GtkListBox *)widget));
    std::vector<StreetIdx> result = findStreetIdsFromPartialStreetName(inputSt2);
    std::cout << "[DEBUG] box2 choose " << getStreetName(result[seled_label]) <<" StIdx = " << result[seled_label]<< std::endl;
    std::string name = getStreetName(result[seled_label]);
    sel2 = result[seled_label];
    ezgl::application* app = static_cast<ezgl::application *>(applic);
    GObject * interlist = app->get_object("input2ListBox"); 
    gtk_container_foreach ((GtkContainer *)interlist, GtkCallback(remove_an_entry), data);

    return TRUE;
}
gboolean change_mode(GtkWidget * widget, gpointer){
    std::cout << "[DEBUG]Click mode changed" << std::endl;
    std::string res;
    res = gtk_entry_get_text((GtkEntry *)widget);
    if (res == "POI"){
        POImode = true;
        Find_path_mode = false;
        Intersection_mode = false;
        
    }else if (res == "Intersection"){
        Intersection_mode = true;
        Find_path_mode = false;
        POImode = false;
        
    }else {
        Find_path_mode = true;
        POImode = false;
        Intersection_mode = false;
        
    }
    return TRUE; 
}
gboolean clean_ui_mode(GtkWidget *, gpointer){
  for (auto &element : intersections){
    element.result = element.highlight = false;
  }
  start = -1;
  end = -1;
  applic->refresh_drawing();
  return TRUE;
}

gboolean on_select_intersection(GtkWidget * widget, gpointer data){
    std::cout << data <<std::endl ;
    gint seled_label = gtk_list_box_row_get_index(gtk_list_box_get_selected_row ((GtkListBox *)widget));
    std::vector<IntersectionIdx> result = findIntersectionsOfTwoStreets(std::make_pair(sel1,sel2));
    if (result.size() == 0 ) return TRUE;
    std::cout << "[DEBUG] Intersection choose " << getIntersectionName(result[seled_label]) <<" IntersectionIdx = " << result[seled_label]<< std::endl;
    std::string name = getIntersectionName(result[seled_label]);
    if(start==-1&&end==-1) start = result[seled_label];
    else if(start!=-1&&end==-1) end = result[seled_label];
    else if(start!=-1&&end!=-1){
        start = seled_label;
        end = -1;
    }
    ezgl::application* app = static_cast<ezgl::application *>(applic);
    GObject * interlist = app->get_object("intersectionList"); 
    gtk_container_foreach ((GtkContainer *)interlist, GtkCallback(remove_an_entry), data);
    applic->refresh_drawing();
    return TRUE;
}
}