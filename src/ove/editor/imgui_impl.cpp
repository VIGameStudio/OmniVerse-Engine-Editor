#define _CRT_SECURE_NO_WARNINGS

#include "imgui_impl.hpp"

#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

#include <ove/system/input.hpp>

using namespace ove::core;
using namespace ove::system;

namespace ove
{
	namespace editor
	{
		namespace detail
		{
			// OpenGL Data
			static GLuint g_GlVersion = 0;                // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries.
			static char g_GlslVersionString[32] = "";   // Specified by user or detected based on compile time GL settings.
			static GLuint g_FontTexture = 0;
			static GLuint g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
			static int g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;                                // Uniforms location
			static int g_AttribLocationVtxPos = 0, g_AttribLocationVtxUV = 0, g_AttribLocationVtxColor = 0; // Vertex attributes location
			static unsigned int g_VboHandle = 0, g_ElementsHandle = 0;

			static cursor_t* g_MouseCursors[ImGuiMouseCursor_COUNT] = {};

			void ImGui_Impl_ScrollCallback(window_t* pWindow, f32 dx, f32 dy)
			{
				ImGuiIO& io = ImGui::GetIO();
				io.MouseWheelH += dx;
				io.MouseWheel += dy;
			}

			void ImGui_Impl_KeyCallback(window_t* pWindow, u8 key, bool pressed)
			{
				ImGuiIO& io = ImGui::GetIO();
				if (pressed)
					io.KeysDown[key] = true;
				else
					io.KeysDown[key] = false;

				// Modifiers are not reliable across systems
				io.KeyCtrl = io.KeysDown[KB_LEFTCTRL] || io.KeysDown[KB_RIGHTCTRL];
				io.KeyShift = io.KeysDown[KB_LEFTSHIFT] || io.KeysDown[KB_RIGHTSHIFT];
				io.KeyAlt = io.KeysDown[KB_LEFTALT] || io.KeysDown[KB_RIGHTALT];
#ifdef _WIN32
				io.KeySuper = false;
#else
				io.KeySuper = io.KeysDown[KB_LEFTMETA] || io.KeysDown[KB_RIGHTMETA];
#endif
			}

			void ImGui_Impl_CharCallback(window_t* pWindow, u32 c)
			{
				ImGuiIO& io = ImGui::GetIO();
				io.AddInputCharacter(c);
			}

			static const char* ImGui_Impl_GetClipboardText(void* user_data)
			{
				window_t* pWindow = static_cast<window_t*>(user_data);
				return pWindow->getClipboardString();
			}

			static void ImGui_Impl_SetClipboardText(void* user_data, const char* text)
			{
				window_t* pWindow = static_cast<window_t*>(user_data);
				pWindow->setClipboardString(text);
			}

			bool ImGui_Impl_InitIO(window_t* pWindow)
			{
				// Setup back-end capabilities flags
				ImGuiIO& io = ImGui::GetIO();
				io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
				io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
				io.BackendPlatformName = "imgui_impl_ove_system";

				// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
				io.KeyMap[ImGuiKey_Tab] = KB_TAB;
				io.KeyMap[ImGuiKey_LeftArrow] = KB_LEFT;
				io.KeyMap[ImGuiKey_RightArrow] = KB_RIGHT;
				io.KeyMap[ImGuiKey_UpArrow] = KB_UP;
				io.KeyMap[ImGuiKey_DownArrow] = KB_DOWN;
				io.KeyMap[ImGuiKey_PageUp] = KB_PAGEUP;
				io.KeyMap[ImGuiKey_PageDown] = KB_PAGEDOWN;
				io.KeyMap[ImGuiKey_Home] = KB_HOME;
				io.KeyMap[ImGuiKey_End] = KB_END;
				io.KeyMap[ImGuiKey_Insert] = KB_INSERT;
				io.KeyMap[ImGuiKey_Delete] = KB_DELETE;
				io.KeyMap[ImGuiKey_Backspace] = KB_BACKSPACE;
				io.KeyMap[ImGuiKey_Space] = KB_SPACE;
				io.KeyMap[ImGuiKey_Enter] = KB_ENTER;
				io.KeyMap[ImGuiKey_Escape] = KB_ESCAPE;
				io.KeyMap[ImGuiKey_KeyPadEnter] = KB_KPENTER;
				io.KeyMap[ImGuiKey_A] = KB_A;
				io.KeyMap[ImGuiKey_C] = KB_C;
				io.KeyMap[ImGuiKey_V] = KB_V;
				io.KeyMap[ImGuiKey_X] = KB_X;
				io.KeyMap[ImGuiKey_Y] = KB_Y;
				io.KeyMap[ImGuiKey_Z] = KB_Z;

				io.SetClipboardTextFn = ImGui_Impl_SetClipboardText;
				io.GetClipboardTextFn = ImGui_Impl_GetClipboardText;
				io.ClipboardUserData = static_cast<void*>(pWindow);
				//#if defined(_WIN32)
				//	io.ImeWindowHandle = (void*)glfwGetWin32Window(g_Window);
				//#endif

				g_MouseCursors[ImGuiMouseCursor_Arrow] = pWindow->loadSystemCursor(WIN_ARROW_CURSOR);
				g_MouseCursors[ImGuiMouseCursor_TextInput] = pWindow->loadSystemCursor(WIN_IBEAM_CURSOR);
				g_MouseCursors[ImGuiMouseCursor_ResizeNS] = pWindow->loadSystemCursor(WIN_VRESIZE_CURSOR);
				g_MouseCursors[ImGuiMouseCursor_ResizeEW] = pWindow->loadSystemCursor(WIN_HRESIZE_CURSOR);
				g_MouseCursors[ImGuiMouseCursor_Hand] = pWindow->loadSystemCursor(WIN_HAND_CURSOR);
				g_MouseCursors[ImGuiMouseCursor_ResizeAll] = pWindow->loadSystemCursor(WIN_RESIZE_ALL_CURSOR);
				g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = pWindow->loadSystemCursor(WIN_RESIZE_NESW_CURSOR);
				g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = pWindow->loadSystemCursor(WIN_RESIZE_NWSE_CURSOR);
				g_MouseCursors[ImGuiMouseCursor_NotAllowed] = pWindow->loadSystemCursor(WIN_NOT_ALLOWED_CURSOR);

				return true;
			}

			bool ImGui_Impl_InitRenderer(window_t* pWindow)
			{
				const char* glsl_version = "#version 130";

				// Query for GL version
#if !defined(IMGUI_IMPL_OPENGL_ES2)
				GLint major, minor;
				glGetIntegerv(GL_MAJOR_VERSION, &major);
				glGetIntegerv(GL_MINOR_VERSION, &minor);
				g_GlVersion = major * 1000 + minor;
#else
				g_GlVersion = 2000; // GLES 2
#endif

// Setup back-end capabilities flags
				ImGuiIO& io = ImGui::GetIO();
				io.BackendRendererName = "imgui_impl_ove_graphics";

				if (g_GlVersion >= 3200)
					io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

				// Store GLSL version string so we can refer to it later in case we recreate shaders.
				// Note: GLSL version is NOT the same as GL version. Leave this to NULL if unsure.
#if defined(IMGUI_IMPL_OPENGL_ES2)
				if (glsl_version == NULL)
					glsl_version = "#version 100";
#elif defined(IMGUI_IMPL_OPENGL_ES3)
				if (glsl_version == NULL)
					glsl_version = "#version 300 es";
#else
				if (glsl_version == NULL)
					glsl_version = "#version 130";
#endif
				IM_ASSERT((int)strlen(glsl_version) + 2 < IM_ARRAYSIZE(g_GlslVersionString));
				strcpy(g_GlslVersionString, glsl_version);
				strcat(g_GlslVersionString, "\n");

				// Make a dummy GL call (we don't actually need the result)
				// IF YOU GET A CRASH HERE: it probably means that you haven't initialized the OpenGL function loader used by this code.
				// Desktop OpenGL 3/4 need a function loader. See the IMGUI_IMPL_OPENGL_LOADER_xxx explanation above.
				GLint current_texture;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

				return true;
			}

			// Functions
			bool ImGui_Impl_Init(window_t* pWindow)
			{
				ImGui_Impl_InitIO(pWindow);
				ImGui_Impl_InitRenderer(pWindow);

				return true;
			}

			void ImGui_Impl_DestroyDeviceObjects();

			void ImGui_Impl_Shutdown()
			{
				ImGui_Impl_DestroyDeviceObjects();
			}

			static void ImGui_Impl_UpdateMousePosAndButtons(window_t& window)
			{
				// Update buttons
				ImGuiIO& io = ImGui::GetIO();
				io.MouseDown[0] = window.isButtonDown(MBTN_LEFT);
				io.MouseDown[1] = window.isButtonDown(MBTN_RIGHT);
				io.MouseDown[2] = window.isButtonDown(MBTN_MIDDLE);

				// Update mouse position
				const ImVec2 mouse_pos_backup = io.MousePos;
				io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

				bool focused = true;
				if (focused)
				{
					if (io.WantSetMousePos)
					{
						window.setMousePos((i32)mouse_pos_backup.x, (i32)mouse_pos_backup.y);
					}
					else
					{
						int mx, my;
						window.getMousePos(mx, my);
						io.MousePos = ImVec2(mx, my);
					}
				}
			}

			static void ImGui_Impl_UpdateMouseCursor(window_t& window)
			{
				ImGuiIO& io = ImGui::GetIO();
				if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || window.getInputMode(WIN_CURSOR) == WIN_CURSOR_DISABLED)
				{
					return;
				}

				ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
				if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
				{
					// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
					window.setInputMode(WIN_CURSOR, WIN_CURSOR_HIDDEN);
				}
				else
				{
					// Show OS mouse cursor
					window.setCursor(g_MouseCursors[imgui_cursor] ? g_MouseCursors[imgui_cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
					window.setInputMode(WIN_CURSOR, WIN_CURSOR_NORMAL);
				}
			}

			bool ImGui_Impl_CreateDeviceObjects();

			void ImGui_Impl_NewFrame(f32 dt, window_t& window)
			{
				if (!g_ShaderHandle)
					ImGui_Impl_CreateDeviceObjects();

				ImGuiIO& io = ImGui::GetIO();
				IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

				// Setup display size (every frame to accommodate for window resizing)
				unsigned int w, h;
				window.getSize(w, h);
				io.DisplaySize = ImVec2((float)w, (float)h);
				if (w > 0 && h > 0)
					io.DisplayFramebufferScale = ImVec2(1.f, 1.f);

				// Setup time step
				io.DeltaTime = dt;

				ImGui_Impl_UpdateMousePosAndButtons(window);
				ImGui_Impl_UpdateMouseCursor(window);

				// Update game controllers (if enabled and available)
				//ImGui_Impl_UpdateGamepads(window);
			}

			static void ImGui_Impl_SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height, GLuint vertex_array_object)
			{
				// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_CULL_FACE);
				glDisable(GL_DEPTH_TEST);
				glEnable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

				// Setup viewport, orthographic projection matrix
				// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
				glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
				float L = draw_data->DisplayPos.x;
				float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
				float T = draw_data->DisplayPos.y;
				float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
				const float ortho_projection[4][4] =
				{
					{ 2.0f / (R - L),   0.0f,         0.0f,   0.0f },
					{ 0.0f,         2.0f / (T - B),   0.0f,   0.0f },
					{ 0.0f,         0.0f,        -1.0f,   0.0f },
					{ (R + L) / (L - R),  (T + B) / (B - T),  0.0f,   1.0f },
				};
				glUseProgram(g_ShaderHandle);
				glUniform1i(g_AttribLocationTex, 0);
				glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
#ifdef GL_SAMPLER_BINDING
				glBindSampler(0, 0); // We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
#endif

				(void)vertex_array_object;
#ifndef IMGUI_IMPL_OPENGL_ES2
				glBindVertexArray(vertex_array_object);
#endif

				// Bind vertex/index buffers and setup attributes for ImDrawVert
				glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
				glEnableVertexAttribArray(g_AttribLocationVtxPos);
				glEnableVertexAttribArray(g_AttribLocationVtxUV);
				glEnableVertexAttribArray(g_AttribLocationVtxColor);
				glVertexAttribPointer(g_AttribLocationVtxPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
				glVertexAttribPointer(g_AttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
				glVertexAttribPointer(g_AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
			}

			// OpenGL3 Render function.
			// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
			// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so.
			void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
			{
				// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
				int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
				int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
				if (fb_width <= 0 || fb_height <= 0)
					return;

				// Backup GL state
				GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
				glActiveTexture(GL_TEXTURE0);
				GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
				GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
#ifdef GL_SAMPLER_BINDING
				GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
#endif
				GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_ES2
				GLint last_vertex_array_object; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array_object);
#endif
#ifdef GL_POLYGON_MODE
				GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
#endif
				GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
				GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
				GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
				GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
				GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
				GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
				GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
				GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
				GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
				GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
				GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
				GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
				bool clip_origin_lower_left = true;
#if defined(GL_CLIP_ORIGIN) && !defined(__APPLE__)
				GLenum last_clip_origin = 0; glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&last_clip_origin); // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
				if (last_clip_origin == GL_UPPER_LEFT)
					clip_origin_lower_left = false;
#endif

				// Setup desired GL state
				// Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to. VAO are not shared among GL contexts)
				// The renderer would actually work without any VAO bound, but then our VertexAttrib calls would overwrite the default one currently bound.
				GLuint vertex_array_object = 0;
#ifndef IMGUI_IMPL_OPENGL_ES2
				glGenVertexArrays(1, &vertex_array_object);
#endif
				ImGui_Impl_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);

				// Will project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
				ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

				// Render command lists
				for (int n = 0; n < draw_data->CmdListsCount; n++)
				{
					const ImDrawList* cmd_list = draw_data->CmdLists[n];

					// Upload vertex/index buffers
					glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

					for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
					{
						const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
						if (pcmd->UserCallback != NULL)
						{
							// User callback, registered via ImDrawList::AddCallback()
							// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
							if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
								ImGui_Impl_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);
							else
								pcmd->UserCallback(cmd_list, pcmd);
						}
						else
						{
							// Project scissor/clipping rectangles into framebuffer space
							ImVec4 clip_rect;
							clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
							clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
							clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
							clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

							if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
							{
								// Apply scissor/clipping rectangle
								if (clip_origin_lower_left)
									glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
								else
									glScissor((int)clip_rect.x, (int)clip_rect.y, (int)clip_rect.z, (int)clip_rect.w); // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)

								// Bind texture, Draw
								glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
#if IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
								if (g_GlVersion >= 3200)
									glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)), (GLint)pcmd->VtxOffset);
								else
#endif
									glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
							}
						}
					}
				}

				// Destroy the temporary VAO
#ifndef IMGUI_IMPL_OPENGL_ES2
				glDeleteVertexArrays(1, &vertex_array_object);
#endif

				// Restore modified GL state
				glUseProgram(last_program);
				glBindTexture(GL_TEXTURE_2D, last_texture);
#ifdef GL_SAMPLER_BINDING
				glBindSampler(0, last_sampler);
#endif
				glActiveTexture(last_active_texture);
#ifndef IMGUI_IMPL_OPENGL_ES2
				glBindVertexArray(last_vertex_array_object);
#endif
				glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
				glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
				glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
				if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
				if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
				if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
				if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
				glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
#endif
				glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
				glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
			}

			bool ImGui_Impl_CreateFontsTexture()
			{
				// Build texture atlas
				ImGuiIO& io = ImGui::GetIO();
				unsigned char* pixels;
				int width, height;
				io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

				// Upload texture to graphics system
				GLint last_texture;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
				glGenTextures(1, &g_FontTexture);
				glBindTexture(GL_TEXTURE_2D, g_FontTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

				// Store our identifier
				io.Fonts->TexID = (ImTextureID)(intptr_t)g_FontTexture;

				// Restore state
				glBindTexture(GL_TEXTURE_2D, last_texture);

				return true;
			}

			void ImGui_Impl_DestroyFontsTexture()
			{
				if (g_FontTexture)
				{
					ImGuiIO& io = ImGui::GetIO();
					glDeleteTextures(1, &g_FontTexture);
					io.Fonts->TexID = 0;
					g_FontTexture = 0;
				}
			}

			// If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
			static bool CheckShader(GLuint handle, const char* desc)
			{
				GLint status = 0, log_length = 0;
				glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
				glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
				if ((GLboolean)status == GL_FALSE)
					fprintf(stderr, "ERROR: ImGui_Impl_CreateDeviceObjects: failed to compile %s!\n", desc);
				if (log_length > 1)
				{
					ImVector<char> buf;
					buf.resize((int)(log_length + 1));
					glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
					fprintf(stderr, "%s\n", buf.begin());
				}
				return (GLboolean)status == GL_TRUE;
			}

			// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
			static bool CheckProgram(GLuint handle, const char* desc)
			{
				GLint status = 0, log_length = 0;
				glGetProgramiv(handle, GL_LINK_STATUS, &status);
				glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
				if ((GLboolean)status == GL_FALSE)
					fprintf(stderr, "ERROR: ImGui_Impl_CreateDeviceObjects: failed to link %s! (with GLSL '%s')\n", desc, g_GlslVersionString);
				if (log_length > 1)
				{
					ImVector<char> buf;
					buf.resize((int)(log_length + 1));
					glGetProgramInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
					fprintf(stderr, "%s\n", buf.begin());
				}
				return (GLboolean)status == GL_TRUE;
			}

			bool ImGui_Impl_CreateDeviceObjects()
			{
				// Backup GL state
				GLint last_texture, last_array_buffer;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_ES2
				GLint last_vertex_array;
				glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
#endif

				// Parse GLSL version string
				int glsl_version = 130;
				sscanf(g_GlslVersionString, "#version %d", &glsl_version);

				const GLchar* vertex_shader_glsl_120 =
					"uniform mat4 ProjMtx;\n"
					"attribute vec2 Position;\n"
					"attribute vec2 UV;\n"
					"attribute vec4 Color;\n"
					"varying vec2 Frag_UV;\n"
					"varying vec4 Frag_Color;\n"
					"void main()\n"
					"{\n"
					"    Frag_UV = UV;\n"
					"    Frag_Color = Color;\n"
					"    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
					"}\n";

				const GLchar* vertex_shader_glsl_130 =
					"uniform mat4 ProjMtx;\n"
					"in vec2 Position;\n"
					"in vec2 UV;\n"
					"in vec4 Color;\n"
					"out vec2 Frag_UV;\n"
					"out vec4 Frag_Color;\n"
					"void main()\n"
					"{\n"
					"    Frag_UV = UV;\n"
					"    Frag_Color = Color;\n"
					"    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
					"}\n";

				const GLchar* vertex_shader_glsl_300_es =
					"precision mediump float;\n"
					"layout (location = 0) in vec2 Position;\n"
					"layout (location = 1) in vec2 UV;\n"
					"layout (location = 2) in vec4 Color;\n"
					"uniform mat4 ProjMtx;\n"
					"out vec2 Frag_UV;\n"
					"out vec4 Frag_Color;\n"
					"void main()\n"
					"{\n"
					"    Frag_UV = UV;\n"
					"    Frag_Color = Color;\n"
					"    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
					"}\n";

				const GLchar* vertex_shader_glsl_410_core =
					"layout (location = 0) in vec2 Position;\n"
					"layout (location = 1) in vec2 UV;\n"
					"layout (location = 2) in vec4 Color;\n"
					"uniform mat4 ProjMtx;\n"
					"out vec2 Frag_UV;\n"
					"out vec4 Frag_Color;\n"
					"void main()\n"
					"{\n"
					"    Frag_UV = UV;\n"
					"    Frag_Color = Color;\n"
					"    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
					"}\n";

				const GLchar* fragment_shader_glsl_120 =
					"#ifdef GL_ES\n"
					"    precision mediump float;\n"
					"#endif\n"
					"uniform sampler2D Texture;\n"
					"varying vec2 Frag_UV;\n"
					"varying vec4 Frag_Color;\n"
					"void main()\n"
					"{\n"
					"    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
					"}\n";

				const GLchar* fragment_shader_glsl_130 =
					"uniform sampler2D Texture;\n"
					"in vec2 Frag_UV;\n"
					"in vec4 Frag_Color;\n"
					"out vec4 Out_Color;\n"
					"void main()\n"
					"{\n"
					"    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
					"}\n";

				const GLchar* fragment_shader_glsl_300_es =
					"precision mediump float;\n"
					"uniform sampler2D Texture;\n"
					"in vec2 Frag_UV;\n"
					"in vec4 Frag_Color;\n"
					"layout (location = 0) out vec4 Out_Color;\n"
					"void main()\n"
					"{\n"
					"    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
					"}\n";

				const GLchar* fragment_shader_glsl_410_core =
					"in vec2 Frag_UV;\n"
					"in vec4 Frag_Color;\n"
					"uniform sampler2D Texture;\n"
					"layout (location = 0) out vec4 Out_Color;\n"
					"void main()\n"
					"{\n"
					"    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
					"}\n";

				// Select shaders matching our GLSL versions
				const GLchar* vertex_shader = NULL;
				const GLchar* fragment_shader = NULL;
				if (glsl_version < 130)
				{
					vertex_shader = vertex_shader_glsl_120;
					fragment_shader = fragment_shader_glsl_120;
				}
				else if (glsl_version >= 410)
				{
					vertex_shader = vertex_shader_glsl_410_core;
					fragment_shader = fragment_shader_glsl_410_core;
				}
				else if (glsl_version == 300)
				{
					vertex_shader = vertex_shader_glsl_300_es;
					fragment_shader = fragment_shader_glsl_300_es;
				}
				else
				{
					vertex_shader = vertex_shader_glsl_130;
					fragment_shader = fragment_shader_glsl_130;
				}

				// Create shaders
				const GLchar* vertex_shader_with_version[2] = { g_GlslVersionString, vertex_shader };
				g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
				glShaderSource(g_VertHandle, 2, vertex_shader_with_version, NULL);
				glCompileShader(g_VertHandle);
				CheckShader(g_VertHandle, "vertex shader");

				const GLchar* fragment_shader_with_version[2] = { g_GlslVersionString, fragment_shader };
				g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
				glShaderSource(g_FragHandle, 2, fragment_shader_with_version, NULL);
				glCompileShader(g_FragHandle);
				CheckShader(g_FragHandle, "fragment shader");

				g_ShaderHandle = glCreateProgram();
				glAttachShader(g_ShaderHandle, g_VertHandle);
				glAttachShader(g_ShaderHandle, g_FragHandle);
				glLinkProgram(g_ShaderHandle);
				CheckProgram(g_ShaderHandle, "shader program");

				g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
				g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
				g_AttribLocationVtxPos = glGetAttribLocation(g_ShaderHandle, "Position");
				g_AttribLocationVtxUV = glGetAttribLocation(g_ShaderHandle, "UV");
				g_AttribLocationVtxColor = glGetAttribLocation(g_ShaderHandle, "Color");

				// Create buffers
				glGenBuffers(1, &g_VboHandle);
				glGenBuffers(1, &g_ElementsHandle);

				ImGui_Impl_CreateFontsTexture();

				// Restore modified GL state
				glBindTexture(GL_TEXTURE_2D, last_texture);
				glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_ES2
				glBindVertexArray(last_vertex_array);
#endif

				return true;
			}

			void ImGui_Impl_DestroyDeviceObjects()
			{
				if (g_VboHandle) { glDeleteBuffers(1, &g_VboHandle); g_VboHandle = 0; }
				if (g_ElementsHandle) { glDeleteBuffers(1, &g_ElementsHandle); g_ElementsHandle = 0; }
				if (g_ShaderHandle && g_VertHandle) { glDetachShader(g_ShaderHandle, g_VertHandle); }
				if (g_ShaderHandle && g_FragHandle) { glDetachShader(g_ShaderHandle, g_FragHandle); }
				if (g_VertHandle) { glDeleteShader(g_VertHandle); g_VertHandle = 0; }
				if (g_FragHandle) { glDeleteShader(g_FragHandle); g_FragHandle = 0; }
				if (g_ShaderHandle) { glDeleteProgram(g_ShaderHandle); g_ShaderHandle = 0; }

				ImGui_Impl_DestroyFontsTexture();
			}
		}
	}
}