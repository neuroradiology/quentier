if(USE_QT5)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Network REQUIRED)
  find_package(Qt5Webkit REQUIRED)
  set(QT_INCLUDES "${Qt5Core_INCLUDES} ${Qt5Gui_INCLUDES} ${Qt5Network_INCLUDES} ${Qt5Webkit_INCLUDES}")
  set(QT_LIBRARIES "${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES} ${Qt5Network_LIBRARIES} ${Qt5Webkit_LIBRARIES}")
  set(QT_DEFINITIONS "${Qt5Core_DEFINITIONS} ${Qt5Gui_DEFINITIONS} ${Qt5Network_DEFINITIONS} ${Qt5Webkit_DEFINITIONS}")
else()
  find_package(Qt4 COMPONENTS QTCORE QTGUI QTNETWORK QTWEBKIT REQUIRED)
  include(${QT_USE_FILE})
endif()

include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")
add_definitions(${QT_DEFINITIONS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR "ON")
