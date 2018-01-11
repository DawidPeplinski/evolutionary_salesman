#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

struct City
{

    int cost;
    int distance;
    int time;
};

static const struct City DISCONNECTED_CITY = {
    .cost = -1,
    .distance = -1,
    .time = -1
};


struct Map
{
    int n;
    struct City **cities;
};

struct Specimen
{
    int score;
    int *sequence;
};

struct Player
{
    int score;
    int rank;
    int *sequence;
};

typedef int (*populationScoreFunctionType)(int cost, int distance, int time);
struct Population
{
    int n;
    struct Map *map;
    struct Specimen *specimens;
    populationScoreFunctionType fscore;
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


void scoreSpecimens(struct Population *pop);

int isCityConnected(struct City city)
{
    return city.cost > 0;
}

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

            if (city.distance > params.distance_cutoff_threshold) {
                city = DISCONNECTED_CITY;
            }

            map->cities[i][j] = city;
            map->cities[j][i] = city;
        }

        map->cities[i][i] = DISCONNECTED_CITY;
    }
}

void printMap(struct Map *map)
{
    int i, j;

    printf("    ");
    for (i = 0; i < map->n; i++) {
        printf("    %-2d      ", i);
    }
    printf("\n");
    for (i = 0; i < map->n; i++) {
        printf("%2d ", i);
        for (j = 0; j < map->n; j++) {
            struct City c = map->cities[i][j];

            if (j) {
                printf(" ");
            }
            if (isCityConnected(c)) {
            printf("(C%-2dD%-2dT%-2d)", c.cost, c.distance, c.time);
            } else {
                printf("    XXX    ");
            }
            if (j == map->n - 1) {
                printf("\n");
            }
        }

    }
}

int validateMap(struct Map *map)
{
    int i;
    int j;
    unsigned char is_orphan;

    for (i = 0; i < map->n; i++) { 
        is_orphan = 1;
        for (j = 0; j < map->n; j++) {
            if (map->cities[i][j].cost > 0
               || map->cities[j][i].cost > 0) {
                
                is_orphan = 0;
                break;
            }
        }
        if (is_orphan) {
            return 0;
        } 
    }  
    return 1;
}

void generateMap(struct Map *map, int n, struct GenerationParameters params)
{

    map->n = n;

    allocateCities(map);
    while (1) {
        generateParameters(map, params);
        printf("Validating map...\n");
        if (!validateMap(map)) {
            printMap(map);
            printf("Regenerating map, because it contained oprhan cities.\n");
            continue;
        }

        break;
    }
}

void freeMap(struct Map *map)
{
    free(map->cities[0]);
    free(map->cities);
}

void allocateSpecimen(struct Population *pop)
{
    int i;
    pop->specimens = malloc(sizeof(*pop->specimens)*pop->n);
    pop->specimens[0].sequence = malloc(sizeof(*pop->specimens[0].sequence)*pop->map->n*pop->n);
    memset(pop->specimens[0].sequence, 0, sizeof(*pop->specimens[i].sequence)*pop->n*pop->map->n);
    for (i = 1; i < pop->n; i++) {
        pop->specimens[i].sequence = pop->specimens[i-1].sequence + pop->map->n;
    }
}

int validateSequence(int *seq, struct Map *map)
{
    int i;
    for (i = 1; i < map->n; i++) {
        struct City c = map->cities[seq[i-1]][seq[i]];

        if (!isCityConnected(c)) {
            return 0;
        }
    }
    return 1;
}

int mutateSpecimen(struct Specimen *specimen, struct Map *map)
{
    int i, j;
    int *tmpseq = malloc(sizeof(*specimen)*map->n);

    int *swapi = malloc(sizeof(*swapi)*map->n);
    int *swapj = malloc(sizeof(*swapj)*map->n);

    int randi, randj, buf;

    memcpy(tmpseq, specimen->sequence, sizeof(*tmpseq)*map->n);

    for (i = 0; i < map->n; i++) {swapi[i] = i;}

    for (i = 0; i < map->n; i++) {
        randi = rand()%(map->n - i);

        for (j = 0; j < map->n; j++) {swapj[j] = j;}

        for (j = 0; j < map->n; j++) {
            randj = rand()%(map->n - j);
            if (swapj[randj] == swapi[randi]) {
                if (j == map->n-1) {
                    break;
                }

                //printf("Skipping equal swap places %d %d.\n", i, j);
                j--;
                continue;
            }
            memcpy(tmpseq, specimen->sequence, sizeof(*tmpseq)*map->n);

            buf = tmpseq[swapi[randi]];
            tmpseq[swapi[randi]] = tmpseq[swapj[randj]];
            tmpseq[swapj[randj]] = buf;
            
            if (validateSequence(tmpseq, map)) {
                //printf("Mutation swapped %d and %d\n", swapi[randi], swapj[randj]);
                memcpy(specimen->sequence, tmpseq, map->n);

                goto mutated;
            }

            swapj[randj] = swapj[map->n - 1 - j];
        }

        swapi[randi] = swapi[map->n - 1 - i];
    }

    //failed:
    printf("Mutation unsuccessful :(\n");

    mutated:
    free(tmpseq);
    free(swapi);
    free(swapj);

    return 1;
}

void mutatePopulation(struct Population *pop)
{
    int i;
    for (i = 0; i < pop->n; i++) {
        mutateSpecimen(&pop->specimens[i], pop->map);
    }

    scoreSpecimens(pop);
}

void randomPopulation(struct Population *pop)
{
    int i, j;
    int *indices = malloc(sizeof(*indices) * pop->map->n);

    for (i = 0; i < pop->n; i++) {

        while (1) {
            // Generate random perumation of indices
            for (j = 0; j < pop->map->n; j++) {
                indices[j] = j;
            }
            // 
            for (j = 0; j < pop->map->n; j++) {
                int index;
                if (j < pop->map->n-1) {
                    index = rand()%(pop->map->n-j);
                } else {
                    index = 0;
                }
                
                pop->specimens[i].sequence[j] = indices[index];
                indices[index] = indices[pop->map->n-1-j];
            }
            
            if (validateSequence(pop->specimens[i].sequence, pop->map)) {
                break;
            }
            
        }
        
        printf("Specimen #%d generated.\n", i);
    }

    scoreSpecimens(pop);

    free(indices);
}

void newPopulation(struct Population *pop, struct Map *map, int pop_n, populationScoreFunctionType fscore)
{
    pop->map = map;
    pop->n = pop_n;
    pop->fscore = fscore;

    allocateSpecimen(pop);
}

void copyPopulation(struct Population *dest, struct Population *src)
{
    if (dest->n != src->n) {
        printf("Population length mismatch!\n");
        return;
    }

    int i;

    for (i = 0; i < dest->n; i++) {
        struct Specimen *d = &dest->specimens[i];
        struct Specimen *s = &src->specimens[i];
        d->score = s->score;
        memcpy(d->sequence, s->sequence, sizeof(*d->sequence)*dest->map->n);
    }
}

void scoreSpecimens(struct Population *pop)
{
    int i, j;
    for (i = 0; i < pop->n; i++) {
        int *seq = pop->specimens[i].sequence;

        int totalCost = 0;
        int totalDistance = 0;
        int totalTime = 0;

        for (j = 1; j < pop->map->n; j++) {
            struct City c = pop->map->cities[seq[j-1]][seq[j]];
            totalCost += c.cost;
            totalDistance += c.distance;
            totalTime += c.time;
        }

        pop->specimens[i].score = pop->fscore(totalCost, totalDistance, totalTime);
    }
}

void printPopulation(struct Population *pop)
{
    int i, j;

    for (i = 0; i < pop->n; i++) {
        struct Specimen *spec = &pop->specimens[i];

        printf("%3d: ", i);
        
        for (j = 0; j < pop->map->n; j++) {
            if (j) {
                printf(" ");
            }
            printf("%d", spec->sequence[j]);
        }

        // scoreSpecimen(spec->sequence, pop->map, &cost, &distance, &time);
        printf(" S%-5d\n", spec->score);
    }
}

void freePopulation(struct Population *pop)
{
    free(pop->specimens[0].sequence);
    free(pop->specimens);
}

int equalScoreFunction(int cost, int distance, int time)
{
    return cost + distance + time;
}

void performTournament(struct Population *pop, struct Population *pop2)
{
    int n = pop->n + pop2->n;
    int seqlen = pop->map->n;
    struct Player *players = malloc(sizeof(*players)*n);
    int i, c;

    c = 0;
    for (i = 0; i < pop->n; i++) {
        players[c].rank = 0;
        players[c].score = pop->specimens[i].score;
        players[c].sequence = malloc(sizeof(*players[c].sequence) * seqlen);
        memcpy(players[c].sequence, pop->specimens[i].sequence, sizeof(int)*seqlen);
    }

    //for (j = 0; )
}

int main()
{
    enum {STEPS = 1};
    enum {CITIES = 12};
    enum {SPECIMENS = 32};
    int step;
    struct Map map;
    struct Population pop;
    struct Population pop2;
    
    srand(time(NULL));

    struct GenerationParameters params;

    params.cost_min = 1;
    params.cost_max = 10;
    params.distance_min = 1;
    params.distance_max = 99;
    params.distance_cutoff_threshold = 75;
    params.time_coef_min = 0.1;
    params.time_coef_max = 0.5;

    generateMap(&map, CITIES, params);
    printf("### MAP ###\n");
    printMap(&map);
    printf("\n");

    newPopulation(&pop, &map, SPECIMENS, equalScoreFunction);
    randomPopulation(&pop);
    newPopulation(&pop2, &map, SPECIMENS, equalScoreFunction);

    printf("### EVOLUTIONARY PROGRAMMING ###\n");
    printf("Initial population:\n");
    printPopulation(&pop);
    printf("\n");

    for (step = 0; step < STEPS; step++) {
        printf("EP STEP #%d\n", step+1);
        copyPopulation(&pop2, &pop);
        printPopulation(&pop2);
        mutatePopulation(&pop2);
        printf("Mutated population:\n");
        printPopulation(&pop2);
        //performTournament(&pop, &pop2);
    }
    

    freePopulation(&pop);
    freeMap(&map);
    return 0;
}