# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ec2-user/QUICKeR

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ec2-user/QUICKeR/build

# Include any dependencies generated for this target.
include src/main/CMakeFiles/routine_operations_updatable_encryption.dir/depend.make

# Include the progress variables for this target.
include src/main/CMakeFiles/routine_operations_updatable_encryption.dir/progress.make

# Include the compile flags for this target's objects.
include src/main/CMakeFiles/routine_operations_updatable_encryption.dir/flags.make

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o: src/main/CMakeFiles/routine_operations_updatable_encryption.dir/flags.make
src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o: ../src/main/routine_operations_updatable_encryption.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ec2-user/QUICKeR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o"
	cd /home/ec2-user/QUICKeR/build/src/main && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o   -c /home/ec2-user/QUICKeR/src/main/routine_operations_updatable_encryption.c

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.i"
	cd /home/ec2-user/QUICKeR/build/src/main && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ec2-user/QUICKeR/src/main/routine_operations_updatable_encryption.c > CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.i

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.s"
	cd /home/ec2-user/QUICKeR/build/src/main && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ec2-user/QUICKeR/src/main/routine_operations_updatable_encryption.c -o CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.s

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o.requires:

.PHONY : src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o.requires

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o.provides: src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o.requires
	$(MAKE) -f src/main/CMakeFiles/routine_operations_updatable_encryption.dir/build.make src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o.provides.build
.PHONY : src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o.provides

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o.provides.build: src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o


# Object files for target routine_operations_updatable_encryption
routine_operations_updatable_encryption_OBJECTS = \
"CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o"

# External object files for target routine_operations_updatable_encryption
routine_operations_updatable_encryption_EXTERNAL_OBJECTS =

src/main/routine_operations_updatable_encryption: src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o
src/main/routine_operations_updatable_encryption: src/main/CMakeFiles/routine_operations_updatable_encryption.dir/build.make
src/main/routine_operations_updatable_encryption: src/common/libcloudhsmpkcs11.a
src/main/routine_operations_updatable_encryption: src/cd_encryption/libcd_encryption.a
src/main/routine_operations_updatable_encryption: src/hsm_encryption/libhsm_encryption.a
src/main/routine_operations_updatable_encryption: src/hiredis_storage/libhiredis_storage.a
src/main/routine_operations_updatable_encryption: src/actions/libactions.a
src/main/routine_operations_updatable_encryption: src/common/libcloudhsmpkcs11.a
src/main/routine_operations_updatable_encryption: src/cd_encryption/libcd_encryption.a
src/main/routine_operations_updatable_encryption: src/hsm_encryption/libhsm_encryption.a
src/main/routine_operations_updatable_encryption: src/hiredis_storage/libhiredis_storage.a
src/main/routine_operations_updatable_encryption: src/main/CMakeFiles/routine_operations_updatable_encryption.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ec2-user/QUICKeR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable routine_operations_updatable_encryption"
	cd /home/ec2-user/QUICKeR/build/src/main && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/routine_operations_updatable_encryption.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/main/CMakeFiles/routine_operations_updatable_encryption.dir/build: src/main/routine_operations_updatable_encryption

.PHONY : src/main/CMakeFiles/routine_operations_updatable_encryption.dir/build

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/requires: src/main/CMakeFiles/routine_operations_updatable_encryption.dir/routine_operations_updatable_encryption.c.o.requires

.PHONY : src/main/CMakeFiles/routine_operations_updatable_encryption.dir/requires

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/clean:
	cd /home/ec2-user/QUICKeR/build/src/main && $(CMAKE_COMMAND) -P CMakeFiles/routine_operations_updatable_encryption.dir/cmake_clean.cmake
.PHONY : src/main/CMakeFiles/routine_operations_updatable_encryption.dir/clean

src/main/CMakeFiles/routine_operations_updatable_encryption.dir/depend:
	cd /home/ec2-user/QUICKeR/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ec2-user/QUICKeR /home/ec2-user/QUICKeR/src/main /home/ec2-user/QUICKeR/build /home/ec2-user/QUICKeR/build/src/main /home/ec2-user/QUICKeR/build/src/main/CMakeFiles/routine_operations_updatable_encryption.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/main/CMakeFiles/routine_operations_updatable_encryption.dir/depend
