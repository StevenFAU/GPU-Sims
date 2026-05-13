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
#   sudo apt install libopenvdb-dev libimath-dev
# (Alembic is vendored via FetchContent in the Alembic block below; only
# Alembic's transitive Imath dep needs an apt package.)
# ============================================================================

if(GPU_SIMS_USE_OPENVDB)
    find_package(OpenVDB CONFIG)
    if(NOT TARGET OpenVDB::openvdb)
        # Fallback: try the older module-based find.
        # Debian/Ubuntu's libopenvdb-dev ships FindOpenVDB.cmake under
        # /usr/lib/<multi-arch-triple>/cmake/OpenVDB/, which is not on
        # CMake's default MODULE search path. Hint it explicitly via a
        # block-scoped append to CMAKE_MODULE_PATH, then restore the
        # variable so this hint does not leak to other find_package
        # MODULE calls later in the configure.
        #
        # On non-Debian distros where CMAKE_LIBRARY_ARCHITECTURE is
        # empty, the appended path becomes /usr/lib//cmake/OpenVDB
        # which is harmless — CMake silently ignores non-existent
        # entries in CMAKE_MODULE_PATH and falls through to its
        # built-in module search.
        set(_gpusims_saved_module_path "${CMAKE_MODULE_PATH}")
        list(APPEND CMAKE_MODULE_PATH
             "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/OpenVDB")
        find_package(OpenVDB MODULE)
        set(CMAKE_MODULE_PATH "${_gpusims_saved_module_path}")
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
    # ------------------------------------------------------------------
    # Alembic 1.8.10 vendored via FetchContent (pinned SHA).
    #
    # Why vendored: `libalembic-dev` was dropped from Ubuntu 24.04 noble
    # after Ubuntu 22.04 jammy (last shipped: 1.7.16-3 on jammy). The
    # CMake-find_package path against a system install is no longer
    # reliable on modern Ubuntu LTS.
    #
    # Why 1.8.10 not 1.8.11: Alembic 1.8.11 raises cmake_minimum_required
    # to 3.29; Ubuntu 24.04 noble ships CMake 3.28.3 (the CI runner
    # baseline). 1.8.10 still requires only CMake 3.13.
    #
    # Pinned SHA: c254caf2705ebf5271408dd37a091aa379258a38 (2025-11-17
    # release tag for v1.8.10).
    #
    # Apt dep needed: libimath-dev (transitive — IMath types in
    # Alembic's public API). HDF5 / OpenEXR / Boost are NOT reached
    # for with the flag set below.
    # ------------------------------------------------------------------
    include(FetchContent)

    set(USE_HDF5             OFF CACHE BOOL "Alembic: skip HDF5 backend; Ogawa only" FORCE)
    set(ALEMBIC_SHARED_LIBS  ON  CACHE BOOL "Alembic: build shared libs" FORCE)
    set(USE_TESTS            OFF CACHE BOOL "Alembic: skip test binaries" FORCE)
    set(USE_BINARIES         OFF CACHE BOOL "Alembic: skip CLI tools (abcconvert, abcecho, ...)" FORCE)
    set(USE_EXAMPLES         OFF CACHE BOOL "Alembic: skip Alembic-internal examples" FORCE)
    set(ALEMBIC_DEBUG_WARNINGS_AS_ERRORS OFF CACHE BOOL "Alembic: don't promote our project flags to errors" FORCE)

    FetchContent_Declare(alembic
        GIT_REPOSITORY https://github.com/alembic/alembic.git
        GIT_TAG        c254caf2705ebf5271408dd37a091aa379258a38   # v1.8.10
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(alembic)

    # Alembic 1.8.10 source contains idiomatic C-style casts (TokenMap.cpp,
    # Murmur3.cpp) and implicit-fallthroughs that the project's global
    # -Wall -Wextra -Wpedantic -Wold-style-cast settings flag as warnings.
    # Alembic's own CMakeLists adds -Werror to its targets, so those
    # warnings get promoted to errors when our project flags meet
    # Alembic's -Werror. Override -Werror to keep Alembic warnings as
    # warnings rather than relaxing first-party project flags or rewriting
    # upstream source.
    if(TARGET Alembic)
        target_compile_options(Alembic PRIVATE -Wno-error)
    endif()

    # FetchContent_MakeAvailable should make the Alembic::Alembic target
    # available directly. find_package after MakeAvailable is a belt-and-
    # suspenders verification step that succeeds when the Alembic build
    # has installed its CMake config into the FetchContent build tree.
    find_package(Alembic CONFIG QUIET)
    if(NOT TARGET Alembic::Alembic)
        # In-flight-fix-authorized fallback per Phase 11 spec § 0 hard
        # rule 6. If find_package post-MakeAvailable doesn't resolve but
        # the bare `Alembic` target exists, treat that as the target.
        if(TARGET Alembic)
            add_library(Alembic::Alembic ALIAS Alembic)
        else()
            message(FATAL_ERROR
                "GPU_SIMS_USE_ALEMBIC=ON but Alembic FetchContent failed to produce a target.\n"
                "Verify libimath-dev is installed:  sudo apt install libimath-dev\n"
                "Verify CMake version is >= 3.13:   cmake --version\n"
                "Then re-run CMake. To proceed without Alembic, set GPU_SIMS_USE_ALEMBIC=OFF."
            )
        endif()
    endif()
    message(STATUS "Alembic:   FOUND (1.8.10 via FetchContent at SHA c254caf2)")
endif()
