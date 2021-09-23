cmake_minimum_required ( VERSION 3.2 )

project ( v_ren 
  VERSION 1.0.0 
  LANGUAGES CXX
)

set ( CMAKE_CXX_STANDARD 17 )

option(VREN_GUI "Build vren-gui" OFF)

add_subdirectory(exts)
add_subdirectory(vren)

if(VREN_GUI)
add_subdirectory(vren-gui)
endif()
