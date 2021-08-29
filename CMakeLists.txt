cmake_minimum_required( VERSION 3.20.0 )

project(DT-Kernel-Cleaner
	VERSION 1.0.3
	LANGUAGES CXX)

include(GNUInstallDirs)

set (CMAKE_CXX_STANDARD 17)

set ( SOURCES main.cpp )
set ( HEADERS )

add_executable( dt-kernel-cleaner ${SOURCES} ${HEADERS})
target_link_libraries( dt-kernel-cleaner )

# installation config
install(TARGETS dt-kernel-cleaner RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
