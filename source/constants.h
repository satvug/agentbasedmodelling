
// Agent params
#define DEFAULT_AGENT_DESIRED_VELOCITY                                 1.5f
#define DEFAULT_AGENT_RELAXATION_TIME                                0.150f
#define DEFAULT_AGENT_RADIUS                                         0.225f
#define DEFAULT_AGENT_PRIVATE_ZONE_RADIUS                            0.320f
#define DEFAULT_AGENT_REPULSION_COEFFICIENT                           5.00f
#define DEFAULT_AGENT_FALLOFF_LENGTH                                 0.400f
#define DEFAULT_AGENT_BODY_FORCE_CONSTANT                           900.00f
#define DEFAULT_AGENT_FRICTION_COEFFICIENT                          400.00f
#define DEFAULT_AGENT_RANDOM_FORCE                                   0.100f
//#define DEFAULT_AGENT_RANDOM_FORCE                                  10.00f
// NOTE: in seconds
#define DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE                         0.5f
#define DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL                         1.0f
#define DEFAULT_AGENT_COLLISION_AVOIDANCE                             0.25f
// World params
#define DEFAULT_AGENT_COUNT                                             100
#define DEFAULT_REPEAT_COUNT 1
#define DEFAULT_OBSTACLE_THICKNESS                                   0.040f
#define DEFAULT_OBSTACLE_BODY_FORCE_CONSTANT                       3000.00f
#define DEFAULT_OBSTACLE_FRICTION_COEFFICIENT                       700.00f
#define MAX_OBSTACLE_COUNT                                             4096
#define MAX_AGENT_COUNT                                                4096
// Just an array of run "prototypes"
#define MAX_RUN_COUNT                                                100000

#define SIM_GRID_DIM                                                      7
#define AGENT_ID_HASH_SIZE                                              256
#define MAX_MID_WAYPOINT_COUNT                                          256
#define MAX_TARGET_WAYPOINT_COUNT                                        16
#define MAX_SINK_RECT_COUNT                                              16
#define MAX_WAYPOINT_COUNT           MAX_MID_WAYPOINT_COUNT + MAX_TARGET_WAYPOINT_COUNT
// Rendering -- system
#define MAX_RENDER_VERTEX_COUNT                                                 U16_MAX
#define MAX_RENDER_ENTRY_COUNT                           (MAX_RENDER_VERTEX_COUNT) >> 2
#define MAX_RENDER_INDEX_COUNT                             (MAX_RENDER_ENTRY_COUNT) * 6
// Rendering -- drawing params
#define DEFAULT_ASPECT_RATIO                                               16.0f / 9.0f
#define DEFAULT_VIEW_HEIGHT                                                       10.0f
#define DEBUG_LINE_THICKNESS                                                      0.03f
// Sim params
#define DEFAULT_TARGET_AGENT_COUNT                                                  100
#define MAX_RUN_STATS_COUNT                                                      200000
#define MIN_FRAME_TIME                                                           0.008f
#define DEFAULT_FRAME_TIME                                                       0.016f
#define MAX_FRAME_TIME                                                           0.033f
// Memory
#define PERSISTENT_MEMORY_SIZE                                         16 * 1024 * 1024
#define RENDER_MEMORY_SIZE                                             16 * 1024 * 1024
#define SCRATCH_MEMORY_SIZE                                           128 * 1024 * 1024
#define STATS_MEMORY_SIZE                                             128 * 1024 * 1024
