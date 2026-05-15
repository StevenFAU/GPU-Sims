# Cat 3 Stack C driver. Built only when GPU_SIMS_BUILD_INTEGRITY_CAT3=ON.

add_executable(integrity_cat3_stack_c main.cpp)
set_target_properties(integrity_cat3_stack_c PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
