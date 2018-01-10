#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct City
{
    int cost;
    int distance;
    int time;
};

struct Map
{
    int n;
    struct City **cities;
};

struct Population
{
    int n;
    struct Map *map;

    int **specimen;
};

struct GenerationParameters
{
    // minimal distance between cities
    int distance_min;
    // maximum distance between cities
    int distance_max;

    // minimal cost between cities
    int cost_min;
    // maximal cost between cities
    int cost_max;

    // time to distance coefficient min
    float time_coef_min;
    // time to distance coefficient max
    float time_coef_max;

    // If the distance between two cities generated is larger than this
    // value, then the cities become disconnected
    int distance_cutoff_threshold;
};

void allocateCities(struct Map *map)
{
    int i;
    map->cities = malloc(sizeof(*map->cities)*map->n);
    map->cities[0] = malloc(sizeof(*map->cities[0])*map->n*map->n);
    for (i = 1; i < map->n; i++) {
        map->cities[i] = map->cities[i-1] + map->n;
        memset(map->cities[i], 0, sizeof(*map->cities[i])*map->n);
    }
}

void generateParameters(struct Map *map, struct GenerationParameters params)
{
    int i, j;
    for (i = 0; i < map->n; i++) {
        for (j = 0; j < i; j++) {
            struct City city;
            city.cost = rand()%(params.cost_max - params.cost_min) + params.cost_min;
            city.distance = rand()%(params.distance_max - params.distance_min) + params.distance_min;
            float coef = (double)rand()/RAND_MAX*(params.time_coef_max-params.time_coef_min) + params.time_coef_min;
            city.time = city.distance*coef;

            map->cities[i][j] = city;
            map->cities[j][i] = city;
        }
    }
}

void printMap(struct Map *map)
{
    int i, j;

    printf("   ");
    for (i = 0; i < map->n; i++) {
        printf("        %1d         ", i);
    }
    printf("\n");
    for (i = 0; i < map->n; i++) {
        printf("%2d ", i);
        for (j = 0; j < map->n; j++) {
            struct City c = map->cities[i][j];

            if (j) {
                printf(" ");
            }
            if (c.distance) {
            printf("(C%-4dD%-4dT%-4d)", c.cost, c.distance, c.time);
            } else {
                printf("       XXX       ");
            }
            if (j == map->n - 1) {
                printf("\n");
            }
        }
    }
}

void generateMap(struct Map *map, int n)
{
    struct GenerationParameters params;
    params.cost_min = 1;
    params.cost_max = 10;
    params.distance_min = 1;
    params.distance_max = 100;
    params.distance_cutoff_threshold = 50;
    params.time_coef_min = 0.1;
    params.time_coef_max = 0.5;

    map->n = n;

    allocateCities(map);
    generateParameters(map, params);
}

void allocateSpecimen(struct Population *pop)
{
    int i;
    pop->specimen = malloc(sizeof(*pop->specimen)*pop->n);
    pop->specimen[0] = malloc(sizeof(*pop->specimen[0])*pop->n*pop->n);

    for (i = 1; i < pop->n; i++) {
        pop->specimen[i] = pop->specimen[i-1] + pop->n;

        memset(pop->specimen[i], 0, sizeof(*pop->specimen[i])*pop->n);
    }
}

void randomPopulation(struct Population *pop)
{
    int i, j;
    int *indices = malloc(sizeof(*indices)*pop->map->n);

    for (i = 0; i < pop->n; i++) {
        for (j = 0; j < pop->map->n; j++) {
            indices[j] = j;
        }

        for (j = 0; j < pop->map->n; j++) {
            int index = rand()%(pop->map->n-j);

            pop->specimen[i][j] = indices[index];
            indices[index] = indices[pop->map->n-j-1];
        }

    }

    free(indices);
}

void generatePopulation(struct Population *pop, struct Map *map, int pop_n)
{
    pop->map = map;
    pop->n = pop_n;

    allocateSpecimen(pop);
    randomPopulation(pop);
}

void scoreSpecimen(int *specimen, struct Map *map, int *cost, int *distance, int *time)
{
    int i;
    
    *cost = 0;
    *distance = 0;
    *time = 0;


    for (i = 1; i < map->n; i++) {
        struct City c = map->cities[specimen[i-1]][specimen[i]];
        *cost += c.cost;
        *distance += c.distance;
        *time += c.time;
    }
}

void printPopulation(struct Population *pop)
{
    int i, j;

    for (i = 0; i < pop->n; i++) {
        int cost, distance, time;

        printf("%d: ", i);
        
        for (j = 0; j < pop->map->n; j++) {
            if (j) {
                printf(" ");
            }
            printf("%d", pop->specimen[i][j]);
        }

        scoreSpecimen(pop->specimen[i], pop->map, &cost, &distance, &time);
        printf("C%-4dD%-4dT%-4d", cost, distance, time);        
        printf("\n");
    }
}

int main()
{
    int n = 4;
    int pn = 8;
    struct Map map;
    struct Population pop;
    
    generateMap(&map, n);
    printf("### MAP ###\n");
    printMap(&map);
    printf("\n");

    generatePopulation(&pop, &map, pn);
    printf("### EVOLUTIONARY PROGRAMMING ###\n");
    printf("Initial population:\n");
    printPopulation(&pop);
    printf("\n");

    return 0;
}