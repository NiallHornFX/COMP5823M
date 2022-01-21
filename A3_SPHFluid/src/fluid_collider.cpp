// COMP5823M - A3 : Niall Horn - fluid_collider.cpp

// Implements
#include "fluid_collider.h"
#include "fluid_object.h"

// ================================== Cloth_Collider Base Class Implementation ===============================
Fluid_Collider::Fluid_Collider(const char *Name)
	: name(Name), eps(1e-02f), friction(0.25f), render(false) {}

// ================================== Cloth_Collider_Plane Implementation ===============================

Fluid_Collider_Plane::Fluid_Collider_Plane(const char *name, const glm::vec3 &Q, const glm::vec3 &Normal, const glm::vec2 &LH)
	: Fluid_Collider(name), q(Q), N(Normal), length(LH.x), height(LH.y)
{
	if (length != 0.f) type = Type::HORIZONTAL; else type = Type::VERTICAL;

	render_setup();
}

// Info : Setup collision render primitive data. 
void Fluid_Collider_Plane::render_setup()
{
	prim = new Primitive((name + "Prim").c_str());

	if (type == Type::HORIZONTAL)
	{
		// Assume q is start position. 
		float x_min = q.x;
		float x_max = q.x + length; 
		float data[12]
		{
			x_min, q.y, 0.f, 0.4f, 0.05f, 0.05f, 
			x_max, q.y, 0.f, 0.4f, 0.05f, 0.05f
		};
		prim->set_data_mesh(data, 2);
		prim->set_shader("../../shaders/collider.vert", "../../shaders/collider.frag");
		prim->mode = Render_Mode::RENDER_LINES;
	}
	else
	{
		float y_min = q.y;
		float y_max = q.y + height;
		float data[12]
		{
			q.x, y_min, 0.f, 0.05f, 0.4f, 0.05f,
			q.x, y_max, 0.f, 0.05f, 0.4f, 0.05f
		};
		prim->set_data_mesh(data, 2);
		prim->set_shader("../../shaders/collider.vert", "../../shaders/collider.frag");
		prim->mode = Render_Mode::RENDER_LINES;
	}
}

// Info : Project positions of particles directly to satisfy the Plane collision inequality condition : (P-Q) * N >= 0
// Within specified bounds (if horizontal or vertical)
void Fluid_Collider_Plane::eval_collision(std::vector<Particle> &particles)
{
	auto vel_decomp = [this](const glm::vec3 &v, float tang_mult, float norm_mult) -> glm::vec3
	{
		// Decompose to tangential and normal components
		glm::vec3 v_N = glm::dot(v, N) * N;
		glm::vec3 v_T = v - v_N;
		return (v_T * tang_mult) + ((-v_N) * norm_mult);
	};
	
	for (Particle &curPt : particles)
	{
		float dist = glm::dot((curPt.P - q), N);
		float eps = 0.1f;

		switch (type) // Account for Plane Bounds over length or height.
		{
			case Type::HORIZONTAL:
			{
				if (curPt.P.x > q.x && curPt.P.x < (q.x + length))
				{
					if (dist <= eps)
					{
						curPt.P += -(dist - eps) * N;
						curPt.V = vel_decomp(curPt.V, 0.8f, 0.4f);
					}
				}
				break;
			}
			case Type::VERTICAL: // Need to add thickness limit on vertical planes so fluid can flow either side. 
			{
				if (curPt.P.y > q.y && curPt.P.y <= (q.y + height))
				{
					if (dist <= eps)
					{
						curPt.P += -(dist - eps) * N;
						curPt.V = vel_decomp(curPt.V, 0.8f, 0.4f);
					}	
				}
				break;
			}
		}
	}
}

