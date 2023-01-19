set(component_srcs
  yuv.c
  to_jpg.cpp
  to_bmp.c
  jpge.cpp
  esp_jpg_decode.c
  )

#message(STATUS "----- CMakeLists.txt")
#message(STATUS "CMAKE_SOURCE_DIR        =${CMAKE_SOURCE_DIR}")
#message(STATUS "CMAKE_BINARY_DIR        =${CMAKE_BINARY_DIR}")
#message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
#message(STATUS "CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
#message(STATUS "CMAKE_CURRENT_LIST_DIR  =${CMAKE_CURRENT_LIST_DIR}")

set(component_include
  include
  ../../managed_components/espressif__esp_jpeg/tjpgd
  )

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "${component_include}" "private_include")
