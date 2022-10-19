set(component_srcs
  yuv.c
  to_jpg.cpp
  to_bmp.c
  jpge.cpp
  esp_jpg_decode.c
  )

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include" "private_include")
