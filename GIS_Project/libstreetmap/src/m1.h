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
#pragma once //protects against multiple inclusions of this header file

#include <string>
#include <vector>
#include <utility>

#include "StreetsDatabaseAPI.h"

class LatLon; //Forward declaration
class OSMID; //Forward declaration

// Use these values if you need the earth's radius or to convert from degrees 
// to radians
constexpr double kEarthRadiusInMeters = 6372797.560856;
constexpr double kDegreeToRadian = 0.017453292519943295769236907684886;


// Different functions have different speed requirements. 
//
// High: this function must be very fast, and is likely to require creation of 
// data structures that allow you to rapidly return the right data rather than 
// always going to the StreetsDatabaseAPI
//
// Moderate: this function is speed tested, but a more straightforward 
// implementation (e.g. just calling the proper streetsDatabaseAPI functions) 
// should pass the speed tests.
//
// None: this function is not speed tested.


// Loads a map streets.bin file. Returns true if successful, false if some error
// occurs and the map can't be loaded.
// Speed Requirement --> moderate
bool loadMap(std::string map_streets_database_filename);

// Close the map (if loaded)
// Speed Requirement --> moderate
void closeMap();

// Returns the distance between two (lattitude,longitude) coordinates in meters
// Speed Requirement --> moderate
double findDistanceBetweenTwoPoints(std::pair<LatLon, LatLon> points);

// Returns the length of the given street segment in meters
// Speed Requirement --> moderate
double findStreetSegmentLength(StreetSegmentIdx street_segment_id);

// Returns the travel time to drive from one end of a street segment 
// to the other, in seconds, when driving at the speed limit
// Note: (time = distance/speed_limit)
// Speed Requirement --> high 
double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id);

// Returns the nearest intersection to the given position
// Speed Requirement --> none
IntersectionIdx findClosestIntersection(LatLon my_position);

// Returns the street segments that connect to the given intersection 
// Speed Requirement --> high
std::vector<StreetSegmentIdx> findStreetSegmentsOfIntersection(IntersectionIdx intersection_id);

// Returns the street names at the given intersection (includes duplicate 
// street names in the returned vector)
// Speed Requirement --> high 
std::vector<std::string> findStreetNamesOfIntersection(IntersectionIdx intersection_id);

// Returns all intersections reachable by traveling down one street segment 
// from the given intersection (hint: you can't travel the wrong way on a 
// 1-way street)
// the returned vector should NOT contain duplicate intersections
// Speed Requirement --> high 
std::vector<IntersectionIdx> findAdjacentIntersections(IntersectionIdx intersection_id);

// Returns all intersections along the a given street.
// There should be no duplicate intersections in the returned vector.
// Speed Requirement --> high
std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id);

// Return all intersection ids at which the two given streets intersect
// This function will typically return one intersection id for streets
// that intersect and a length 0 vector for streets that do not. For unusual 
// curved streets it is possible to have more than one intersection at which 
// two streets cross.
// There should be no duplicate intersections in the returned vector.
// Speed Requirement --> high
std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(std::pair<StreetIdx, StreetIdx> street_ids);

// Returns all street ids corresponding to street names that start with the 
// given prefix 
// The function should be case-insensitive to the street prefix. 
// The function should ignore spaces.
//  For example, both "bloor " and "BloOrst" are prefixes to 
// "Bloor Street East".
// If no street names match the given prefix, this routine returns an empty 
// (length 0) vector.
// You can choose what to return if the street prefix passed in is an empty 
// (length 0) string, but your program must not crash if street_prefix is a 
// length 0 string.
// Speed Requirement --> high 
std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix);

// Returns the length of a given street in meters
// Speed Requirement --> high 
double findStreetLength(StreetIdx street_id);

// Return the smallest axis-aligned rectangle that contains all the 
// intersections and curve points of the given street (i.e. the min,max 
// lattitude and longitude bounds that can just contain all points of the 
// street).
// Speed Requirement --> none 
LatLonBounds findStreetBoundingBox(StreetIdx street_id);

// Returns the nearest point of interest of the given name to the given position
// Speed Requirement --> none 
POIIdx findClosestPOI(LatLon my_position, std::string POIname);

// Returns the area of the given closed feature in square meters
// Assume a non self-intersecting polygon (i.e. no holes)
// Return 0 if this feature is not a closed polygon.
// Speed Requirement --> moderate
double findFeatureArea(FeatureIdx feature_id);
