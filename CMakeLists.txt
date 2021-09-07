cmake_minimum_required ( VERSION 3.2 )

project ( v_ren 
  VERSION 1.0.0 
  LANGUAGES CXX
)

set ( CMAKE_CXX_STANDARD 17 )

add_subdirectory(exts)
add_subdirectory(vren)
# ignore vren-gui for now