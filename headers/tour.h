#include "../headers/graph.h"

typedef struct tour_t tour;

tour* CreateTour();

void FreeTour();

void AddCity(tour* tour_t, graph* graph_t, int city);

void RemoveLastCity(tour* tour_t, graph* graph_t);

int TourContainCity(tour* tour_t, int city);

int TourContainCityOrHometown(tour* tour_t, int city);

int BestTour(tour* tour_t, tour* best);

int GetTourLastCity(tour* tour_t);

int GetTourMaxCities(tour* tour_t);

int GetTourNumberCities(tour* tour_t);

float GetTourCost(tour* tour_t);

int * GetCitiesInTour(tour* tour_t);

void AddCitiesToTour(tour* tour_t, graph* graph_t, int* cities, int max_cities);

void CopyTour(tour* dest, tour* orig);

void SetTourCost(tour* tour_t, float new_cost);

// Just for debugging
void PrintTourInfo(tour* tour_t);
