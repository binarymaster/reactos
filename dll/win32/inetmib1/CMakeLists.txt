
add_definitions(-D__ROS_LONG64__)
spec2def(inetmib1.dll inetmib1.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/inetmib1_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/inetmib1.def)

add_library(inetmib1 MODULE ${SOURCE})
set_module_type(inetmib1 win32dll)
target_link_libraries(inetmib1 wine)
add_delay_importlibs(inetmib1 iphlpapi)
add_importlibs(inetmib1 snmpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET inetmib1 DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(inetmib1) # CORE-5743: No ARRAY_SIZE macro
