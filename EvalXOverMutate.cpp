/* EvalXOverMutate.cpp
* Implementaion of evaluate(), crossover() and mutate() along with
* helper functions and parallelized them with OpenMP
*
*/


#include <iostream>  // cout
#include <fstream>   // ifstream
#include <string.h>  // strncpy
#include <stdlib.h>  // rand
#include <math.h>    // sqrt, pow
#include <omp.h>     // OpenMP
#include <algorithm> // qsort
#include "Timer.h"
#include "Trip.h"

//array to store the result of distances between city i and j in distances[i][j]
static float distances[CITIES][CITIES] = { 0 };
// array to store the computed distance between origin and city i 
static float distancesOrigin[CITIES] = { 0 };  
// Origin 
static const int origin[] = { 0, 0 };                    

/*
* Calculates the distance between two cities using distance formula
*
* @param cityA[2]: coordinates of the first city
* @param cityB[2]: coordinates of the second city
*
* @return float distance between two cities
*/
inline float fitnessBetweenTwoCities(const int cityA[2], const int cityB[2]) {

    int x_diff = cityB[0] - cityA[0];    // difference in x coordinates
    int y_diff = cityB[1] - cityA[1];    // difference in y coordinates

    return sqrt((x_diff * x_diff) + (y_diff * y_diff));
}


/*
* Retruns the index (int) corresponding to the char of the city
*
* @param city: char whose index is needed
*
* @return int index of the city
*/
inline int getCityIndex(char city) {
    if (city >= 'A' && city <= 'Z') {
        return int(city - 'A');
    }
    else {
        return int(city - '0' + 26);
    }
}


/*
* Retruns the city (char) corresponding to the index
*
* @param cityIndex: int whose char is needed
*
* @return char of the city
*/
inline char getCityFromIndex(int cityIndex) {
    if (cityIndex < 26) {
        return char('A' + cityIndex);
    }
    else {
        return char('0' + cityIndex - 26);
    }
}


/*
* Retriving the stored distance between the two cities
*
* @param cityA: char of the from city
* @param cityB: char of the to city
*
* @return float distance from origin to city
*/
inline float fitnessBetweenTwoCities(char cityA, char cityB)
{
    return distances[getCityIndex(cityA)][getCityIndex(cityB)];
}


/*
* Populates the distances[] and the distancesOrigin[] arrays, so that the 
* distance between two cities is never computed more than once
*
* @param coordinates[CITIES][2]: coordinates of each cities in the trip
*/
void computeDistanceBetweenCities(int coordinates[CITIES][2]) {
    #pragma omp parallel for firstprivate(coordinates)
    for ( int i = 0; i < CITIES; i++ ) {
        #pragma omp parallel for firstprivate (coordinates)
        for ( int j = i + 1; j < CITIES; j++ ) {
            distances[i][j] = 
                fitnessBetweenTwoCities(coordinates[i], coordinates[j]);
            distances[j][i] = distances[i][j];
        }
        distancesOrigin[i] = fitnessBetweenTwoCities(origin, coordinates[i]);
    }
}


/*
* Retruns wether the first trip is shorter or longer than the second trip
* Used by qsort()
*
* @param a: pointer to first trip
* @param b: pointer to second trip
*/
int compareFitness(const void* a, const void* b) {
    if ( ((Trip*)a)->fitness <  ((Trip*)b)->fitness ) return -1;
    if ( ((Trip*)a)->fitness   == ((Trip*)b)->fitness ) return 0;
    return 1;
}


/*
* Retruns the complete complement corresponding to each city.
* child[i+1] is complement of child[i]
* Calculates the corresponding complement index and then
* returns the char associated with that index
*
* @param city: char whose complement is needed
*
* @return char of the city
*/
inline char getComplementCity(char city) {
    int complementIndex = (CITIES - 1) - getCityIndex(city);
    return getCityFromIndex(complementIndex);
}


/*
* Returns a random unvisited city to add to offsprings[i]
* Randomly generates an index. If the value corresponding to the index
* is true ( visited ), then it again generates another random index.
* It repeats this until unvisted city is found.
*
* @param visitedCities[]: boolean array ( true : visited, false : unvisted )
*
* @return char of the city
*/
char getRandomUnvisitedCity(bool visitedCities[]) {
    int unvistedCityIndex = rand() % CITIES;      // generate a random index
    while ( visitedCities[unvistedCityIndex] ) {  // if the value is true                 
        unvistedCityIndex = rand() % CITIES;      // again generate random                
    }
    return getCityFromIndex(unvistedCityIndex);
}


/*
* Evaluates the distance of each trip and sorts out all the trips using qsort 
* in the shortest-first order
* calculate each trip distance like:
* [0,0] -> 1st city -> 2nd city -> 3rd city -> ..... -> 36th city
* not including a travel back to [0,0]
*
* @param trip[CHROMOSOMES]: all the trips
* @param coordinates[CITIES][2]: coordinates of each cities in the trip
*/
void evaluate(Trip trip[CHROMOSOMES], int coordinates[CITIES][2]) {
    float tempDistances[CHROMOSOMES];           // array to store temp distances
    #pragma omp parallel for default(none) firstprivate(trip, coordinates, distances, distancesOrigin) shared(tempDistances)
    for ( int i = 0; i < CHROMOSOMES; i++ ) {
        #pragma omp parallel for default(none)  firstprivate(trip, i, distances, distancesOrigin) shared(tempDistances)                          
        for ( int j = 0; j < CITIES - 1; j++ ) {
            if (j == 0) {    // starting from origin
                tempDistances[i] = 
                    distancesOrigin[getCityIndex(trip[i].itinerary[0])];
            }                
            // adding all other remaining cities in the trip
            tempDistances[i] += 
                distances[getCityIndex(trip[i].itinerary[j])]
                [getCityIndex(trip[i].itinerary[j + 1])];
        } 
    }
    // inserting the distances into the trip fitness
    #pragma omp parallel for 
    for ( int i = 0; i < CHROMOSOMES; i++ ) {
        trip[i].fitness = tempDistances[i];
    } 
    // qsort to sort the trips in ascending order
    qsort(trip, CHROMOSOMES, sizeof(Trip), compareFitness);
}


/*
* Generates 25,000 offsprings from the parents. It selects the first city of 
* parent1, compares the cities leaving that city in parent_1 and parent_2, and 
* chooses the closer one to extend child_1’s trip.
* If one city has already appeared in the trip, we choose the other city.
* If both cities have already appeared, we randomly select a non-selected city.
* Generate child_2’s trip as a complement of child1.
*
* @param parents[TOP_X]: top 50% of trips to be crossed over to form children
* @param offsprings[TOP_X]: children formed by crossing over the parents 
*        and complement of child1
* @param coordinates[CITIES][2]: coordinates of each cities in the trip
*/
void crossover(Trip parents[TOP_X], Trip offsprings[TOP_X], int coordinates[CITIES][2]) {
    #pragma omp parallel for default(none) shared(offsprings, parents)    // parallelization
    for ( int temp = 0; temp < TOP_X / 2; temp++ )
    {
        // boolean array to track the already visited cities
        // array value is true for cities (corresponding index) in offspring
        bool visitedCities[CITIES] = { false };

        int i = temp * 2;                         // 2 parents in each loop

        char current = parents[i].itinerary[0];   // parent_1

        visitedCities[getCityIndex(current)] = true;

        // inserting the city into offsprings_1
        offsprings[i].itinerary[0] = current;                         
        // offspring_2 : complement of the offspring_1
        offsprings[i + 1].itinerary[0] = getComplementCity(current);  

        #pragma omp parallel for 
        for ( int j = 1; j < CITIES; j++ ) {  
            // closer neighboring city choosen to extend offsprings_1's trip
            char choosenNext = ' ';       

            // to find the current city in parent_1 and parent_2 and 
            //     then its neighbouring city 
            // if the current city is the last index, then city at 
            //     index 0 becomes neighbour
            char* nextFromParent_1 = find(parents[i].itinerary,
                parents[i].itinerary + CITIES, current);
            nextFromParent_1++;
            if ( nextFromParent_1 >= (parents[i].itinerary + CITIES) ) {
                nextFromParent_1 = parents[i].itinerary;
            }

            char* nextFromParent_2 = find(parents[i + 1].itinerary,
                parents[i + 1].itinerary + CITIES, current);
            nextFromParent_2++;
            if ( nextFromParent_2 >= (parents[i + 1].itinerary + CITIES) ) {
                nextFromParent_2 = parents[i + 1].itinerary;
            }

            // Four cases to choose the next city for offsprings_1
            // Case 1: neighbour from parent_1 is already in offsprings_1, 
            //         then neighbour from parent_2 is choosen
            // Case 2: neighbour from parent_2 is already in offsprings_1, 
            //         then neighbour from parent_1 is choosen
            // Case 3: both the neighbours are already in offsprins_1, 
            //         then choose a random unvisited city
            // Case 4: both the neighbours are not present in offsprings_1, 
            //         then the neighbour with the shortest distance is choosen
            if (visitedCities[getCityIndex(*nextFromParent_1)]) {      // Case 1
                if (!visitedCities[getCityIndex(*nextFromParent_2)]) {
                    choosenNext = *nextFromParent_2;
                }
                else {
                    choosenNext = getRandomUnvisitedCity(visitedCities);//Case 3
                }
            }
            else {
                if (visitedCities[getCityIndex(*nextFromParent_2)]) {  // Case 2
                    choosenNext = *nextFromParent_1;
                }
                else {                                                 // Case 4
                    float distance1 = 
                        fitnessBetweenTwoCities(current, *nextFromParent_1);
                    float distance2 =
                        fitnessBetweenTwoCities(current, *nextFromParent_2);
                    if ( distance1 < distance2 ) {
                        choosenNext = *nextFromParent_1;
                    }
                    else {
                        choosenNext = *nextFromParent_2;
                    }
                }
            }
            // updating the current to the newly extended city in the offsprings_1 
            visitedCities[getCityIndex(choosenNext)] = true;
            offsprings[i].itinerary[j] = choosenNext;
            offsprings[i + 1].itinerary[j] = 
                getComplementCity(offsprings[i].itinerary[j]);
            current = choosenNext;      
        }
    }
}


/*
* Randomly chooses two distinct cities in each trip with a given probability,
* and swaps them.
*
* @param parents[TOP_X]: top 50% of trips to be mutated
*/
void mutate( Trip offsprings[TOP_X] ) {
    for (int i = 0; i < TOP_X; i++) {
        // choosing two random cities within given probability
        if (rand() % 100 < MUTATE_RATE) {

            int city_1 = rand() % 36;
            int city_2 = rand() % 36;

            // swap two cities
            if (city_1 != city_2) {
                char temp = offsprings[i].itinerary[city_1];
                offsprings[i].itinerary[city_1] = 
                    offsprings[i].itinerary[city_2];
                offsprings[i].itinerary[city_2] = temp;
            }
        }
    }
}


