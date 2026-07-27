export module core;

export import :resource_manager;
export import :asset_manager;
export import :input;
export import :input_core;
export import :controller_base;
export import :win32_input;
export import :gametimer;
export import :render;
export import :thread_affinity;
export import :mesh;
export import :skeleton;
export import :animation_clip;
export import :animator;
export import :animation_controller;
export import :skinned_mesh;
export import :obj_loader;
export import :math_utils;
export import :geometry;
export import :EnTTHelpers;
export import :gameplay;
export import :gameplay_interaction_point;
export import :gameplay_object_reservation_system;
export import :ai_system;
export import :ai_agent_world_state;
export import :ai_action_runtime;
export import :ai_action_task;
export import :ai_follow_route_action_runtime;
export import :ai_follow_route_action;
export import :ai_move_to_action;
export import :gameplay_request;
export import :gameplay_steering;
export import :gameplay_route_search;
export import :gameplay_graph_assets;
export import :ai_decision_contracts;
export import :gameplay_route;
export import :gameplay_route_follower;
export import :gameplay_graph;
export import :gameplay_runtime;
export import :gameplay_ai_movement_development_scenario;
export import :json_utils;
export import :hash_utils;
export import :string_utils;
export import :assimp_loader;
export import :editor_selection_service;
export import :editor_commands;
export import :character_controller;
export import :character_movement;

#if defined(CORE_USE_DX12)
export import :render_dx12;
export import :imgui_debug_ui;
#else
export import :render_gl;
#endif