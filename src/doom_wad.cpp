#ifndef DOOM_WAD
#define DOOM_WAD

#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include "gl_helper.cpp"
#include "space.cpp"

struct WadFuncs {
	char* lump_data;
	int lump_size;
	
	// Find these lumps in the directory in order. For example, { "MAP01", "VERTEXES" } will search
	// from the top of the wad directory for the VERTEXES lump of MAP01 and store the data and size
	// of VERTEXES.
	bool StoreLump (DoomWad& wad, std::vector <std::string> lump_order) {
		wad.BeginLumpSearch();
		
		for(int k = 0; k < lump_order.size(); k++) {
			const auto& lump_name = lump_order [k];
			
			if(!wad.FindLump(lump_name)) {
				return false;
			}
			
			// The last will be stored.
			else if(k == lump_order.size() - 1) {
				lump_data = &wad.data [wad.current_lump->offset];
				lump_size = wad.current_lump->size;
			}
		}
		
		return true;
	}
	
	// Convert VERTEXES lump into a list of float positions. Vanilla DOOM format, so it's short
	// to float conversion, particularly good for open-gl.
	std::vector <float> VanillaVertexesLumpToFloat () {
		
		struct vertexes_entry {
			short x;
			short y;
		};
		
		int count = lump_size / sizeof(vertexes_entry);
		int coord_count = count + count;
		
		// Iterate it and fill this vector in.
		vertexes_entry* current_vertex = reinterpret_cast <vertexes_entry*> (lump_data);
		std::vector <float> float_xy(coord_count);
		
		for(int k = 0; k < coord_count; k += 2) {
			float_xy [0 + k] = (float)(current_vertex->x);
			float_xy [1 + k] = (float)(current_vertex->y);
			current_vertex++;
		}
		
		return float_xy;
	}
	
	// Convert a map THINGS lump into a vector of { x pos, y pos, spawn angle radians } triplets.
	std::vector <float> VanillaThingsLumpToFloat () {
		struct things_entry {
			short x;
			short y;
			unsigned short byte_angle;
			unsigned short thing_enum;
			unsigned short spawn_flags;
		};
		
		int count = lump_size / sizeof(things_entry);
		int info_count = 3 * count; // x, y and radiant
		
		things_entry* current_thing = reinterpret_cast <things_entry*> (lump_data);
		std::vector <float> float_thing(info_count);
		
		for(int k = 0; k < info_count; k += 3) {
			float_thing [0 + k] = (float)(current_thing->x);
			float_thing [1 + k] = (float)(current_thing->y);
			float_thing [2 + k] = (float)(current_thing->byte_angle) * SpaceConst::Pi() / 128;
			current_thing++;
		}
		
		return float_thing;
	}
	
	// Convert a map LINEDEFS lump into a vector of indices into VERTEXES lump vertices.
	std::vector <int> VanillaLinedefsLumpToVertexIndices () {
		struct linedefs_entry {
			unsigned short vert_a;
			unsigned short vert_b;
			unsigned short flags;
			unsigned short line_enum;
			unsigned short sector_tag;
			unsigned short sidedef_a;
			unsigned short sidedef_b;
		};
		
		linedefs_entry* current_linedef = reinterpret_cast <linedefs_entry*> (lump_data);
		int count = lump_size / sizeof(linedefs_entry);
		int index_count = count + count;
		std::vector <int> indices(index_count);
		
		for(int k = 0; k < index_count; k += 2) {
			indices [0 + k] = current_linedef->vert_a;
			indices [1 + k] = current_linedef->vert_b;
			current_linedef++;
		}
		
		return indices;
	}
	
	// Convert an index into a DOOM2 compatible map lump name ("MAP01" to "MAP32" inclusive).
	void Doom2MapLumpName (std::string& buffer, int zero_based_map_index) {
		static constexpr int map_count = 32;
		zero_based_map_index %= map_count;
		
		if(zero_based_map_index < 0) {
			zero_based_map_index += map_count;
		}
		
		int map_num = 1 + zero_based_map_index;
		buffer = "MAP01";
		
		buffer [3] = '0' + (map_num / 10);
		buffer [4] = '0' + (map_num % 10);
	}
	
	void UltimateDoomMapLumpName (std::string& buffer, int zero_based_episode_index, int zero_based_map_index) {
		static constexpr int episode_count = 4;
		static constexpr int map_count = 9;
		
		zero_based_episode_index %= episode_count;
		zero_based_map_index %= map_count;
		
		if(zero_based_episode_index < 0) {
			zero_based_episode_index += episode_count;
		}
		
		if(zero_based_map_index < 0) {
			zero_based_map_index += map_count;
		}
		
		int episode_num = 1 + zero_based_episode_index;
		int map_num = 1 + zero_based_map_index;
		
		buffer = "E1M1";
		buffer [1] = '0' + (episode_num % 10);
		buffer [3] = '0' + (map_num % 10);
	}
};

struct WadAppData {
	
	// Agnostic data.
	int window_x_size;
	int window_y_size;
	int exit_pressed;
	int timer;
	
	double cursor_x_pos;
	double cursor_y_pos;
	double cursor_dx;
	double cursor_dy;
	float map_x_pos;
	float map_y_pos;
	float zoom_f;
	float zoom_target_f;
	float map_rotation_rad_target;
	float map_rotation_rad;
	
	// Wad data.
	DoomWad wad;
	int vertex_count;
	int linedef_count;
	int thing_count;
	int display_timer;
	int wad_map_index;
	
	std::vector <float> map_vertices;
	std::string open_wad_path;
	
	// Open-gl rendering.
	GlFuncs gl_funcs;
	GlModelFuncs gl_model_funcs;
	WadFuncs wad_funcs;
	
	int map_draw_program;
	int bar_draw_program;
	int grid_draw_program;
	
	// command line args
	int cmd_arg_count;
	char** cmd_args;
	
	// GLFW data.
	GLFWwindow* window;
};

struct WadApp {
	WadApp (WadAppData& app_data) : d(app_data) {}
	
	// Open a DOOM wad file for display. This resets the display.
	void OpenWad (std::string path) {
		d.wad_map_index = 0;
		d.open_wad_path = path;
		d.wad = DoomWad(d.open_wad_path);
		d.display_timer = d.wad.IsLoaded() ? 0 : -1;
	}
	
	void OnFirstTick () {
		
		// User settings.
		d.window_x_size = 1280;
		d.window_y_size = 720;
		d.zoom_f = 1.0;
		d.zoom_target_f = d.zoom_f;
		d.map_rotation_rad_target = 0;
		d.map_rotation_rad = 0;
		d.display_timer = -1;
		
		// Initialise GLFW for rendering and viewing.
		glfwInit();
		glfwDefaultWindowHints();
		d.window = glfwCreateWindow(d.window_x_size, d.window_y_size, "Map Viewer", nullptr, nullptr);
		glfwMakeContextCurrent(d.window);
		
		glfwSetWindowUserPointer(d.window, this);
		
		glfwSetScrollCallback(d.window, [] (GLFWwindow* window, double scroll_x, double scroll_y) {
			auto* app = reinterpret_cast <WadApp*> (glfwGetWindowUserPointer(window));
			app->d.zoom_target_f = std::clamp(app->d.zoom_target_f + scroll_y / 15, 0.25, 4.0);
			
		});
		
		glfwSetDropCallback(d.window, [] (GLFWwindow* window, int path_count, const char* paths []) {
			auto* app = reinterpret_cast <WadApp*> (glfwGetWindowUserPointer(window));
			app->OpenWad(paths [0]);
		});
		
		// Ensure the viewport resizes with the window.
		glfwSetFramebufferSizeCallback(d.window, [] (GLFWwindow* window, int x_size, int y_size) {
			glViewport(0, 0, x_size, y_size);
		});
		
		glfwGetCursorPos(d.window, &d.cursor_x_pos, &d.cursor_y_pos);
		d.cursor_dx = 0;
		d.cursor_dy = 0;
		
		glfwGetWindowSize(d.window, &d.window_x_size, &d.window_y_size);
		
		// Rendering setup for open-gl.
		gladLoadGL();
		glDisable(GL_LINE_SMOOTH);
		glPointSize(3);
		glLineWidth(1);
		
		d.map_draw_program = d.gl_funcs.LoadProgram( {
			{ "shader_map_vertex.txt", GL_VERTEX_SHADER },
			{ "shader_map_fragment.txt", GL_FRAGMENT_SHADER }
		});
		
		d.bar_draw_program = d.gl_funcs.LoadProgram( {
			{ "shader_zoom_bar_vertex.txt", GL_VERTEX_SHADER },
			{ "shader_zoom_bar_fragment.txt", GL_FRAGMENT_SHADER }
		});
		
		d.grid_draw_program = d.gl_funcs.LoadProgram( {
			{ "shader_map_grid_vertex.txt", GL_VERTEX_SHADER },
			{ "shader_map_grid_fragment.txt", GL_FRAGMENT_SHADER }
		});
		
		// If there is a command line argument, try opening it.
		if(2 <= d.cmd_arg_count) {
			OpenWad(d.cmd_args [1]);
			
			if(d.wad.IsLoaded() && 3 <= d.cmd_arg_count) {
				d.wad_map_index = std::stoi(d.cmd_args [2]);
			}
		}
	}
	
	void OnFirstMapTick () {
		
		d.zoom_f = 1.0;
		d.zoom_target_f = d.zoom_f;
		
		std::string current_map;
		d.wad_funcs.Doom2MapLumpName(current_map, d.wad_map_index);
		
		// Open up a DOOM wad and see what's inside.
		d.wad.BeginLumpSearch();
		d.wad.FindLump(current_map);
		
		if(!d.wad.LumpExists()) {
			d.display_timer = -1;
			return;
		}
		
		d.wad_funcs.StoreLump(d.wad, { current_map, "VERTEXES" });
		d.map_vertices = d.wad_funcs.VanillaVertexesLumpToFloat();
		
		d.wad_funcs.StoreLump(d.wad, { current_map, "LINEDEFS" });
		auto indices = d.wad_funcs.VanillaLinedefsLumpToVertexIndices();
		
		d.wad_funcs.StoreLump(d.wad, { current_map, "THINGS" });
		auto things = d.wad_funcs.VanillaThingsLumpToFloat();
				
		glBindBuffer(GL_ARRAY_BUFFER, 1);
		glBufferData(GL_ARRAY_BUFFER, d.map_vertices.size() * sizeof(float), d.map_vertices.data(), GL_STATIC_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 2);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices [0]), indices.data(), GL_STATIC_DRAW);
		
		glBindBuffer(GL_ARRAY_BUFFER, 3);
		glBufferData(GL_ARRAY_BUFFER, things.size() * sizeof(float), things.data(), GL_STATIC_DRAW);
				
		d.vertex_count = indices.size();
		d.thing_count = things.size();
		
		std::vector <float> quad;
		d.gl_model_funcs.Make2dQuadTris(quad);
		glBindBuffer(GL_ARRAY_BUFFER, 200);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * quad.size(), quad.data(), GL_STATIC_DRAW);
		
		// Full screen quad for the background grid.
		std::vector <float> screen_quad;
		d.gl_model_funcs.Make2dQuadTris(screen_quad);
		d.gl_model_funcs.Rescale2dModel(screen_quad, 2, 0.5, 0.5);
		glBindBuffer(GL_ARRAY_BUFFER, 201);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * screen_quad.size(), screen_quad.data(), GL_STATIC_DRAW);
		
		// Arrows for things.
		std::vector <float> arrow;
		d.gl_model_funcs.Make2dArrow(arrow);
		d.gl_model_funcs.Rescale2dModel(arrow, 1, 0.5, 0.5);
		glBindBuffer(GL_ARRAY_BUFFER, 202);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * arrow.size(), arrow.data(), GL_STATIC_DRAW);
	}
	
	void OnLastTick () {
		glfwTerminate();
	}
	
	void OnTick () {
		if(0 == d.display_timer) {
			OnFirstMapTick();
		}
		
		auto old_cursor_x_pos = d.cursor_x_pos;
		auto old_cursor_y_pos = d.cursor_y_pos;
		
		glfwGetCursorPos(d.window, &d.cursor_x_pos, &d.cursor_y_pos);
		
		d.cursor_dx = d.cursor_x_pos - old_cursor_x_pos;
		d.cursor_dy = d.cursor_y_pos - old_cursor_y_pos;
		
		float zoom_lerp_f = 0.25;
		d.zoom_f = zoom_lerp_f * d.zoom_target_f + (1 - zoom_lerp_f) * d.zoom_f;
		
		float rot_lerp_f = 0.25;
		d.map_rotation_rad = rot_lerp_f * d.map_rotation_rad_target + (1 - rot_lerp_f) * d.map_rotation_rad;
		
		auto map_scale = 1.0 / 2000 * d.zoom_f;
		
		// Handle mouse button inputs. Here is panning.
		if(glfwGetMouseButton(d.window, 0)) {
			d.map_x_pos -= d.cursor_dx * d.zoom_f;
			d.map_y_pos += d.cursor_dy * d.zoom_f;
		}
		
		// Mouse input for rotation.
		if(glfwGetMouseButton(d.window, 1)) {
			d.map_rotation_rad_target += d.cursor_dx * 0.005;
		}
		
		// Shader inputs to scale the map lines and grid.
		auto zoom_unit_f = (d.zoom_f - 0.25) / (4.0 - 0.25);
		auto aspect_ratio_f = 1.0 * d.window_x_size / d.window_y_size;
		
		glUseProgram(d.map_draw_program);
		glUniform2f(0, (float)d.map_x_pos, (float)d.map_y_pos);
		// glUniform2f(0, 0, 0);
		glUniform1f(1, map_scale);
		glUniform1f(2, 0);
		glUniform1f(3, aspect_ratio_f);
		glUniform1f(4, d.map_rotation_rad);
		
		glUseProgram(d.bar_draw_program);
		glUniform1f(0, zoom_unit_f);
		glUniform4f(1, 1, 0, 0, 1);
		
		glUseProgram(d.grid_draw_program);
		glUniform1f(2, map_scale);
		glUniform1f(3, aspect_ratio_f);
		glUniform2f(4, (float)d.map_x_pos, (float)d.map_y_pos);
		glUniform1f(5, d.map_rotation_rad);
		
		auto normal_cursor_x = d.cursor_x_pos / d.window_x_size;
		auto normal_cursor_y = d.cursor_y_pos / d.window_y_size;
		
		auto window_mid_x = d.window_x_size / 2;
		auto window_mid_y = d.window_y_size / 2;
		
		auto cursor_map_x =  (d.cursor_x_pos - window_mid_x) / map_scale / d.window_x_size * 2;
		auto cursor_map_y = -(d.cursor_y_pos - window_mid_y) / map_scale / d.window_y_size * 2;
		
		if(0 <= d.display_timer) {
			/* Vec2f p = Vec2f(cursor_map_x, cursor_map_y) + 3 * RadVec2 <float> (0.02 * d.timer);
			glBindBuffer(GL_ARRAY_BUFFER, 1);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(p), (void*)&p); */
		
			d.display_timer++;
		}
	}
	
	void OnDraw () {
		if(0 <= d.display_timer) {
			OnDrawMap();
		}
	}
	
	void OnDrawMap () {
		
		// Draw the background grid.
		glUseProgram(d.grid_draw_program);
		glBindBuffer(GL_ARRAY_BUFFER, 201);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		// Draw the map lines and vertices.
		glUseProgram(d.map_draw_program);
		glBindBuffer(GL_ARRAY_BUFFER, 1);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 2);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		glDrawElements(GL_LINES, d.vertex_count, GL_UNSIGNED_INT, nullptr);
		glDrawElements(GL_POINTS, d.vertex_count, GL_UNSIGNED_INT, nullptr);
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
		// Draw the thing dots.
		glBindBuffer(GL_ARRAY_BUFFER, 3);
		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 3);
		// glBindBuffer(GL_ARRAY_BUFFER, 202);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(sizeof(float)) );
		glDrawArrays(GL_POINTS, 0, d.thing_count);
		// glDrawElements(GL_LINES, d.thing_count, GL_FLOAT, nullptr);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		// Draw the zoom bar.
		glUseProgram(d.bar_draw_program);
		glBindBuffer(GL_ARRAY_BUFFER, 200);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	int MainLoop () {
		d.timer = 0;
		d.exit_pressed = 0;
		
		while(d.exit_pressed < 1) {
			glfwPollEvents();
			
			if(glfwWindowShouldClose(d.window)) {
				d.exit_pressed++;
			}
			
			if(0 == d.timer) {
				OnFirstTick();
			}
			
			OnTick();
			
			glClearColor(0.343, 0.03030, 0.143, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			OnDraw();
			glfwSwapBuffers(d.window);
			d.timer++;
		}
		
		OnLastTick();
		
		return 0;
	}
	
	WadAppData& d;
};

int main (int argc, char * argv []) {
	WadAppData app_data;
	app_data.cmd_arg_count		= argc;
	app_data.cmd_args			= argv;
	
	WadApp app(app_data);
	return app.MainLoop();
}

#endif
