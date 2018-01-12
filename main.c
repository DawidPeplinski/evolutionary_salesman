#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <omp.h>

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
    int disconnected = 0;
    for (i = 0; i < map->n; i++) {
        for (j = 0; j < i; j++) {
            struct City city;
            city.cost = rand()%(params.cost_max - params.cost_min) + params.cost_min;
            city.distance = rand()%(params.distance_max - params.distance_min) + params.distance_min;
            float coef = (double)rand()/RAND_MAX*(params.time_coef_max-params.time_coef_min) + params.time_coef_min;
            city.time = city.distance*coef;

            if (city.distance > params.distance_cutoff_threshold) {
                city = DISCONNECTED_CITY;
                disconnected++;
            }

            map->cities[i][j] = city;
            map->cities[j][i] = city;
        }

        map->cities[i][i] = DISCONNECTED_CITY;
    }
    printf("%d connections cut off.\n", disconnected);
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
    int connections;
    for (i = 0; i < map->n; i++) { 
        connections = 0;
        
        for (j = 0; j < map->n; j++) {
            if (map->cities[i][j].cost > 0
               || map->cities[j][i].cost > 0) {
                
                connections++;
            }
        }
        if (connections <= 1) {
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
    int i, j, c;
    int *indices = malloc(sizeof(*indices) * pop->map->n);

    clock_t t = clock();
    for (i = 0; i < pop->n; i++) {
        struct Specimen *spec = &pop->specimens[i];

        while (1) {
            char valid = 1;
            // Generate indices to permutate
            for (j = 0; j < pop->map->n; j++) {
                indices[j] = j;
            }
            // 
            for (j = 0; j < pop->map->n; j++) {
                int index;
                c = 0;
                while (1) {
                    if (j < pop->map->n-1) {
                        index = rand()%(pop->map->n-j);
                    } else {
                        index = 0;
                    }
                    c++;
                    if (j > 0 && c < pop->map->n*2
                        && !isCityConnected(pop->map->cities[spec->sequence[j-1]][indices[index]])) {
                        //printf("retrying...\n");
                        continue;
                    }
                    break;
                } 

                spec->sequence[j] = indices[index];
                indices[index] = indices[pop->map->n-1-j];

                if (j > 0 && !isCityConnected(pop->map->cities[spec->sequence[j-1]][spec->sequence[j]])) {
                    valid = 0;
                    break;
                }
            }

            if (valid) {
                break;
            }

        }

        if (i) {
            printf(" ");
        }
        printf("%d", i + 1);
    }
    t = clock() - t;
    printf("\n");
    printf("Specimens generated in %lfs.\n", (double)t/CLOCKS_PER_SEC);

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
            printf("%3d", spec->sequence[j]);
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

int distanceOnlyFunction(int cost, int distance, int time)
{
    return distance;
}

int comparePlayers(const void *vp1, const void *vp2)
{
    const struct Player *p1 = (const struct Player *) vp1;
    const struct Player *p2 = (const struct Player *) vp2;

    return p2->rank - p1->rank;
}

void performTournament(struct Population *pop, struct Population *pop2, int battles)
{
    int n = pop->n + pop2->n;
    int seqlen = pop->map->n;
    struct Player *players = malloc(sizeof(*players)*n);
    int i, j, c;
    int *plseqs = malloc(sizeof(*players[0].sequence)*seqlen*n);
    players[0].sequence = plseqs;
    for (i = 1; i < n; i++) {
        players[i].sequence = players[i-1].sequence + seqlen;
    }

    c = 0;
    // Copying specimens from "pop"
    for (i = 0; i < pop->n; i++) {
        players[c].rank = 0;
        players[c].score = pop->specimens[i].score;
        memcpy(players[c].sequence, pop->specimens[i].sequence, sizeof(int)*seqlen);
        c++;
    }

    // Copying specimens from "pop2"
    for (i = 0; i < pop2->n; i++) {
        players[c].rank = 0;
        players[c].score = pop2->specimens[i].score;
        memcpy(players[c].sequence, pop2->specimens[i].sequence, sizeof(int)*seqlen);
        c++;
    }

    // Preforming the tournament
    for (i = 0; i < n; i++) {
        for (j = 0; j < battles; j++) {
            do {
                c = rand()%n;
            } while(c == i);

            if (players[i].score < players[c].score) {
                players[i].rank++;
            }
        }
    }

    // Sorting players
    qsort(players, n, sizeof(*players), comparePlayers);
    /*
    printf("Tournament results (%d participants):\n", n);
    for (i = 0; i < n; i++) {
        printf("#%2d R%-3d ", i, players[i].rank);
        for (j = 0; j < seqlen; j++) {
            if (j) {
                printf(" ");
            }
            printf("%3d", players[i].sequence[j]);
        }
        printf(" S%-5d\n", players[i].score);
    }
    */
    for (i = 0; i < pop->n; i++) {
        pop->specimens[i].score = players[i].score;
        memcpy(pop->specimens[i].sequence, players[i].sequence,
                sizeof(*players[i].sequence)*seqlen);
    }

    free(plseqs);
    free(players);
}

void measurePopulationStatistics(struct Population *pop, FILE *o)
{
    int i;
    double mean = 0;
    double var = 0;
    
    int min = INT_MAX;
    int max = 0;

    for (i = 0; i < pop->n; i++) {
        int sc = pop->specimens[i].score;
        mean += sc;
        var += sc*sc;

        if (sc < min) {
            min = sc;
        }
        if (sc > max) {
            max = sc;
        }
    }
    mean /= pop->n;
    var /= pop->n;
    var -= mean*mean;

    printf("     Average score: %lf\n", mean);
    // printf("          Variance: %lf\n", var);
    printf("     Minimum score: %d\n", min);
    printf("     Maximum score: %d\n", max);
    fprintf(o, "%lf %d %d\n", mean, min, max);
}

void printGenerationParameters(struct GenerationParameters params)
{
    printf(" - cost: <%d;%d>\n", params.cost_min, params.cost_max);
    printf(" - distance: <%d;%d>\n", params.distance_min, params.distance_max);
    printf(" - distance cutoff threshold: %d\n", params.distance_cutoff_threshold);
    printf(" - time-distance coefficient: <%f;%f>\n", params.time_coef_min, params.time_coef_max);
}

int main()
{
    enum {STEPS = 5000};
    enum {CITIES = 1000};
    enum {SPECIMENS = 100};
    int step;
    struct Map map;
    struct Population pop;
    struct Population pop2;
    int i;
    
    FILE *output = fopen("stats.dat", "w");
    fprintf(output, "# AVG MIN MAX\n");

    srand(time(NULL));
    //srand(1234);
    struct GenerationParameters params;

    params.cost_min = 1;
    params.cost_max = 99;
    params.distance_min = 1;
    params.distance_max = 99;
    params.distance_cutoff_threshold = 50;
    params.time_coef_min = 0.2;
    params.time_coef_max = 0.8;

    printGenerationParameters(params);
    generateMap(&map, CITIES, params);
    if (CITIES <= 12) {
        printf("### %d CITIES ###\n", CITIES);
        printMap(&map);
        printf("\n");
    } else {
        printf("Generated a map of %d cities.\n", CITIES);
    }

    newPopulation(&pop, &map, SPECIMENS, equalScoreFunction);
    printf("Generating %d specimens.\n", SPECIMENS);
    randomPopulation(&pop);
    newPopulation(&pop2, &map, SPECIMENS, equalScoreFunction);

    printf("### EVOLUTIONARY PROGRAMMING ###\n");
    #if 0
    printf("Initial population:\n");
    printPopulation(&pop);
    printf("\n");
    #endif

    for (step = 0; step < STEPS; step++) {
        printf("EP STEP #%d\n", step+1);
        copyPopulation(&pop2, &pop);
        //printPopulation(&pop2);
        mutatePopulation(&pop2);
        //printf("Mutated population:\n");
        //printPopulation(&pop2);
        performTournament(&pop, &pop2, SPECIMENS);
        //printPopulation(&pop);
        measurePopulationStatistics(&pop, output);
    }
    
    printf("Wynik:\n");
    for (i = 0; i < CITIES; i++) {
        if (i) {
            printf(" ");
        }
        printf("%d", pop.specimens[0].sequence[i]);
    }

    fclose(output);
    freePopulation(&pop);
    freeMap(&map);
    return 0;
}