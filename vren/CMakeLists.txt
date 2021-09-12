add_executable(vren
  box.h 
  bvh.cpp
  bvh.h
  bvh_model.h
  bvh_structs.h
  callbacks.h 
  camera.h
  color.h 
  envmap.h
  hit_record.h
  hittable.h
  hittable_list.h
  material.h
  medium.h
  model.h
  onb.h
  pathtracer.h
  pdf.h
  plane.h
  rawdata.h
  ray.h
  rnd.h
  rtw_stb_image.h
  rtweekend.h
  sphere.h
  stb_image.h
  texture.h
  thread_pool.hpp
  tracer.h
  tracer_callback.h
  vec3.h
  main.cpp
)

#message("yocto_gl_SOURCE_DIR = ${yocto_gl_SOURCE_DIR}")

target_include_directories(vren PRIVATE ${yocto_gl_SOURCE_DIR}/libs)
target_link_directories( vren PRIVATE ${yocto_gl_BINARY_DIR} )

if(MSVC)
  target_link_directories( vren PUBLIC "/Program\ Files/Intel/Embree3/lib" "C:/Program\ Files/Intel/Embree3/lib" )
endif(MSVC)
if(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(vren Threads::Threads)
endif(UNIX)

target_link_libraries( vren yocto )
target_link_libraries( vren embree3 tbb )