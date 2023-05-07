add_shader(circle_program, "shaders/circle.glsl");
add_shader(rectangle_program, "shaders/rectangle.glsl");
add_shader(round_line_program, "shaders/round_line.glsl");
add_shader(debug_text_program, "shaders/debug_text.glsl");
add_shader(draw_text_program, "shaders/draw_text.glsl");
add_shader(sprite_program, "shaders/sprite.glsl");
add_shader(ring_program, "shaders/ring.glsl");
add_shader(sphere_program, "shaders/sphere.glsl");
add_shader(line_3d_program, "shaders/line_3D.glsl");
add_shader(fullscreen_texture_program, "shaders/fullscreen_texture.glsl");
add_shader(fullscreen_texture_ssao_program, "shaders/fullscreen_texture_ssao.glsl");
add_shader(render_prepass_program, "shaders/render_prepass.glsl");
add_shader(render_world_program, "shaders/render_world.glsl");
add_shader(render_editor_voxels_program, "shaders/render_editor_voxels.glsl");
add_shader(cast_probes_program, "shaders/cast_probes.glsl");
add_shader(update_lightprobe_color_program, "shaders/update_lightprobe_color.glsl");
add_shader(update_lightprobe_depth_program, "shaders/update_lightprobe_depth.glsl");
add_shader(draw_lightprobes_program, "shaders/draw_lightprobes.glsl");
add_shader(move_lightprobes_program, "shaders/move_lightprobes.glsl");
add_shader(denoise_program, "shaders/denoise.glsl");
add_shader(draw_particles_program, "shaders/draw_particles.glsl");
add_shader(draw_beams_program, "shaders/draw_beams.glsl");
add_shader(cast_rays_program, "shaders/cast_rays.glsl");
add_shader(update_beams_program, "shaders/update_beams.glsl");
add_shader(simulate_chunk_atomic_program, "shaders/simulate_chunk_atomic.glsl");
add_shader(edit_world_program, "shaders/edit_world.glsl");
add_shader(encode_world_cells_program, "shaders/encode_world_cells.glsl");
add_shader(encode_world_cells_scan_program, "shaders/encode_world_cells_scan.glsl");
add_shader(decode_world_cells_scan_program, "shaders/decode_world_cells_scan.glsl");
add_shader(decode_world_cells_program, "shaders/decode_world_cells.glsl");
add_shader(decode_world_cells_write_program, "shaders/decode_world_cells_write.glsl");
add_shader(simulate_particles_program, "shaders/simulate_particles.glsl");
add_shader(simulate_body_program, "shaders/simulate_body.glsl");

#undef add_shader