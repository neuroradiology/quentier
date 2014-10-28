if(CMAKE_COMPILER_IS_GNUCXX)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  message(STATUS "Using GNU C++ compiler, version ${GCC_VERSION}")
  if(GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8)
    message(STATUS "Your compiler supports C++11 standard.")
    add_definitions("-std=gnu++11")
    if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
      add_definitions("-fPIC")
    endif()
    add_definitions("-DCPP11_COMPLIANT=1")
  else()
    message(FATAL_ERROR "Your compiler is known to not support C++ standard as much
                         as it is required to build this application. Consider upgrading
                         your compiler to version 4.8 at least")
  endif()
  add_definitions("-Wno-uninitialized -ldl")
  if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions("-rdynamic")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE CLANG_VERSION)
  message(STATUS "Using LLVM/Clang C++ compiler, version info: ${CLANG_VERSION}")
  if(NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.1)
    message(STATUS "Your compiler supports C++11 standard.")
    add_definitions("-DCPP11_COMPLIANT=1")
  else()
    message(WARNING "Your compiler may not support all the necessary C++11 standard features
                     to build this application. If you'd get any compilation errors, consider
                     upgrading to a compiler version which fully supports the C++11 standard.")
  endif()
  add_definitions("-std=c++11 -Wno-uninitialized -Wno-null-conversion -Wno-format -Wno-deprecated")
  add_definitions("-Wmismatched-tags")
  if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions("-fPIC")
  endif()
  # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS_INIT} $ENV{LDFLAGS} -fsanitize=undefined")
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR USE_LIBCPP)
    find_library(LIBCPP NAMES libc++.so libc++.so.1.0 libc++.dylib OPTIONAL)
    if(LIBCPP)
      message(STATUS "Using native Clang's C++ standard library: ${LIBCPP}")
      add_definitions("-stdlib=libc++ -DHAVELIBCPP")
    endif()
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC12")
  message(STATUS "Visual C++ 2013 compiler supports C++11 standard.")
  add_definitions("-DCPP11_COMPLIANT=1")
else()
  message(WARNING "Your C++ compiler is not officially supported for building of this application.
                   If you'd get any compilation errors, consider upgrading to a compiler version
                   which fully supports the C++11 standard.")
endif(CMAKE_COMPILER_IS_GNUCXX)
