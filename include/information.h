#ifndef INFORMATION
#define INFORMATION

#include <string>
#include <json.hpp>

using json = nlohmann::json;


class AnimationInformation {
public:
	std::string texture;
	std::string textureBlue;
	std::string textureRed;
	int num_frames;
	int size_x;
	int size_y;
	int center_x;
	int center_y;
	int frame_size_x;
	int frame_size_y;
	int frame_origin_x;
	int frame_origin_y;
	double step;
	int ticks_per_frame;
	double length;

	AnimationInformation(){}
	AnimationInformation(json input);
};


class SoldierInformation {
public:
	//general stats
	std::string tag;
	double radius;
	int tilesize;
	double mass;
	double max_speed;
	double acceleration;
	double turn_speed;
	double on_target_dampening;
	int max_hp;
	int armor;
	//melee stats
	bool melee_melee;
	double melee_range;
	double melee_angle;
	int melee_cooldown;
	bool melee_aoe;
	int melee_attack;
	int melee_defense;
	int melee_armor_piercing;
	int melee_damage;
	//ranged stats
	bool ranged_ranged;
	double ranged_range;
	double ranged_min_range;
	double ranged_radius;
	bool ranged_heavy;
	int ranged_draw_timer;
	int ranged_reload_timer;
	double ranged_max_speed_for_firing;
	int ranged_defense;
	double ranged_projectile_speed;
	int ranged_armor_piercing;
	double ranged_ally_protection_aoe;
	double ranged_aoe;
	int projectile_tilesize;
	int ranged_damage;
	//keywords
	bool kw_infantry;
	bool kw_large;
	bool kw_anti_infantry;
	bool kw_anti_large;
	//animation information
	AnimationInformation anime_legs_information;
	AnimationInformation anime_melee_information;
	AnimationInformation anime_ranged_information;
	AnimationInformation anime_body_information;
	AnimationInformation anime_projectile_information;

	SoldierInformation(){}
	SoldierInformation(json input);
};


class UnitInformation {
public:
	//general stats
	std::string tag;
	std::string soldier_type;
	//formation stats
	int formation_max_soldiers;
	int formation_rows;
	int formation_columns;
	double formation_x_spacing;
	double formation_y_spacing;
	//ranged stats
	bool ranged_ranged;
	double ranged_range;
	double ranged_angle;
	bool primary_ranged;

	UnitInformation(json input);
};


class SettingsInformation {
public:
	AnimationInformation damageInfo;
	bool custom_background;
	AnimationInformation backgroundInfo;
	bool show_map_object_outlines;
	bool show_all_unit_orders;
	int turn_duration;
	bool simulation_mode;
	double base_melee_attack;
	double ranged_base_attack;
	double max_hit_chance;
	double min_hit_chance;
	double anti_large_attack_bonus;
	double anti_large_damage_bonus;
	double anti_infantry_attack_bonus;
	double anti_infantry_damage_bonus;
	bool auto_generate_map_grids;
	std::vector<int> map_grids;
	bool set_custom_omp_num_threads;
	int custom_omp_num_threads;
	int player1_type;
	int player2_type;

	SettingsInformation() {}
	SettingsInformation(json input);
};


class MapEditorSettingsInformation {
public:
	bool custom_background;
	AnimationInformation backgroundInfo;

	MapEditorSettingsInformation() {}
	MapEditorSettingsInformation(json input);
};

#endif