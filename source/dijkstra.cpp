#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "../math.h"
#include "../types.h"
#include "../memory.h"
#include "../random.h"

#include <SDL2/SDL.h>

#define WINDOW_SIZE 640
#define NODE_RADIUS 2

#define assert(expr, string)   \
    if(!(expr))                 \
    {                           \
        puts(string);           \
        *(s32 *)0 = 0;          \
    }                           \

static MemoryBlock block;

struct Node;
struct Edge;

struct Node
{
    V2 p;
    Node *from;
    Edge *edges;
};

struct Edge
{
    Edge *next;
    Node *to;
    f32 weight;
};

struct HeapEntry
{
    f32 key;
    u64 index;
};

struct Heap
{
    u64 size;
    HeapEntry *entries;
};

void heap_insert(Heap *heap, f32 node_key, u64 node_index, u64 *heap_indices)
{
    u64 index = heap->size++;
    HeapEntry entry = {node_key, node_index};
    while(index > 0)
    {
        u64 parent = (index - 1) / 2;
        if(heap->entries[parent].key > node_key)
        {
            heap->entries[index] = heap->entries[parent];
            heap_indices[heap->entries[index].index] = index;
            
            heap->entries[parent] = entry;
            heap_indices[heap->entries[parent].index] = parent;
            
            index = parent;
        }
        else
        {
            break;
        }
    }
    heap->entries[index] = entry;
    heap_indices[heap->entries[index].index] = index;
}

Node *heap_remove(Heap *heap, u64 index, u64 *heap_indices, Node *nodes)
{
    Node *result = 0;
    
    if(heap->size > index)
    {
        result = nodes + heap->entries[index].index;
        HeapEntry entry = heap->entries[--heap->size];
        heap->entries[index] = entry;

        while(index > 0)
        {
            u64 parent = (index - 1) / 2;
            if(heap->entries[parent].key > entry.key)
            {
                heap->entries[index] = heap->entries[parent];
                heap_indices[heap->entries[index].index] = index;
                
                heap->entries[parent] = entry;
                heap_indices[heap->entries[parent].index] = parent;
                
                index = parent;
            }
            else
            {
                break;
            }
        }
        heap->entries[index] = entry;
        heap_indices[heap->entries[index].index] = index;

        while(index < heap->size)
        {
            u64 child_0 = 2 * index + 1;
            u64 child_1 = 2 * index + 2;

            if(child_0 < heap->size)
            {
                if((child_1 < heap->size) &&
                   (heap->entries[child_1].key < heap->entries[child_0].key))
                {
                    child_0 = child_1;
                }
                
                if(entry.key > heap->entries[child_0].key)
                {
                    heap->entries[index] = heap->entries[child_0];
                    heap_indices[heap->entries[index].index] = index;
                    
                    heap->entries[child_0] = entry;
                    heap_indices[heap->entries[child_0].index] = child_0;
                    
                    index = child_0;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
    return result;
}

void dijkstra(Node *from, Node *nodes, u32 node_count)
{
    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);

    u64 was_used = block.used;
    
    Heap heap = {};
    heap.entries = push_array(&block, HeapEntry, node_count);
    
    u64 *heap_indices = push_array(&block, u64, node_count);
    f32 *distances = push_array(&block, f32, node_count);
    bool *visited = push_array(&block, bool, node_count);
    
    for(u32 node_index = 0; node_index < node_count; ++node_index)
    {
        distances[node_index] = infinity;
        visited[node_index] = false;
        nodes[node_index].from = 0;
    }

    Node *current = from;
    u64 current_index = current - nodes;
    distances[current_index] = 0;

    do
    {
        for(Edge *edge = current->edges; edge; edge = edge->next)
        {
            Node *node = edge->to;
            u64 node_index = node - nodes;

            if(!visited[node_index])
            {
                f32 distance = distances[current_index] + edge->weight;

                if(distances[node_index] > distance)
                {
                    if(distances[node_index] < infinity)
                    {
                        u64 heap_index = heap_indices[node_index];
                        heap_remove(&heap, heap_index, heap_indices, nodes);
                    }
                    distances[node_index] = distance;
                    node->from = current;

                    heap_insert(&heap, distance, node_index, heap_indices);
                }
            }
        }

        visited[current_index] = true;
        u64 current_heap_index = heap_indices[current_index];
        current = heap_remove(&heap, current_heap_index, heap_indices, nodes);
        current_index = current - nodes;
    }
    while(current);

    printf("%lu bytes used\n", block.used - was_used);
    block.used = was_used;
    
    clock_gettime(CLOCK_MONOTONIC, &finish);
    f64 elapsed = (finish.tv_sec - start.tv_sec) + 0.000000001f * (finish.tv_nsec - start.tv_nsec);
    printf("Done in %.4f seconds\n", elapsed);
}

#define BUFFER_SIZE 256

s32 main(s32 argc, char **argv)
{
    if(argc == 2)
    {
        FILE *input = fopen(argv[1], "r");

        if(input)
        {
            block.size = 2l * 1024 * 1024 * 1024;
            block.base = malloc(block.size);
        
            u64 buffer_size = BUFFER_SIZE;
            char *buffer = push_array(&block, char, buffer_size);
        
            u32 node_count = 0;
            u32 edge_count = 0;
        
            Node *nodes = (Node *)get_offset(&block);
            
            while(getline(&buffer, &buffer_size, input) != -1)
            {
                if(*buffer == 'v')
                {
                    Node *node = push_struct(&block, Node);
                    sscanf(buffer, "v %f %f\n", &node->p.x, &node->p.y);
                    node->edges = 0;
                    ++node_count;
                }
                else if(*buffer == 'e')
                {
                    u32 from, to;
                    sscanf(buffer, "e %d %d\n", &from, &to);
        
                    assert(from < node_count && to < node_count, "invalid node index");
        
                    Edge *edge = push_struct(&block, Edge);
                    edge->to = nodes + to;
                    edge->next = nodes[from].edges;
                    edge->weight = distance(nodes[from].p, nodes[to].p);
                    nodes[from].edges = edge;
        
                    ++edge_count;
                }
            }
        
            printf("Graph loaded\n%d verices\n%d edges\n", node_count, edge_count);
            
            f32 relative_circle_radius_squared = squared(NODE_RADIUS / (f32)WINDOW_SIZE);
            Node *selected_from = nodes + 0;
            Node *selected_to = 0;
            V2 mouse_p = {infinity, infinity};
            
            SDL_Init(SDL_INIT_VIDEO);
            SDL_Window *screen = SDL_CreateWindow("Dijskstra", 0, 0, WINDOW_SIZE, WINDOW_SIZE, 0);
            SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            SDL_Event event;
        
            s32 texture_size = 33;
            SDL_Surface *circle_surface = SDL_CreateRGBSurface(0,
                                                            texture_size,
                                                            texture_size,
                                                            32,
                                                            0xff000000,
                                                            0x00ff0000,
                                                            0x0000ff00,
                                                            0x000000ff);
        
            V2 center = v2((texture_size - 1) / 2, (texture_size - 1) / 2);
            f32 outer_radius = 0.5 * (texture_size - 2);
            Circle outer_circle = {center, outer_radius};
            f32 inner_radius = 0.4 * (texture_size - 2);
            Circle inner_circle = {center, inner_radius};
            u32 *pixels = (u32 *)circle_surface->pixels;
        
            for(s32 y = 0; y < texture_size; ++y)
            {
                for(s32 x = 0; x < texture_size; ++x)
                {
                    V2 pixel_p = v2(x, y);
                    u32 color = inside(outer_circle, pixel_p) &&
                                !inside(inner_circle, pixel_p) ?
                                0xffffff80 : 0x00000000;
                    *pixels++ = color;
                }
            }
            
            SDL_Texture* circle_texture =
                SDL_CreateTextureFromSurface(renderer, circle_surface);
        
            SDL_Texture *backbuffer = SDL_CreateTexture(renderer,
                                                        SDL_PIXELFORMAT_RGBA8888,
                                                        SDL_TEXTUREACCESS_TARGET,
                                                        WINDOW_SIZE,
                                                        WINDOW_SIZE);
        
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            
            SDL_SetRenderTarget(renderer, backbuffer);
            SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0x80);
            SDL_Rect rect = {0, 0, 2 * NODE_RADIUS, 2 * NODE_RADIUS};
            clock_t t = clock();
            for(u32 node_index = 0; node_index < node_count; ++node_index)
            {
                V2 a_p = nodes[node_index].p * WINDOW_SIZE;
                /*
                rect.x = (s32)(a_p.x - NODE_RADIUS);
                rect.y = (s32)(a_p.y - NODE_RADIUS);
                SDL_RenderCopy(renderer, circle_texture, 0, &rect);
                */
                for(Edge *edge = nodes[node_index].edges; edge; edge = edge->next)
                {
                    V2 b_p = edge->to->p * WINDOW_SIZE;
                    SDL_RenderDrawLine(renderer, a_p.x, a_p.y, b_p.x, b_p.y);
                }
            }
            printf("Drawn in %.4f seconds\n", (clock() - t) / (f32)CLOCKS_PER_SEC);
            SDL_SetRenderTarget(renderer, 0);
            SDL_RenderCopy(renderer, backbuffer, 0, 0);
        
            bool rolling = true;
            while(rolling)
            {
                SDL_RenderPresent(renderer);
        
                bool mouse_moved = false;
                bool mouse_clicked = false;
                bool selection_changed = false;
                
                while(SDL_PollEvent(&event))
                {
                    switch(event.type)
                    {
                        case SDL_KEYDOWN:
                        case SDL_QUIT:
                        {
                            rolling = false;
                            break;
                        }
                        
                        case SDL_MOUSEMOTION:
                        {
                            mouse_moved = true;
                            mouse_p = v2(event.motion.x, event.motion.y) / (f32)WINDOW_SIZE;
                            break;
                        }
            
                        case SDL_MOUSEBUTTONDOWN:
                        {
                            mouse_clicked = true;
                            break;
                        }
                    };
                }
        
                if(mouse_moved)
                {
                    for(u32 index_a = 0; index_a < node_count; ++index_a)
                    {
                        Node *node_a = nodes + index_a;
                        if(distance_squared(node_a->p, mouse_p) < relative_circle_radius_squared &&
                        node_a != selected_to)
                        {
                            selected_to = node_a;
                            selection_changed = true;
                        }
                    }
                }
        
                if(selection_changed)
                {
                    SDL_RenderCopy(renderer, backbuffer, 0, 0);
                    SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff);
                    Node *to = selected_to;
                    for(Node *from = to->from; from;)
                    {
                        V2 to_p = to->p * WINDOW_SIZE;
                        V2 from_p = from->p * WINDOW_SIZE;
                        SDL_RenderDrawLine(renderer, to_p.x, to_p.y,
                                            from_p.x, from_p.y);
                        to = from;
                        from = to->from;
                    }
                    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0x80);
                }
                    
                if(mouse_clicked && selected_to && selected_to != selected_from)
                {
                    SDL_RenderCopy(renderer, backbuffer, 0, 0);
                    selected_from = selected_to;
                    dijkstra(selected_from, nodes, node_count);
                }
            }
        }
        else
        {
            puts("Invalid path specified");
        }
    }
    else
    {
        puts("Please specify the file to read the graph from");
    }
    
    return 0;
}
