
enum ObstacleType
{
	obstacle_type_none,
	
	obstacle_type_line,
	obstacle_type_circle,
	obstacle_type_rect,
};

struct ObstacleLine
{
	union
	{
		struct
		{
			V2 p0;
			V2 p1;
		};
		V2 points[2];
		f32 e[4];
	};

	f32 length; // NOTE: just to avoid taking sqrt each time it's needed.
};

struct ObstacleCircle
{
	V2 p;
	f32 r;
};

union ObstacleRect
{
	Rect2 rect;
	struct
	{
		V2 min;
		V2 max;
	};
};

struct Obstacle
{
	ObstacleType type;

	union
	{
		ObstacleLine line;
		ObstacleCircle circle;
		ObstacleRect rect;
	};
	/*
	union
	{
		struct
		{
			V2 p0;
			V2 p1;
		};
		V2 points[2];
		f32 e[4];
	};

	f32 thickness;
	f32 length; // NOTE: just to avoid taking sqrt each time it's needed.
	*/
	f32 thickness;
	
	f32 friction_coefficient;
	f32 body_force_constant;
};
