cmake_minimum_required(VERSION 2.8)
project(hsm_encryption)

find_library(cloudhsmpkcs11 STATIC)

add_library(hsm_encryption hsm_aes_gcm.c hsm_aes_gcm.h aes.c aes.h)

IF (NOT WIN32)
  target_link_libraries(hsm_encryption dl)
ENDIF()
