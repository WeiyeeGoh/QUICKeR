cmake_minimum_required(VERSION 2.8)
project(UpdatableEncryptionResearchProject)

SET(ORGANIZATION_NAME "Amazon.com, Inc.")
IF (MSVC)
  SET(CMAKE_C_FLAGS "/W3 /WX /J /std:c11 /Wv:18")
ELSE()
  SET(CMAKE_C_FLAGS "-Wall -Wno-pointer-sign -g -fno-signed-char -lmemcached -msse2avx -mavx2 -lcrypto -ldl -lm -lhiredis -lrt -lpthread")
ENDIF()

ENABLE_TESTING()

include_directories(include/pkcs11/v2.40)
include_directories(include/gopt)
include_directories(src/common)
include_directories(src/hsm_encryption)
include_directories(src/cd_encryption)
include_directories(src/hiredis_storage)
include_directories(src/actions)

add_subdirectory(src/common)
add_subdirectory(src/hsm_encryption)
add_subdirectory(src/cd_encryption)
add_subdirectory(src/hiredis_storage)
add_subdirectory(src/main)
add_subdirectory(src/actions)

IF(LINUX)
  add_subdirectory(src/tools)
ENDIF()