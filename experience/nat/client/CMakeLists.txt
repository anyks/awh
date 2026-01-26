add_library(common STATIC ../common/socket_utils.cpp)
target_include_directories(common PUBLIC ../common)

add_executable(client client.cpp)
target_link_libraries(client common)

if(APPLE)
	# client/CMakeLists.txt
	find_path(MINIUPNPC_INCLUDE_DIR
		NAMES miniupnpc/miniupnpc.h
		PATHS /opt/homebrew/opt/miniupnpc/include /usr/local/include
	)

	find_library(MINIUPNPC_LIBRARY
		NAMES miniupnpc
		PATHS /opt/homebrew/opt/miniupnpc/lib /usr/local/lib
	)

	if(NOT MINIUPNPC_INCLUDE_DIR OR NOT MINIUPNPC_LIBRARY)
		message(FATAL_ERROR "miniupnpc not found. Run: brew install miniupnpc")
	endif()

	add_library(miniupnpc STATIC IMPORTED)
	set_target_properties(miniupnpc PROPERTIES
		IMPORTED_LOCATION "${MINIUPNPC_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${MINIUPNPC_INCLUDE_DIR}"
	)

	target_link_libraries(client miniupnpc)
endif()
