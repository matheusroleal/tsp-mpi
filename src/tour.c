#include <stdlib.h>
#include <stdio.h>
#include "../headers/tour.h"

struct tour_t {
  int* cities;
  int max_cities;
  int n_cities;
  float cost;
};


tour* CreateTour(int max_cities) {
  tour* tour_t = (tour*) calloc (1, sizeof(tour));

  if(!tour_t) { 
    printf("Failed to create tour.\n"); 
    exit(-1); 
  }

  tour_t->cities = (int*) calloc (max_cities, sizeof(int));

  if(!tour_t->cities) { 
    printf("Failed to create cities array.\n"); 
    exit(-1); 
  }

  for(int i=0; i < max_cities; i++) { 
    tour_t->cities[i] = -1; 
  };
  tour_t->max_cities = max_cities;
  tour_t->n_cities = 0;
  tour_t->cost = 0;

  return tour_t;
}

void FreeTour(tour* tour_t) {
  free(tour_t->cities);
  free(tour_t);
}

void AddCity(tour* tour_t, graph* graph_t, int city) {
  int last_city = tour_t->cities[tour_t->n_cities - 1];

  if (tour_t->n_cities == tour_t->max_cities) {
    printf("Tour full!\n");
    return;
  }

  tour_t->cities[tour_t->n_cities] = city;
  tour_t->n_cities++;
  tour_t->cost += GetEdgeWeight(graph_t, last_city, city);
}

void RemoveLastCity(tour* tour_t, graph* graph_t) {
  int last_city = tour_t->cities[tour_t->n_cities - 1];
  int before_last_city = tour_t->cities[tour_t->n_cities - 2];
  float cost = GetEdgeWeight(graph_t, before_last_city, last_city);

  tour_t->cities[tour_t->n_cities] = -1;
  tour_t->n_cities--;
  tour_t->cost -= cost;
}

int TourContainCity(tour* tour_t, int city) {
  // not include hometown in this verification
  for(int i=1; i < tour_t->n_cities; i++) {

    if(tour_t->cities[i] == city) { 
      return 1; 
    }
  }

  return 0;
}

int TourContainCityOrHometown(tour* tour_t, int city) {
  for(int i=0; i < tour_t->n_cities; i++) {

    if(tour_t->cities[i] == city) { 
      return 1; 
    }
  }
  return 0;
}

int BestTour(tour* tour_t, tour* best) {
  // initial case, when best empty
  if(best->n_cities == 0) { 
    return 1; 
  }
  if(tour_t->cost < best->cost) { 
    return  1; 
  }

  return 0;
}

int GetTourLastCity(tour* tour_t) {
  return tour_t->cities[tour_t->n_cities - 1];
}

int GetTourMaxCities(tour* tour_t) {
  return tour_t->max_cities;
}

int GetTourNumberCities(tour* tour_t) {
  return tour_t->n_cities;
}

float GetTourCost(tour* tour_t) {
  return tour_t->cost;
}

void CopyTour(tour* dest, tour* orig) {
  for (int i=0; i < dest->max_cities; i++) {
    dest->cities[i] = orig->cities[i];
  }

  dest->max_cities = orig->max_cities;
  dest->n_cities = orig->n_cities;
  dest->cost = orig->cost;
}

int * GetCitiesInTour(tour* tour_t) {
  int * cities = (int*) calloc (tour_t->max_cities, sizeof(int));
  for (int i=0; i < tour_t->max_cities; i++) {
    cities[i] = tour_t->cities[i];
  }

  return cities;
}

void AddCitiesToTour(tour* tour_t, graph* graph_t, int* cities, int max_cities) {
  for (int i=0; i < max_cities; i++) {
    if (cities[i] != -1)
      AddCity(tour_t, graph_t, cities[i]);
  }
}

void SetTourCost(tour* tour_t, float new_cost) {
  tour_t->cost = new_cost;
}

// Just for debugging
void PrintTourInfo(tour* tour_t) {
  for (int i=0; i < tour_t->max_cities; i++) {
    printf("%d ", tour_t->cities[i]);
  }

  printf("\nNCITIES: %d\nCOST: %.2f\n\n", tour_t->n_cities, tour_t->cost);
}
