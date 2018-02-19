#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <stddef.h>
#include <unistd.h>

// TODO: see stack overflow: https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y)) 
#define ABS(X) (((X) > 0) ? (X) : (-1*(X))) 

enum sex {MALE, FEMALE};
typedef enum sex sex_t;

enum task_type {MOVE, GATHER};
typedef enum task_type task_type_t;

typedef struct task_t task_t;
typedef struct task_node_t task_node_t;
typedef struct person_t person_t;
typedef struct terrain_t terrain_t;
typedef struct world_t world_t;

typedef struct location_t {
    int x;
    int y;
} location_t;

typedef struct task_node_t {
    person_t * person;

    task_node_t * parent;

    task_node_t ** children; // this can maybe be an array of task nodes instead of an array of task node pointers
    size_t num_children;
    size_t children_cap;
    size_t active_child;

    task_t ** tasks; // this can maybe be an array of tasks instead of an array of task pointers
    size_t num_tasks;
    size_t tasks_cap;
    size_t active_task;
} task_node_t;

typedef struct task_t {
    task_node_t * parent;
    // person_t * person;
    task_type_t type;
    location_t location;
    int counter;
    int done_counter;
} task_t;

typedef struct person_t {
    world_t * world;
    int age;
    sex_t sex;
    location_t location;
    task_node_t * task_tree;
} person_t;

typedef struct terrain_cell_t {
    int elevation;
    int tree;
    int logs;
} terrain_cell_t;

typedef struct terrain_t {
    size_t width;
    size_t height;
    terrain_cell_t * cells;
} terrain_t;

typedef struct world_t {
    task_node_t ** task_queue; // TODO: should this be an array of task nodes instead of an array of task node pointers?
    size_t num_tasks;
    size_t tasks_cap;
    terrain_t terrain;
    size_t num_people;
    size_t people_cap;
    person_t * people;
    char * grid; // for printing
    int tick;
} world_t;

person_t * addPersonToWorld(world_t * w) {
    person_t * person = NULL;
    w->num_people++;
    if(w->num_people > w->people_cap) {
        w->people_cap *= 2;
        w->people = realloc(w->people, sizeof(*(w->people)) * w->people_cap);
    }
    person = w->people + (w->num_people - 1);
    person->world = w;
    return person;
}

void assignTask(person_t * p, task_node_t * t) {
    t->person = p;
    p->task_tree = t;
}

void updateTask(task_t * t) {
    t->counter++;

    if(t->counter >= t->done_counter) {
        if(t->type == MOVE) {
            person_t * person = t->parent->person;
            if(t->location.x - person->location.x > 0) {
                person->location.x++;
            }
            if(t->location.x - person->location.x < 0) {
                person->location.x--;
            }
            if(t->location.y - person->location.y > 0) {
                person->location.y++;
            }
            if(t->location.y - person->location.y < 0) {
                person->location.y--;
            }
        }
        if(t->type == GATHER) {
            int x = t->location.x;
            int y = t->location.y;
            person_t * person = t->parent->person;
            size_t width = person->world->terrain.width;
            person->world->terrain.cells[y*width+x].tree = 0;
        }
        t->parent->active_task++;
        //t->person->task_tree = NULL;
        //free(t);
    }
}

void updateTaskTree(task_node_t * t) {
    if(t == NULL) {
        return;
    }

    if(t->num_children > 0) {
        if(t->active_child == t->num_children) {
            // all the children are done
            if(t->parent != NULL) {
                t->parent->active_child++;
            }
            else {
                // this is the parent; TODO: clear the task
            }
        }
        else {
            updateTaskTree(t->children[t->active_child]);
            // TODO: consider checking return value to decide
            // whether to increment active_child
        }
    }
    else {
        if(t->num_tasks > 0) {
            if(t->active_task == t->num_tasks) {
                // all the tasks are done
                if(t->parent != NULL) {
                    t->parent->active_child++;
                }
                else {
                    // this is the parent; TODO: clear the task
                }
            }
            else {
                updateTask(t->tasks[t->active_task]);
                // TODO: consider checking return value to decide
                // whether to increment active_task
            }
        }
    }
}

void updatePerson(person_t * p) {
    updateTaskTree(p->task_tree);
}

void initWorld(world_t * w) {
    size_t r, c;
    size_t width = 10;
    size_t height = 10;
    w->num_people = 0;
    w->people_cap = 10;
    w->people = malloc(sizeof(*(w->people)) * w->people_cap);
    w->terrain.width = width;
    w->terrain.height = height;
    w->terrain.cells = malloc(sizeof(*w->terrain.cells)*width*height);
    for(r=0; r<height; r++) {
        for(c=0; c<width; c++) {
            w->terrain.cells[r*width+c].elevation = 0;
        }
    }

    w->terrain.cells[5*width+3].tree = 10; // place trees here

    w->grid = NULL;

    w->tick = 0;

    w->tasks_cap = 10;
    w->task_queue = malloc(sizeof(*(w->task_queue)) * w->tasks_cap);
    w->num_tasks = 0;
}

void updateWorld(world_t * w) {

    size_t i;
    for(i=0; i < w->num_people; i++) {
        person_t * p_p = w->people+i;
        updatePerson(p_p);
    }

}

// TODO: implement path-finding algorithm
task_node_t * makeMoveTaskTree(location_t * src, location_t * dst) {
    task_node_t * new_task_node = NULL;
    task_t ** ts = NULL;

    int adx = ABS(dst->x - src->x);
    int ady = ABS(dst->y - src->y);
    int max = MAX(adx, ady);

    ts = malloc(sizeof(*ts)*max);

    int x = src->x;
    int y = src->y;

    size_t i;
    for(i=0; i < max; i++) {
        if(x < dst->x) x++;
        if(x > dst->x) x--;
        if(y < dst->y) y++;
        if(y > dst->y) y--;
        ts[i] = malloc(sizeof(task_t));
        ts[i]->type = MOVE;
        ts[i]->location.x = x;
        ts[i]->location.y = y;
        ts[i]->counter = 0;
        ts[i]->done_counter = 3;
    }

    new_task_node = malloc(sizeof(task_node_t));
    new_task_node->person = NULL;
    new_task_node->parent = NULL;
    new_task_node->children = NULL;
    new_task_node->num_children = 0;
    new_task_node->children_cap = 0;
    new_task_node->active_child = 0;
    new_task_node->tasks = malloc(sizeof(*new_task_node->tasks)*max);
    for(i=0; i < max; i++) {
        new_task_node->tasks[i] = ts[i];
        ts[i]->parent = new_task_node;
    }
    new_task_node->num_tasks = max;
    new_task_node->tasks_cap = max;
    new_task_node->active_task = 0;

    free(ts);

    return new_task_node;
}

void printPerson(person_t * person) {
    printf("person %p:\n",person);
    printf(" location: (%d, %d)\n", person->location.x, person->location.y);
    printf(" task: %p\n", person->task_tree);
}

void printWorld(world_t * world) {
    //size_t i;
    //for(i=0; i < world->num_people; i++) {
    //    printPerson(world->people+i);
    //}
    size_t r, c;
    size_t p;
    size_t width = world->terrain.width;
    size_t height = world->terrain.height;
    char * grid;

    printf("tick %d\n", world->tick);

    if(world->grid == NULL) {
        world->grid = malloc(sizeof(char)*width*height);
    }
    grid = world->grid;
    for(r=0; r<height; r++) {
        for(c=0; c<width; c++) {
            if(world->terrain.cells[r*width+c].tree > 0)
                grid[r*width+c] = 'T';
            else
                grid[r*width+c] = '.';
        }
    }

    for(p=0; p < world->num_people; p++) {
        c = world->people[p].location.x;
        r = world->people[p].location.y;
        grid[r*width+c] = 'P';
    }

    for(r=0; r<height; r++) {
        for(c=0; c<width; c++) {
            printf("%c", grid[r*width+c]);
        }
        printf("\n");
    }

    for(r=0; r<72; r++) {
        printf("\n");
    }
}

void allocateJobs(world_t * w) {
}

void allocateTasks(world_t * w) {
    size_t t, p;
    for(t=0; t< w->num_tasks; t++) {
        if(w->task_queue[t]->person != NULL) {
            continue;
        }
        for(p=0; p< w->num_people; p++) {
            if(w->people[p].task_tree != NULL) {
                continue;
            }
            //printf("assigning task (%d) to person (%d)...\n", t, p );
            w->people[p].task_tree = w->task_queue[t];
            w->task_queue[t]->person = w->people+p;
            break;
        }
    }
}

task_node_t * newTask(world_t * w) {
    task_node_t ** new_task = NULL;
    w->num_tasks++;
    if(w->num_tasks > w->tasks_cap) {
        w->tasks_cap *= 2;
        w->task_queue = realloc(w->task_queue, sizeof(*(w->task_queue)) * w->tasks_cap);
    }
    //new_task = w->task_queue + (w->num_tasks - 1);
    new_task = w->task_queue + (w->num_tasks - 1);
    *new_task = malloc(sizeof(task_node_t));
    (*new_task)->person = NULL;
    return *new_task;
}

int main(int argc, char * argv[]) {
    int done = 0;

    world_t w;
    initWorld(&w);

    person_t * p0_p = addPersonToWorld(&w);
    p0_p->age = 25;
    p0_p->sex = MALE;
    p0_p->location.x = 1;
    p0_p->location.y = 1;
    p0_p->task_tree = NULL;

    person_t * p1_p = addPersonToWorld(&w);
    p1_p->age = 35;
    p1_p->sex = MALE;
    p1_p->location.x = 6;
    p1_p->location.y = 1;
    p1_p->task_tree = NULL;

    while(!done) {

        allocateJobs(&w);
        allocateTasks(&w);
        updateWorld(&w);
        printWorld(&w);

        if(w.tick == 1) {
            location_t src1 = {p0_p->location.x, p0_p->location.y};
            location_t dst1 = {4, 5};
            location_t src2 = {dst1.x, dst1.y};
            location_t dst2 = {2, 3};
            task_node_t * move1 = makeMoveTaskTree(&src1, &dst1);
            task_node_t * gather = malloc(sizeof(*gather));
            task_node_t * move2 = makeMoveTaskTree(&src2, &dst2);
            task_node_t * tree = malloc(sizeof(*tree));

            gather->person = p0_p;
            //gather->parent = tree;
            gather->children = NULL;
            gather->num_children = 0;
            gather->children_cap = 0;
            gather->tasks = malloc(sizeof(*gather->tasks));
            gather->num_tasks = 1;
            gather->tasks_cap = 1;
            gather->active_task = 0;
            gather->tasks[0] = malloc(sizeof(task_t));
            gather->tasks[0]->parent = gather;
            gather->tasks[0]->type = GATHER;
            gather->tasks[0]->location.x = 3;
            gather->tasks[0]->location.y = 5;
            gather->tasks[0]->counter = 0;
            gather->tasks[0]->done_counter = 2;

            move1->person = p0_p;
            move2->person = p0_p;
            //move1->parent = tree;
            //move2->parent = tree;
            //tree->parent = 0;
            //tree->children = malloc(sizeof(*tree->children)*3);
            //tree->children_cap = 3;
            //tree->num_children = 3;
            //tree->children[0] = move1;
            //tree->children[1] = gather;
            //tree->children[2] = move2;
            //tree->active_child = 0;
            //tree->num_tasks = 0;
            //tree->tasks_cap = 0;
            //tree->tasks = NULL;
            //assignTask(p0_p, tree);
            task_node_t * new_task = newTask(&w);
            new_task->parent = 0;
            new_task->children = malloc(sizeof(*tree->children)*3);
            new_task->children_cap = 3;
            new_task->num_children = 3;
            new_task->children[0] = move1;
            new_task->children[1] = gather;
            new_task->children[2] = move2;
            new_task->active_child = 0;
            new_task->num_tasks = 0;
            new_task->tasks_cap = 0;
            new_task->tasks = NULL;

            gather->parent = new_task;
            move1->parent = new_task;
            move2->parent = new_task;

        }

        usleep(10000);
        w.tick++;
        if(w.tick > 100) {
            done = 1;
        }
    }

    return 0;
}
