#set(This ETest1)
#
#set(Sources
#        test1.c
#        )
#
#add_executable(
#        ${This} ${Sources}
#)
#target_link_libraries(
#        ${This} PUBLIC
#        gtest_main
#        ketteq_extension
#)
#
#add_test(
#        NAME ${This}
#        COMMAND ${This}
#)