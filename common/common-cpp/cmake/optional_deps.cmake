# ============================================================================
# Optional dependencies — looked up via find_package, gated by user options.
# ============================================================================
#
# OpenVDB and Alembic are heavyweight C++ libraries with significant transitive
# dependency trees (Boost, TBB, IlmBase/Imath, Blosc, HDF5). Building them via
# FetchContent is impractical. Instead we:
#
#   1. Default the option OFF.
#   2. If user enables, find_package the system install.
#   3. If found, define a CMake target alias used by common-cpp's CMakeLists.
#   4. If enabled but not found, error with installation hint.
#   5. If OFF, common-cpp's vdb_writer.cpp / alembic_writer.cpp compile to
#      stubs that log warnings and return failure when called.
#
# Ubuntu install:
#   sudo apt install libopenvdb-dev libalembic-dev
# ============================================================================

if(GPU_SIMS_USE_OPENVDB)
    find_package(OpenVDB CONFIG)
    if(NOT TARGET OpenVDB::openvdb)
        # Fallback: try the older module-based find.
        find_package(OpenVDB MODULE)
    endif()
    if(NOT TARGET OpenVDB::openvdb)
        message(FATAL_ERROR
            "GPU_SIMS_USE_OPENVDB=ON but OpenVDB was not found.\n"
            "Install with:  sudo apt install libopenvdb-dev\n"
            "Or:            see https://www.openvdb.org/documentation/doxygen/build.html\n"
            "Then re-run CMake. To proceed without OpenVDB, set GPU_SIMS_USE_OPENVDB=OFF."
        )
    endif()
    message(STATUS "OpenVDB:   FOUND (${OpenVDB_VERSION})")
endif()

if(GPU_SIMS_USE_ALEMBIC)
    find_package(Alembic CONFIG)
    if(NOT TARGET Alembic::Alembic)
        message(FATAL_ERROR
            "GPU_SIMS_USE_ALEMBIC=ON but Alembic was not found.\n"
            "Install with:  sudo apt install libalembic-dev\n"
            "Or:            see https://github.com/alembic/alembic\n"
            "Then re-run CMake. To proceed without Alembic, set GPU_SIMS_USE_ALEMBIC=OFF."
        )
    endif()
    message(STATUS "Alembic:   FOUND (${Alembic_VERSION})")
endif()
