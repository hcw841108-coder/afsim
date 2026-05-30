project(wsf_onboard_computer)
include(swdev_project)

FILE(GLOB HDRS source/*.hpp)
FILE(GLOB SRCS source/*.cpp)

wsf_project_template(${PROJECT_NAME} PLUGIN
HDRS
   ${HDRS}
SRCS
   ${SRCS}
GRAMMARS
   grammar/wsf_onboard_computer.ag
NOTICES
   NO_EXPORT.md
   README.md
)
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "wsf_plugins/xdu")

find_package(Boost REQUIRED COMPONENTS headers PATHS ${CMAKE_CURRENT_SOURCE_DIR}/Boost)
if(Boost_headers_FOUND)
    #message("Boost_FOUND")
	#message("Boost_INCLUDE_DIRS:" ${Boost_INCLUDE_DIRS})
	#message("Boost_LIBRARIES:" ${Boost_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} Boost::headers)
endif()

target_link_libraries(${PROJECT_NAME} ${WSF_LIBS} utilqt)