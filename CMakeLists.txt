cmake_minimum_required(VERSION 3.5)

project(ssmaze)

file(GLOB SOURCES
	source/shared/*.c
	source/pc/*.c
)

option(DEBUG "Build with debug symbols" OFF)
option(CONSOLE "Show console on Windows" ON)

if (DEBUG)
	set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(freeglut REQUIRED)

add_executable(ssmaze ${SOURCES})
target_include_directories(ssmaze PRIVATE ${PROJECT_SOURCE_DIR}/source/shared ${PROJECT_SOURCE_DIR}/source/thirdparty)

target_link_libraries(ssmaze glew32 glu32 freeglut SDL2)
target_link_options(ssmaze PRIVATE -static-libstdc++)

if (WIN32)
	target_link_libraries(ssmaze opengl32 ddraw dinput8 dxguid dxerr8 user32 gdi32 winmm)
	target_compile_definitions(ssmaze PRIVATE PLATFORM_SDL FX_SINGLE)

	if(NOT CONSOLE)
		target_link_options(ssmaze PRIVATE -mwindows)
	endif()

else()
	target_link_libraries(ssmaze GL)
endif()

#add_custom_command(TARGET ssmaze POST_BUILD
#                   COMMAND ${CMAKE_COMMAND} -E copy_directory
#                       ${PROJECT_SOURCE_DIR}/data $<TARGET_FILE_DIR:ssmaze>/data)