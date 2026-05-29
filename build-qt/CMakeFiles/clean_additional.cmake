# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\DANDU_MOD_injecter_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\DANDU_MOD_injecter_autogen.dir\\ParseCache.txt"
  "DANDU_MOD_injecter_autogen"
  )
endif()
