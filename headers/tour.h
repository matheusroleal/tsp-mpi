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

int GetTourCost(tour* tour_t);

void CopyTour(tour* dest, tour* orig);

// Just for debugging
void PrintTourInfo(tour* tour_t);
