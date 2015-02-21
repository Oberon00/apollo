# Part of the apollo library -- Copyright (c) Christian Neumüller 2015
# This file is subject to the terms of the BSD 2-Clause License.
# See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

set (EXAMPLES
    basic-push-to
    basic-create_table
    basic-class
    raw_function
)

foreach(example ${EXAMPLES})
    add_executable(ex_${example} "examples/${example}.cpp")
    target_link_libraries(ex_${example} apollo)
    set_target_properties(ex_${example} PROPERTIES
        FOLDER "Examples"
        COMPILE_DEFINITIONS "BOOST_TEST_MODULE=${example}")
endforeach()

needs_apollo_dll(ex_basic-push-to) # Any target is enough.
