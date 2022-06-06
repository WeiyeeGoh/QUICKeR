cmake_minimum_required(VERSION 2.8)
project(hiredis_storage)

add_library(hiredis_storage redis_storage.c redis_storage.h)

IF (NOT WIN32)
  target_link_libraries(hiredis_storage dl)
ENDIF()