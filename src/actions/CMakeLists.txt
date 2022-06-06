cmake_minimum_required(VERSION 2.8)
project(mainfile)

find_library(cloudhsmpkcs11 STATIC)
find_library(cd_encryption STATIC)
find_library(hsm_encryption STATIC)
find_library(hiredis_storage STATIC)

add_library(actions actions.c actions.h)

IF (NOT WIN32)
  target_link_libraries(actions dl)
ENDIF()

target_link_libraries(actions cloudhsmpkcs11)
target_link_libraries(actions cd_encryption)
target_link_libraries(actions hsm_encryption)
target_link_libraries(actions hiredis_storage)
