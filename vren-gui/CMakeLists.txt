find_package(OpenGL REQUIRED)

add_executable(vren-gui
	imgui/imconfig.h
	imgui/imgui.cpp
	imgui/imgui.h
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_impl_glfw.cpp
	imgui/imgui_impl_glfw.h
	imgui/imgui_impl_opengl3.cpp
	imgui/imgui_impl_opengl3.h
	imgui/imgui_internal.h
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/imstb_rectpack.h
	imgui/imstb_textedit.h
	imgui/imstb_truetype.h

	tool/camera.h
	tool/imGuiManager.cpp
	tool/imguiManager.h
	tool/lines.h
	tool/scene.h
	tool/screen_texture.h
	tool/shader.h
	tool/tool.cpp
	tool/tool.h
	tool/tool_callbacks.h
	tool/tracer_data.h
	tool/widgets.h
	tool/window.cpp
	tool/window.h
	tool/zoom_pixel.h
	
	exts/glad/glad.c

	main.cpp
)

target_include_directories( vren-gui PRIVATE ${PROJECT_SOURCE_DIR}/vren )

target_include_directories( vren-gui PRIVATE ${yocto_gl_SOURCE_DIR}/libs )
target_link_directories( vren-gui PRIVATE ${yocto_gl_BINARY_DIR} )

target_include_directories( vren-gui PRIVATE ${glfw_SOURCE_DIR}/include )
target_link_directories( vren-gui PRIVATE ${glfw_BINARY_DIR} )

target_include_directories( vren-gui PRIVATE ${glm_SOURCE_DIR} )
target_include_directories( vren-gui PRIVATE exts )

if(MSVC)
  target_link_directories( vren-gui PUBLIC "/Program\ Files/Intel/Embree3/lib" "C:/Program\ Files/Intel/Embree3/lib" )
endif(MSVC)
if(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(vren-gui Threads::Threads)
endif(UNIX)

target_link_libraries( vren-gui yocto embree3 tbb glfw opengl32 )