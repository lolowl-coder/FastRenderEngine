set(APP "App")

set(INCLUDES
	)

set(HEADERS
	)

set(SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
	)

# Other libraries are propagated from public AppLib's libraries
set(LIBS
	"AppLib"
	)

buildApp("${APP}" "${SOURCES}" "${HEADERS}" "${LIBS}" "${INCLUDES}")

set_target_properties(${APP} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Data"
)

if(WIN32)
    set(RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${APP} PRIVATE ${RESOURCE_FILES})
endif()

# INSTALL TARGETS

# Install fonts
#install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Fonts DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Data/Fonts/Roboto-Regular.ttf DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/Fonts)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Data/Fonts/Roboto-Bold.ttf DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/Fonts)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Data/Fonts/icomoon.ttf DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/Fonts)

# Install shaders
# Use file(GLOB ...) to get all .SPV files in the Shaders directory
file(GLOB SPV_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shaders/*.SPV")
install(FILES ${SPV_SHADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/Shaders)

# Install meshes
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Models/unitCube DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/Models)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Models/unitQuad DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/Models)

# Install textures
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Data/Textures/default.jpg DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/Textures)

installApp(${APP})

# set(EXECUTABLE_NAME "App_test")
# add_subdirectory(Tests)
# buildExample()