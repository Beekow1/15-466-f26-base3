#include "Mode.hpp"
#include "Mesh.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	// input tracking:
	struct Button
	{
		uint8_t pressed = 0;
	} left, right;

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;
	Scene level;

	Scene::Transform *body = nullptr;

	struct AABB
	{
		glm::vec2 min;
		glm::vec2 max;
	};

	bool aabb_intersect(AABB const &a, AABB const &b) const;
	AABB make_aabb(Mesh const &mesh, Scene::Transform const &transform) const;

	struct Tile
	{
		AABB bounds;
		Scene::Drawable *drawable = nullptr;
		bool target = false;
		bool alive = true;
		float health = 200.0f;
		float damage = 100.0f;
	};

	std::vector<Tile> tiles;

	AABB player_bounds;
	glm::vec2 player_velocity = glm::vec2(0.0f);
	float move_speed = 4.0f;
	bool grounded = false;
	bool facing = false;
	double player_health = 240.0;

	void load_level(int index);

	static constexpr int lvl_cnt = 4;
	int current_level = 0;

	void start_song();
	void attack(double time);

	std::shared_ptr<Sound::PlayingSample> song;

	double song_bpm = 120.0;
	double first_beat = 0.0;
	double audio_offset = 0.0;
	double song_start = 0.0;
	double song_duration = 0.0;

	float attack_range = 1.5f;
	float attack_damage = 100.0f;

	bool game_finished = false;

	float step_timer = 0.0f;
	float step_interval = 0.3f;

	// camera:
	Scene::Camera *camera = nullptr;
	glm::vec3 camera_offset = glm::vec3(0.0f, -12.0f, 2.0f);
};
