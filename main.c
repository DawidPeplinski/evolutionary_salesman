#include <stdio.h>
#include <stdlib.h>

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

void allocateCities(struct Map *map, int n)
{
    int i;
    map->n = n;
    map->cities = malloc(sizeof(*map->cities)*n);
    map->cities[0] = malloc(sizeof(*map->cities[0])*n*n);
    for (i = 1; i < n; i++) {
        map->cities[i] = map->cities[i-1] + n;
    }
}

void generateParameters(struct Map *map)
{
    int i, j;
    for (i = 0; i < map->n; i++) {
        for (j = 0; j < i; j++) {
            struct City city;
            city.cost = rand()%100+1;
            city.distance = rand()%200+1;
            city.time = rand()%5+1;

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
            printf("(C%4dD%4dT%4d)", c.cost, c.distance, c.time);

            if (j == map->n - 1) {
                printf("\n");
            }
        }
    }
}

void generateMap(struct Map *map, int n)
{
    allocateCities(map, n);
    generateParameters(map);
    printMap(map);
}

int main()
{
    int n = 4;
    struct Map map;
    generateMap(&map, n);
    return 0;
}