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

#ifndef EZGL_CALLBACK_HPP
#define EZGL_CALLBACK_HPP

#include "ezgl/application.hpp"
#include "ezgl/camera.hpp"
#include "ezgl/canvas.hpp"
#include "ezgl/control.hpp"
#include "ezgl/graphics.hpp"

#include <iostream>

namespace ezgl {

/**** Callback functions for keyboard and mouse input, and for all the ezgl predefined buttons. *****/

/**
 * React to a <a href = "https://developer.gnome.org/gtk3/stable/GtkWidget.html#GtkWidget-key-press-event">keyboard
 * press event</a>.
 *
 * @param widget The GUI widget where this event came from.
 * @param event The keyboard event.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_key(GtkWidget *widget, GdkEventKey *event, gpointer data);

/**
 * React to <a href = "https://developer.gnome.org/gtk3/stable/GtkWidget.html#GtkWidget-button-press-event">mouse click
 * event</a>
 *
 * @param widget The GUI widget where this event came from.
 * @param event The click event.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_mouse(GtkWidget *widget, GdkEventButton *event, gpointer data);

/**
 * React to <a href = "https://developer.gnome.org/gtk3/stable/GtkWidget.html#GtkWidget-button-release-event">mouse release
 * event</a>
 *
 * @param widget The GUI widget where this event came from.
 * @param event The click event.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean release_mouse(GtkWidget *widget, GdkEventButton *event, gpointer data);

/**
 * React to <a href = "https://developer.gnome.org/gtk3/stable/GtkWidget.html#GtkWidget-button-release-event">mouse release
 * event</a>
 *
 * @param widget The GUI widget where this event came from.
 * @param event The click event.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean move_mouse(GtkWidget *widget, GdkEventButton *event, gpointer data);

/**
 * React to <a href = "https://developer.gnome.org/gtk3/stable/GtkWidget.html#GtkWidget-scroll-event"> scroll_event
 * event</a>
 *
 * @param widget The GUI widget where this event came from.
 * @param event The click event.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean scroll_mouse(GtkWidget *widget, GdkEvent *event, gpointer data);

/**
 * React to the clicked zoom_fit button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_zoom_fit(GtkWidget *widget, gpointer data);

/**
 * React to the clicked zoom_in button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */

gboolean press_zoom_in(GtkWidget *widget, gpointer data);

/**
 * React to the clicked zoom_out button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_zoom_out(GtkWidget *widget, gpointer data);

/**
 * React to the clicked up button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_up(GtkWidget *widget, gpointer data);

/**
 * React to the clicked up button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_down(GtkWidget *widget, gpointer data);

/**
 * React to the clicked up button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_left(GtkWidget *widget, gpointer data);

/**
 * React to the clicked up button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_right(GtkWidget *widget, gpointer data);

/**
 * React to the clicked proceed button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_proceed(GtkWidget *widget, gpointer data);


/**
 * React to the clicked Instruction button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean press_loadMap(GtkWidget *widget, gpointer data);

/**
 * React to the change maplist button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean on_mapList_changed(GtkEntry *widget, gpointer data);

/**
 * React to the clicked change click mode menu 
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean on_POI_changed(GtkWidget *widget, gpointer data);

/**
 * React to the clicked subway button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean subway_display(GtkWidget *widget, gpointer data);
/**
 * React to the clicked search button
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean start_search(GtkWidget *, gpointer data);

/**
 * React to the second input block
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean on_second_changed(GtkWidget *widget, gpointer data);
/**
 * React to the first  input block
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean on_first_changed(GtkWidget *widget, gpointer data);
/**
 * React to the first  select block
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean on_select_selSearchbox1(GtkWidget *, gpointer data);
/**
 * React to the second  select block
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean on_select_selSearchbox2(GtkWidget *, gpointer data);
/**
 * internal callback to remove an entry from a list box 
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 */
void remove_an_entry(GtkWidget *widget, gpointer data);
/**
 * React to the night mode checkbox 
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean night_mode(GtkWidget *widgt, gpointer data);
/**
 * React to the POI mode list 
 *
 * @param widget The GUI widget where this event came from.
 * @param data A pointer to any user-specified data you passed in.
 *
 * @return FALSE to allow other handlers to see this event, too. TRUE otherwise.
 */
gboolean change_mode(GtkWidget *widgt, gpointer data);
gboolean clean_ui_mode(GtkWidget *, gpointer);
gboolean on_select_intersection(GtkWidget * widget, gpointer data);
}

#endif //EZGL_CALLBACK_HPP



  