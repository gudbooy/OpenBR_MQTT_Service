find_package(Threads REQUIRED)
find_package(OpenCV 2.4.5 REQUIRED)
set(OPENCV_DEPENDENCIES opencv_core opencv_highgui opencv_imgproc opencv_ml opencv_objdetect)
set(BR_THIRDPARTY_LIBS ${BR_THIRDPARTY_LIBS} ${OPENCV_DEPENDENCIES})

set(QT_DEPENDENCIES Concurrent Core)
	option(BR_EMBEDDED "Limit software dependencies")
	if(NOT BR_EMBEDDED)
	  set(QT_DEPENDENCIES ${QT_DEPENDENCIES} Gui Network Sql Widgets Xml)
	endif()
	foreach(QT_DEPENDENCY ${QT_DEPENDENCIES})
	  find_package(Qt5${QT_DEPENDENCY})
	endforeach()
	set(BR_THIRDPARTY_LIBS ${BR_THIRDPARTY_LIBS} ${Qt5Core_QTMAIN_LIBRARIES})


find_path(OPENBR_DIR include/openbr/openbr.h)
include_directories(${OPENBR_DIR}/include)
link_directories(${OPENBR_DIR}/lib)
set(OPENBR_LIBS "openbr")

include_directories(./)
add_executable(test listen_pub.cpp client_shared.cpp)
target_link_libraries(test openbr libmosquitto mosquitto_pub ${OPENCV_DEPENDENCIES}  ${CMAKE_THREAD_LIBS_INIT})
qt5_use_modules(test ${QT_DEPENDENCIES})

add_definitions(-g `pkg-config --libs --cflags dbus-1 glib-2.0 dbus-glib-1`)
