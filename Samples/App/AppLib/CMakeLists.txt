set(LIB "AppLib")

set(INCLUDES_PRIVATE
	${GEN_DIR}
	"${CMAKE_CURRENT_SOURCE_DIR}/Includes"
	)
set(INCLUDES_PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/Includes"
	)

set(HEADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/Includes/Renderer/AppRenderer.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Includes/AppData.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Includes/AppEngine.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Includes/Options.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Includes/Pointers.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Includes/resource.h"
	)

set(SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/Renderer/AppRenderer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/AppData.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/AppEngine.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Options.cpp"
	)

set (LIBS_PRIVATE
	)

set (LIBS_PUBLIC
		"fre"
		"nvtx3-cpp"
        )

buildLib("${LIB}" "${SOURCES}" "${HEADERS}" "${LIBS_PRIVATE}" "${LIBS_PUBLIC}" "${INCLUDES_PRIVATE}" "${INCLUDES_PUBLIC}")