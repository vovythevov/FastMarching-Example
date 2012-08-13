#
# Create a very simple ITk project
#

cmake_minimum_required (VERSION 2.6) 
set (Project_Name OrcArmy)
project (${Project_Name})

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

set(BUILD_SHARED_LIBS OFF)

add_executable (${Project_Name} OrcArmy.cxx)
target_link_libraries(${Project_Name} ${ITK_LIBRARIES})