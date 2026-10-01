cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#idf_build_set_property(MINIMAL_BUILD ON)
# the warning flags the author's build driver (buildit) passes to every build - the code
# relies on them
idf_build_set_property(COMPILE_OPTIONS "-Wno-parentheses" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers" APPEND)
project(ultra-remote-idf)
