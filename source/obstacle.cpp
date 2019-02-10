
void add_obstacle_line(SimStorage *storage, V2 p0, V2 p1)
{
	if(storage->obstacle_count < MAX_OBSTACLE_COUNT)
	{
	    Obstacle *obstacle = storage->obstacles + storage->obstacle_count++;
		
		obstacle->type = obstacle_type_line;
		
	    obstacle->line.p0 = p0;
		obstacle->line.p1 = p1;

		obstacle->line.length = length(p1 - p0);
		
		obstacle->thickness = DEFAULT_OBSTACLE_THICKNESS;
		obstacle->friction_coefficient = DEFAULT_OBSTACLE_FRICTION_COEFFICIENT;
		obstacle->body_force_constant = DEFAULT_OBSTACLE_BODY_FORCE_CONSTANT;
	}
	else
	{
		crash();
	}
}

void add_obstacle_circle(SimStorage *storage, V2 p, f32 r)
{
	if(storage->obstacle_count < MAX_OBSTACLE_COUNT)
	{
	    Obstacle *obstacle = storage->obstacles + storage->obstacle_count++;
		
		obstacle->type = obstacle_type_circle;
		
	    obstacle->circle.p = p;
		obstacle->circle.r = r;
		
		obstacle->thickness = DEFAULT_OBSTACLE_THICKNESS;
		obstacle->friction_coefficient = DEFAULT_OBSTACLE_FRICTION_COEFFICIENT;
		obstacle->body_force_constant = DEFAULT_OBSTACLE_BODY_FORCE_CONSTANT;
	}
	else
	{
		crash();
	}
}

void add_obstacle_rect(SimStorage *storage, V2 min, V2 max)
{
	if(storage->obstacle_count < MAX_OBSTACLE_COUNT)
	{
	    Obstacle *obstacle = storage->obstacles + storage->obstacle_count++;
		
		obstacle->type = obstacle_type_rect;
		
	    obstacle->rect.min = min;
		obstacle->rect.max = max;
		
		obstacle->thickness = DEFAULT_OBSTACLE_THICKNESS;
		obstacle->friction_coefficient = DEFAULT_OBSTACLE_FRICTION_COEFFICIENT;
		obstacle->body_force_constant = DEFAULT_OBSTACLE_BODY_FORCE_CONSTANT;
	}
	else
	{
		crash();
	}
}

bool intersect(Obstacle *obstacle, V2 p00, V2 p01)
{
	bool result = false;

	switch(obstacle->type)
	{
		case obstacle_type_line:
		{
			f32 l0_norm_x = p00.y - p01.y;
			f32 l0_norm_y = p01.x - p00.x;
	
			f32 v0_x = obstacle->line.p0.x - p00.x;
			f32 v0_y = obstacle->line.p0.y - p00.y;
			f32 v1_x = obstacle->line.p1.x - p00.x;
			f32 v1_y = obstacle->line.p1.y - p00.y;

			f32 dot00 = v0_x * l0_norm_x + v0_y * l0_norm_y;
			f32 dot01 = v1_x * l0_norm_x + v1_y * l0_norm_y;

			if(dot00 * dot01 <= 0)
			{
				f32 l1_norm_x = obstacle->line.p0.y - obstacle->line.p1.y;
				f32 l1_norm_y = obstacle->line.p1.x - obstacle->line.p0.x;

				f32 v2_x = obstacle->line.p0.x - p01.x;
				f32 v2_y = obstacle->line.p0.y - p01.y;

				f32 dot10 = v0_x * l1_norm_x + v0_y * l1_norm_y;
				f32 dot11 = v2_x * l1_norm_x + v2_y * l1_norm_y;
		
				result = dot10 * dot11 <= 0;
			}
			break;
		}
		case obstacle_type_circle:
		{
			// very bad and slow
			V2 line = p01 - p00;
			f32 line_length = length(line);
			V2 line_n = line / line_length;
			f32 t = clamp(0, dot(obstacle->circle.p - p00, line_n), line_length);
			V2 closest_point = p00 + t * line_n;
		    result = length_squared(closest_point - obstacle->circle.p) <= squared(obstacle->circle.r);
			break;
		}
		case obstacle_type_rect:
        // thanks https://tavianator.com/fast-branchless-raybounding-box-intersections/
		{
			V2 line = p01 - p00;
			f32 line_length = length(line);
			f32 inv_dx = line_length/line.x;
			f32 inv_dy = line_length/line.y;
			
			f32 tx0 = inv_dx * (obstacle->rect.min.x - p00.x);
			f32 tx1 = inv_dx * (obstacle->rect.max.x - p00.x);

			f32 tx_min, tx_max;
			if(tx0 < tx1)
			{
				tx_min = tx0;
				tx_max = tx1;
			}
			else
			{
				tx_min = tx1;
				tx_max = tx0;
			}

			f32 ty0 = inv_dy * (obstacle->rect.min.y - p00.y);
			f32 ty1 = inv_dy * (obstacle->rect.max.y - p00.y);
			
			f32 ty_min, ty_max;
			if(ty0 < ty1)
			{
				ty_min = ty0;
				ty_max = ty1;
			}
			else
			{
				ty_min = ty1;
				ty_max = ty0;
			}

			// we choose the furthest min and the closest max
			f32 t0 = tx_min > ty_min ? tx_min : ty_min;
			f32 t1 = tx_max < ty_max ? tx_max : ty_max;

			result = t0 >= 0 && t0 <= line_length && t0 <= t1;

			break;
		}
	}
	
	return result;
}

inline V2 get_closest_point(Obstacle *obstacle, V2 p)
{
	V2 result;

	switch(obstacle->type)
	{
		case obstacle_type_line:
		{
			V2 line_n = (obstacle->line.p1 - obstacle->line.p0)/obstacle->line.length;
			f32 t = clamp(0, dot(p - obstacle->line.p0, line_n), obstacle->line.length);
		    result = obstacle->line.p0 + t * line_n;
			break;
		}
		case obstacle_type_circle:
		{
			V2 line = p - obstacle->circle.p;
			f32 distance = length(line);
			if(distance > obstacle->circle.r)
			{
				result = obstacle->circle.p + obstacle->circle.r * line / distance;
			}
			else
			{
			    result = -(obstacle->circle.p + obstacle->circle.r * line / distance);
			}
			break;
		}
		case obstacle_type_rect:
		{
			if(p.x > obstacle->rect.max.x)
			{
				if(p.y > obstacle->rect.max.y)
				{
					// upper right corner
					result = obstacle->rect.max;
				}
				else if(p.y < obstacle->rect.min.y)
				{
					// lower right corner
					result = v2(obstacle->rect.max.x, obstacle->rect.min.y);
				}
				else
				{
					// strictly to the right
					result = v2(obstacle->rect.max.x, p.y);
				}
			}
			else if(p.x < obstacle->rect.min.x)
			{
				if(p.y > obstacle->rect.max.y)
				{
					// upper left corner
					result = v2(obstacle->rect.min.x, obstacle->rect.max.y);
				}
				else if(p.y < obstacle->rect.min.y)
				{
					// lower left corner
					result = obstacle->rect.min;
				}
				else
				{
					// strictly to the left
					result = v2(obstacle->rect.min.x, p.y);
				}
			}
			else if(p.y > obstacle->rect.max.y)
			{
				// strictly above
				result = v2(p.x, obstacle->rect.max.y);
			}
			else if(p.y < obstacle->rect.min.y)
			{
				// strictly below
				result = v2(p.x, obstacle->rect.min.y);
			}
			else
			{
				// inside
				// WHAT DO WE DO?
				result = p;
			}
			break;
		}
		default:
		{
			result = v2();
			crash();
		}
	};

	return result;
}

void draw_obstacle(RenderInfo *render_info, Obstacle *obstacle)
{
	switch(obstacle->type)
	{
		case obstacle_type_line:
		{
			push_line(render_info, obstacle->line.p0, obstacle->line.p1, obstacle->thickness,
					  color_white, rendering_priority_sim_base);
			break;
		}
		case obstacle_type_circle:
		{
			push_disk(render_info, obstacle->circle.p, obstacle->circle.r, obstacle->thickness,
					  color_white, rendering_priority_sim_base);
			break;
		}
		case obstacle_type_rect:
		{
			push_rect_outline(render_info, obstacle->rect.rect, obstacle->thickness,
							  color_white, rendering_priority_sim_base);
			break;
		}
	};
}
