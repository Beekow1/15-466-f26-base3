#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>

#define TileSize 1.0f
#define Rows 6
#define Columns 12

GLuint player_meshes_for_lit_color_texture_program = 0;

Load<MeshBuffer> player_meshes(LoadTagDefault, []() -> MeshBuffer const *
							   {
	MeshBuffer const *ret = new MeshBuffer(data_path("player.pnct"));
	player_meshes_for_lit_color_texture_program =
		ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> player_scene(LoadTagDefault, []() -> Scene const *
						 { return new Scene(data_path("player.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
											{
												 Mesh const &mesh = player_meshes->lookup(mesh_name);

												 scene.drawables.emplace_back(transform);
												 Scene::Drawable &drawable = scene.drawables.back();

												 drawable.pipeline = lit_color_texture_program_pipeline;

												 drawable.pipeline.vao = player_meshes_for_lit_color_texture_program;
												 drawable.pipeline.type = mesh.type;
												 drawable.pipeline.start = mesh.start;
												 drawable.pipeline.count = mesh.count; }); });

Load<Sound::Sample> song_sample(LoadTagDefault, []() -> Sound::Sample const *
								{ return new Sound::Sample(data_path("song.wav")); });
Load<Sound::Sample> hit_sample(LoadTagDefault, []() -> Sound::Sample const *
							   { return new Sound::Sample(data_path("HitSound.wav")); });
Load<Sound::Sample> step_sample(LoadTagDefault, []() -> Sound::Sample const *
								{ return new Sound::Sample(data_path("Step.wav")); });
Load<Sound::Sample> jump_sample(LoadTagDefault, []() -> Sound::Sample const *
								{ return new Sound::Sample(data_path("Jump.wav")); });

GLuint tileset_meshes_for_lit_color_texture_program = 0;

Load<MeshBuffer> tileset_meshes(LoadTagDefault, []() -> MeshBuffer const *
								{
	MeshBuffer const *ret = new MeshBuffer(data_path("tileset.pnct"));

	tileset_meshes_for_lit_color_texture_program =
		ret->make_vao_for_program(lit_color_texture_program->program);

	return ret; });

void PlayMode::load_level(int index)
{
	if (index < 0 || index >= lvl_cnt)
	{
		return;
	}

	int const layouts[lvl_cnt][Rows][Columns] = {
		{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 2, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0},
		 {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
		{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0},
		 {0, 0, 0, 0, 0, 3, 0, 0, 1, 1, 1, 1},
		 {0, 2, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1},
		 {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
		{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0},
		 {0, 2, 3, 0, 0, 1, 1, 0, 0, 0, 3, 0},
		 {1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1}},
		{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0},
		 {0, 0, 0, 0, 0, 0, 3, 1, 0, 0, 0, 3},
		 {0, 2, 3, 0, 0, 1, 1, 1, 0, 0, 1, 1},
		 {1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1}}};

	tiles.clear();
	level.drawables.clear();
	level.transforms.clear();

	current_level = index;

	game_finished = false;

	Mesh const &obstacle_mesh = tileset_meshes->lookup("Base_Tile");

	float tile_scale = TileSize / (obstacle_mesh.max.x - obstacle_mesh.min.x);

	for (int row = 0; row < Rows; ++row)
	{
		for (int column = 0; column < Columns; ++column)
		{
			int cell = layouts[current_level][row][column];

			if (cell == 0)
			{
				continue;
			}

			if (cell == 2)
			{
				body->position = glm::vec3((float(column) - 0.5f * float(Columns - 1)) * TileSize, 0.0f, (float(Rows - 1 - row) + 0.5f) * TileSize);
				continue;
			}

			Mesh const &mesh = tileset_meshes->lookup(
				cell == 3 ? "Enemy" : "Base_Tile");

			level.transforms.emplace_back();
			Scene::Transform &transform = level.transforms.back();

			transform.position = glm::vec3(
				(float(column) - 0.5f * float(Columns - 1)) * TileSize,
				0.0f,
				(0.5f * float(Rows - 1) - float(row)) * TileSize - mesh.min.z * tile_scale);

			transform.scale = glm::vec3(tile_scale);

			level.drawables.emplace_back(&transform);
			Scene::Drawable &drawable = level.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;
			drawable.pipeline.vao = tileset_meshes_for_lit_color_texture_program;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;

			Tile tile;
			tile.bounds = make_aabb(mesh, transform);
			tile.drawable = &drawable;
			tile.target = cell == 3;

			tiles.push_back(tile);
		}
	}

	player_health = 240.0;
	player_velocity = glm::vec2(0.0f);
	grounded = false;
	glm::vec2 center = glm::vec2(body->position.x, body->position.z);

	// create 1x1 aabb
	player_bounds.min = center - glm::vec2(0.5f, 0.5f);
	player_bounds.max = center + glm::vec2(0.5f, 0.5f);
	camera->transform->position = body->position + camera_offset;

	step_timer = 0.0f;

	start_song();
}

PlayMode::PlayMode() : scene(*player_scene)
{
	for (auto &transform : scene.transforms)
	{
		if (transform.name == "Player")
		{
			body = &transform;
		}
	}

	if (body == nullptr)
	{
		throw std::runtime_error("Body not found.");
	}

	body->rotation = glm::angleAxis(
		glm::radians(90.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	camera = &scene.cameras.back();
	camera->fovy = glm::radians(45.0f);

	camera->transform->scale = glm::vec3(1.0f);

	camera->transform->rotation = glm::angleAxis(
		glm::radians(90.0f),
		glm::vec3(1.0f, 0.0f, 0.0f));

	camera->fovy = glm::radians(45.0f);
	camera->transform->position = body->position + camera_offset;

	SDL_SetWindowRelativeMouseMode(Mode::window, false);

	body->scale = glm::vec3(1.0f);

	load_level(0);
}
void PlayMode::start_song()
{
	if (song)
	{
		song->stop(0.0f);
	}

	song_start = double(SDL_GetTicksNS()) * 1.0e-9;
	song = Sound::play(*song_sample);
}
PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{
	if (evt.type == SDL_EVENT_KEY_DOWN)
	{
		if (evt.key.key == SDLK_A)
		{
			left.pressed = true;
			facing = false;
			return true;
		}
		else if (evt.key.key == SDLK_D)
		{
			right.pressed = true;
			facing = true;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			if (!evt.key.repeat && grounded)
			{
				player_velocity.y = 7.0f;
				grounded = false;
				Sound::play(*jump_sample, 0.5f);
			}

			return true;
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_A)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_D)
		{
			right.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (evt.button.button == SDL_BUTTON_LEFT)
		{
			attack(double(evt.button.timestamp) * 1.0e-9);
			return true;
		}
	}

	return false;
}

bool PlayMode::aabb_intersect(AABB const &a, AABB const &b) const
{
	return a.min.x < b.max.x &&
		   a.max.x > b.min.x &&
		   a.min.y < b.max.y &&
		   a.max.y > b.min.y;
}

PlayMode::AABB PlayMode::make_aabb(Mesh const &mesh, Scene::Transform const &transform) const
{
	glm::mat4x3 world_from_local = transform.make_world_from_local();

	AABB bounds;
	bounds.min = glm::vec2(std::numeric_limits<float>::infinity());
	bounds.max = glm::vec2(-std::numeric_limits<float>::infinity());

	glm::vec3 transformed = world_from_local * glm::vec4(mesh.min.x, mesh.min.y, mesh.min.z, 1.0f);
	glm::vec2 point = glm::vec2(transformed.x, transformed.z);

	bounds.min = glm::min(bounds.min, point);
	bounds.max = glm::max(bounds.max, point);

	transformed = world_from_local * glm::vec4(mesh.min.x, mesh.min.y, mesh.max.z, 1.0f);
	point = glm::vec2(transformed.x, transformed.z);

	bounds.min = glm::min(bounds.min, point);
	bounds.max = glm::max(bounds.max, point);

	transformed = world_from_local * glm::vec4(mesh.min.x, mesh.max.y, mesh.min.z, 1.0f);
	point = glm::vec2(transformed.x, transformed.z);

	bounds.min = glm::min(bounds.min, point);
	bounds.max = glm::max(bounds.max, point);

	transformed = world_from_local * glm::vec4(mesh.min.x, mesh.max.y, mesh.max.z, 1.0f);
	point = glm::vec2(transformed.x, transformed.z);

	bounds.min = glm::min(bounds.min, point);
	bounds.max = glm::max(bounds.max, point);

	transformed = world_from_local * glm::vec4(mesh.max.x, mesh.min.y, mesh.min.z, 1.0f);
	point = glm::vec2(transformed.x, transformed.z);

	bounds.min = glm::min(bounds.min, point);
	bounds.max = glm::max(bounds.max, point);

	transformed = world_from_local * glm::vec4(mesh.max.x, mesh.min.y, mesh.max.z, 1.0f);
	point = glm::vec2(transformed.x, transformed.z);

	bounds.min = glm::min(bounds.min, point);
	bounds.max = glm::max(bounds.max, point);

	transformed = world_from_local * glm::vec4(mesh.max.x, mesh.max.y, mesh.min.z, 1.0f);
	point = glm::vec2(transformed.x, transformed.z);

	bounds.min = glm::min(bounds.min, point);
	bounds.max = glm::max(bounds.max, point);

	transformed = world_from_local * glm::vec4(mesh.max.x, mesh.max.y, mesh.max.z, 1.0f);
	point = glm::vec2(transformed.x, transformed.z);

	bounds.min = glm::min(bounds.min, point);
	bounds.max = glm::max(bounds.max, point);

	return bounds;
}

void PlayMode::attack(double time)
{
	double song_time = time - song_start - audio_offset;

	double beat_length = 60.0 / song_bpm;
	long long beat = std::llround((song_time - first_beat) / beat_length);

	double beat_time = first_beat + double(beat) * beat_length;

	glm::vec2 position = glm::vec2(body->position.x, body->position.z);
	Tile *target = nullptr;
	float closest_dist = float(std::numeric_limits<float>::infinity());

	for (Tile &tile : tiles)
	{
		if (!tile.target || !tile.alive)
		{
			continue;
		}
		glm::vec2 enemy_pos = 0.5f * (tile.bounds.min + tile.bounds.max);

		if (facing)
		{
			if (enemy_pos.x <= position.x)
			{
				continue;
			}
		}
		else
		{
			if (enemy_pos.x >= position.x)
			{
				continue;
			}
		}

		float dist = glm::length(position - enemy_pos);
		if (dist <= closest_dist)
		{
			closest_dist = dist;
			target = &tile;
		}
	}

	if (closest_dist > attack_range || target == nullptr)
	{
		return;
	}

	double error = std::abs(song_time - beat_time);

	double perfect_window = beat_length * 0.10;
	double good_window = beat_length * 0.25;
	double okay_window = beat_length * 0.35;

	double accuracy;

	if (error <= perfect_window)
	{
		accuracy = 1.0;
	}
	else if (error <= good_window)
	{
		accuracy = 0.66;
	}
	else if (error <= okay_window)
	{
		accuracy = 0.33;
	}
	else
	{
		accuracy = 0.0;
	}

	if (accuracy < 0.5)
	{

		player_health -= target->damage * (1.0 - accuracy * 2.0);

		if (player_health <= 0.0f)
		{
			load_level(current_level);
		}

		return;
	}

	else
	{
		float damage = glm::clamp(attack_damage * float(accuracy), 0.0f, attack_damage);

		target->health -= damage;
		Sound::play(*hit_sample, 0.7f);

		if (target->health <= 0.0f)
		{
			target->alive = false;
			target->drawable->pipeline.count = 0;
		}
	}
}

void PlayMode::update(float elapsed)
{
	if (game_finished)
	{
		return;
	}
	if (facing)
	{
		body->rotation = glm::quat(glm::vec3(0.0f, 0.0f, glm::radians(90.0f)));
	}
	else
	{
		body->rotation = glm::quat(glm::vec3(0.0f, 0.0f, glm::radians(-90.0f)));
	}

	bool end_lvl = true;

	for (Tile const &tile : tiles)
	{
		if (tile.target && tile.alive)
		{
			end_lvl = false;
			break;
		}
	}

	if (end_lvl)
	{
		if (current_level + 1 < lvl_cnt)
		{
			load_level(current_level + 1);
		}
		else
		{
			game_finished = true;
			player_velocity = glm::vec2(0.0f);
			grounded = false;

			if (song)
			{
				song->stop();
			}
		}

		return;
	}

	player_velocity.x = 0.0f;
	if (right.pressed)
	{
		player_velocity.x += move_speed;
	}
	if (left.pressed)
	{
		player_velocity.x -= move_speed;
	}

	float dt = glm::min(elapsed, 0.1f);
	player_velocity.y = glm::max(player_velocity.y - 18.0f * dt, -15.0f);

	// Keep the movement direction even after a collision zeros the velocity.
	glm::vec2 const movement = player_velocity * dt;
	body->position.x += movement.x;
	glm::vec2 center = glm::vec2(body->position.x, body->position.z);
	player_bounds.min = center - glm::vec2(0.5f);
	player_bounds.max = center + glm::vec2(0.5f);

	for (Tile const &tile : tiles)
	{
		if (!tile.alive || !aabb_intersect(player_bounds, tile.bounds))
		{
			continue;
		}
		player_velocity.x = 0.0f;
		if (movement.x > 0.0f)
		{
			body->position.x = tile.bounds.min.x - 0.5f;
		}
		else
		{
			body->position.x = tile.bounds.max.x + 0.5f;
		}

		center = glm::vec2(body->position.x, body->position.z);
		player_bounds.min = center - glm::vec2(0.5f);
		player_bounds.max = center + glm::vec2(0.5f);
	}

	grounded = false;

	body->position.z += movement.y;

	// recalc bounds for next collision check
	center = glm::vec2(body->position.x, body->position.z);
	player_bounds.min = center - glm::vec2(0.5f);
	player_bounds.max = center + glm::vec2(0.5f);

	for (Tile const &tile : tiles)
	{
		if (!tile.alive || !aabb_intersect(player_bounds, tile.bounds))
		{
			continue;
		}

		player_velocity.y = 0.0f;
		if (movement.y > 0.0f)
		{
			body->position.z = tile.bounds.min.y - 0.5f;
		}
		else
		{
			body->position.z = tile.bounds.max.y + 0.5f;
			grounded = true;
		}

		center = glm::vec2(body->position.x, body->position.z);
		player_bounds.min = center - glm::vec2(0.5f);
		player_bounds.max = center + glm::vec2(0.5f);
	}

	if (grounded && player_velocity.x != 0.0f)
	{
		step_timer -= dt;

		if (step_timer <= 0.0f)
		{
			Sound::play(*step_sample, 0.4f);
			step_timer += step_interval;
		}
	}
	else
	{
		step_timer = 0.0f;
	}

	if (body->position.z < -10.0f)
	{
		load_level(current_level);
	}

	camera->transform->position = body->position + camera_offset;
	Sound::listener.set_position_right(
		camera->transform->position, glm::vec3(1.0f, 0.0f, 0.0f), elapsed);
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	// update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	// set up light type and position for lit_color_texture_program:
	//  TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);
	level.draw(*camera);

	{ // use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f));

		constexpr float H = 0.09f;
		lines.draw_text("Use A + D to move, Space to jump, Left Click to attack.",
						glm::vec3(-aspect + 0.1f * H, -1.0 + 1.1f * H, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Time your attacks to the music. If you miss, you take damage.",
						glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}
