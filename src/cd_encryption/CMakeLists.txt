cmake_minimum_required(VERSION 2.8)
project(cd_encryption)

add_library(cd_encryption AE_Nested_AES.c aes_gcm.c aes_ctr.c AE_Nested_AES.h aes_gcm.h aes_ctr.h)

IF (NOT WIN32)
  target_link_libraries(cd_encryption dl)
ENDIF()
