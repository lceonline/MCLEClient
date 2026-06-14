# Vanilla dedicated server (no plugin support).
#
# The FourKit-enabled flavour is built by Minecraft.Server.FourKit/CMakeLists.txt
# from the same shared source list. Both targets use configure_lce_server_target()
# from cmake/ServerTarget.cmake to apply identical compile/link/asset settings.

include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Common.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/CommonSources.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ServerTarget.cmake")

set(MINECRAFT_SERVER_SOURCES
  ${MINECRAFT_SERVER_COMMON}
  ${SOURCES_COMMON}

  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/UI/UIScene_AchievementsMenu.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/UI/UIControl_AchievementsList.cpp"
    "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/StringUtils.cpp"
)

add_executable(Minecraft.Server ${MINECRAFT_SERVER_SOURCES})
configure_lce_server_target(Minecraft.Server)
