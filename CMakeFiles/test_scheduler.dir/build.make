# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/liaochao/workspace/mysylar

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/liaochao/workspace/mysylar

# Include any dependencies generated for this target.
include CMakeFiles/test_scheduler.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test_scheduler.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_scheduler.dir/flags.make

CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.o: CMakeFiles/test_scheduler.dir/flags.make
CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.o: test/test_scheduler.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/liaochao/workspace/mysylar/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.o -c /home/liaochao/workspace/mysylar/test/test_scheduler.cpp

CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/liaochao/workspace/mysylar/test/test_scheduler.cpp > CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.i

CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/liaochao/workspace/mysylar/test/test_scheduler.cpp -o CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.s

# Object files for target test_scheduler
test_scheduler_OBJECTS = \
"CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.o"

# External object files for target test_scheduler
test_scheduler_EXTERNAL_OBJECTS =

bin/test_scheduler: CMakeFiles/test_scheduler.dir/test/test_scheduler.cpp.o
bin/test_scheduler: CMakeFiles/test_scheduler.dir/build.make
bin/test_scheduler: lib/libmysylar.a
bin/test_scheduler: /usr/local/lib/libyaml-cpp.a
bin/test_scheduler: CMakeFiles/test_scheduler.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/liaochao/workspace/mysylar/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/test_scheduler"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_scheduler.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_scheduler.dir/build: bin/test_scheduler

.PHONY : CMakeFiles/test_scheduler.dir/build

CMakeFiles/test_scheduler.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_scheduler.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_scheduler.dir/clean

CMakeFiles/test_scheduler.dir/depend:
	cd /home/liaochao/workspace/mysylar && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/liaochao/workspace/mysylar /home/liaochao/workspace/mysylar /home/liaochao/workspace/mysylar /home/liaochao/workspace/mysylar /home/liaochao/workspace/mysylar/CMakeFiles/test_scheduler.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_scheduler.dir/depend

