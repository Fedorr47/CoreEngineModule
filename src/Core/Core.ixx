export module core;

export import :resource_manager;
export import :asset_manager;
export import :input;
export import :win32_input;
export import :gametimer;
export import :render;
export import :mesh;
export import :obj_loader;
export import :math_utils;

#if defined(CORE_USE_DX12)
export import :render_dx12;
export import :imgui_debug_ui;
#else
export import :render_gl;
#endif