set (EXAMPLES
    basic-push-from_stack
    basic-create_table
    basic-class
)

foreach(example ${EXAMPLES})
    add_executable(ex_${example} "examples/${example}.cpp")
    target_link_libraries(ex_${example} apollo)
    set_target_properties(ex_${example} PROPERTIES
        FOLDER "Examples"
        COMPILE_DEFINITIONS "BOOST_TEST_MODULE=${example}")
endforeach()

needs_apollo_dll(ex_basic-push-from_stack) # Any target is enough.
