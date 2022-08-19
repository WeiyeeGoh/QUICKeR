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
include src/main/CMakeFiles/populate_database_updatable.dir/depend.make

# Include the progress variables for this target.
include src/main/CMakeFiles/populate_database_updatable.dir/progress.make

# Include the compile flags for this target's objects.
include src/main/CMakeFiles/populate_database_updatable.dir/flags.make

src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o: src/main/CMakeFiles/populate_database_updatable.dir/flags.make
src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o: ../src/main/populate_database_updatable.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ec2-user/QUICKeR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o"
	cd /home/ec2-user/QUICKeR/build/src/main && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o   -c /home/ec2-user/QUICKeR/src/main/populate_database_updatable.c

src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.i"
	cd /home/ec2-user/QUICKeR/build/src/main && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ec2-user/QUICKeR/src/main/populate_database_updatable.c > CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.i

src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.s"
	cd /home/ec2-user/QUICKeR/build/src/main && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ec2-user/QUICKeR/src/main/populate_database_updatable.c -o CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.s

src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o.requires:

.PHONY : src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o.requires

src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o.provides: src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o.requires
	$(MAKE) -f src/main/CMakeFiles/populate_database_updatable.dir/build.make src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o.provides.build
.PHONY : src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o.provides

src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o.provides.build: src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o


# Object files for target populate_database_updatable
populate_database_updatable_OBJECTS = \
"CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o"

# External object files for target populate_database_updatable
populate_database_updatable_EXTERNAL_OBJECTS =

src/main/populate_database_updatable: src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o
src/main/populate_database_updatable: src/main/CMakeFiles/populate_database_updatable.dir/build.make
src/main/populate_database_updatable: src/common/libcloudhsmpkcs11.a
src/main/populate_database_updatable: src/cd_encryption/libcd_encryption.a
src/main/populate_database_updatable: src/hsm_encryption/libhsm_encryption.a
src/main/populate_database_updatable: src/hiredis_storage/libhiredis_storage.a
src/main/populate_database_updatable: src/actions/libactions.a
src/main/populate_database_updatable: src/common/libcloudhsmpkcs11.a
src/main/populate_database_updatable: src/cd_encryption/libcd_encryption.a
src/main/populate_database_updatable: src/hsm_encryption/libhsm_encryption.a
src/main/populate_database_updatable: src/hiredis_storage/libhiredis_storage.a
src/main/populate_database_updatable: src/main/CMakeFiles/populate_database_updatable.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ec2-user/QUICKeR/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable populate_database_updatable"
	cd /home/ec2-user/QUICKeR/build/src/main && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/populate_database_updatable.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/main/CMakeFiles/populate_database_updatable.dir/build: src/main/populate_database_updatable

.PHONY : src/main/CMakeFiles/populate_database_updatable.dir/build

src/main/CMakeFiles/populate_database_updatable.dir/requires: src/main/CMakeFiles/populate_database_updatable.dir/populate_database_updatable.c.o.requires

.PHONY : src/main/CMakeFiles/populate_database_updatable.dir/requires

src/main/CMakeFiles/populate_database_updatable.dir/clean:
	cd /home/ec2-user/QUICKeR/build/src/main && $(CMAKE_COMMAND) -P CMakeFiles/populate_database_updatable.dir/cmake_clean.cmake
.PHONY : src/main/CMakeFiles/populate_database_updatable.dir/clean

src/main/CMakeFiles/populate_database_updatable.dir/depend:
	cd /home/ec2-user/QUICKeR/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ec2-user/QUICKeR /home/ec2-user/QUICKeR/src/main /home/ec2-user/QUICKeR/build /home/ec2-user/QUICKeR/build/src/main /home/ec2-user/QUICKeR/build/src/main/CMakeFiles/populate_database_updatable.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/main/CMakeFiles/populate_database_updatable.dir/depend

