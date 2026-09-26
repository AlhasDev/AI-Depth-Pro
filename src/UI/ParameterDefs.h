#pragma once

namespace AIDepthPro {
namespace Params {

// Engine & Quality Group
constexpr const char* kGroupEngine = "grp_engine";
constexpr const char* kParamEngine = "engine";
constexpr const char* kParamQuality = "quality";
constexpr const char* kParamAutoMode = "auto_mode";
constexpr const char* kParamModelPath = "model_path";

// Temporal Stability Group
constexpr const char* kGroupTemporal = "grp_temporal";
constexpr const char* kParamTemporalStability = "temporal_stability";
constexpr const char* kParamMotionCompensation = "motion_compensation";
constexpr const char* kParamFlickerReduction = "flicker_reduction";

// Depth Adjustments Group
constexpr const char* kGroupDepthAdjust = "grp_depth_adjust";
constexpr const char* kParamInvertDepth = "invert_depth";
constexpr const char* kParamNearRange = "near_range";
constexpr const char* kParamFarRange = "far_range";
constexpr const char* kParamDepthGamma = "depth_gamma";
constexpr const char* kParamDepthContrast = "depth_contrast";
constexpr const char* kParamDepthOffset = "depth_offset";
constexpr const char* kParamDepthScale = "depth_scale";

// Visualization Group
constexpr const char* kGroupVisualization = "grp_visualization";
constexpr const char* kParamViewMode = "view_mode";
constexpr const char* kParamColorMap = "colormap_type";

// Depth Mask Group
constexpr const char* kGroupMask = "grp_mask";
constexpr const char* kParamEnableMask = "enable_mask";
constexpr const char* kParamMaskMin = "mask_min";
constexpr const char* kParamMaskMax = "mask_max";
constexpr const char* kParamMaskSoftness = "mask_softness";
constexpr const char* kParamMaskFeather = "mask_feather";
constexpr const char* kParamMaskInvert = "mask_invert";

// Depth of Field Group
constexpr const char* kGroupDoF = "grp_dof";
constexpr const char* kParamEnableDoF = "enable_dof";
constexpr const char* kParamFocusPoint = "focus_point";
constexpr const char* kParamFocusDepth = "focus_depth";
constexpr const char* kParamAutoSampleFocus = "auto_sample_focus";
constexpr const char* kParamFocusRange = "focus_range";
constexpr const char* kParamBlurStrength = "blur_strength";
constexpr const char* kParamBokehAmount = "bokeh_amount";
constexpr const char* kParamBokehShape = "bokeh_shape";
constexpr const char* kParamHighlightBoost = "highlight_boost";
constexpr const char* kParamFgBlur = "fg_blur";
constexpr const char* kParamBgBlur = "bg_blur";

// 3D Parallax Group
constexpr const char* kGroupParallax = "grp_parallax";
constexpr const char* kParamEnableParallax = "enable_parallax";
constexpr const char* kParamParallaxX = "parallax_x";
constexpr const char* kParamParallaxY = "parallax_y";
constexpr const char* kParamDepthStrength = "depth_strength";
constexpr const char* kParamCameraDistance = "camera_distance";
constexpr const char* kParamEdgeFill = "edge_fill";

// Depth Zoom Group
constexpr const char* kGroupDepthZoom = "grp_depth_zoom";
constexpr const char* kParamEnableDepthZoom = "enable_depth_zoom";
constexpr const char* kParamNearScale = "near_scale";
constexpr const char* kParamFarScale = "far_scale";
constexpr const char* kParamZoomCenter = "zoom_center";

// Depth Fog Group
constexpr const char* kGroupFog = "grp_fog";
constexpr const char* kParamEnableFog = "enable_fog";
constexpr const char* kParamFogAmount = "fog_amount";
constexpr const char* kParamFogStart = "fog_start";
constexpr const char* kParamFogEnd = "fog_end";
constexpr const char* kParamFogColor = "fog_color";
constexpr const char* kParamFogFalloff = "fog_falloff";

// Relighting Group
constexpr const char* kGroupRelighting = "grp_relighting";
constexpr const char* kParamEnableRelighting = "enable_relighting";
constexpr const char* kParamLightDirX = "light_dir_x";
constexpr const char* kParamLightDirY = "light_dir_y";
constexpr const char* kParamLightHeight = "light_height";
constexpr const char* kParamLightStrength = "light_strength";
constexpr const char* kParamLightSoftness = "light_softness";
constexpr const char* kParamAmbientLight = "ambient_light";

// Edge Refinement Group
constexpr const char* kGroupEdgeRefine = "grp_edge_refine";
constexpr const char* kParamEnableEdgeRefine = "enable_edge_refine";
constexpr const char* kParamEdgeRadius = "edge_radius";
constexpr const char* kParamEdgeEps = "edge_eps";

// Debug / Performance Group
constexpr const char* kGroupDebug = "grp_debug";
constexpr const char* kParamShowPerfOverlay = "show_perf_overlay";
constexpr const char* kParamClearCache = "clear_cache";
constexpr const char* kParamPerfStatsLabel = "perf_stats_label";

} // namespace Params
} // namespace AIDepthPro
