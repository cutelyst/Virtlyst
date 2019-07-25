file(GLOB_RECURSE Virtlyst_SRCS *.cpp *.h lib/*.cpp lib/*.h)

set(Virtlyst_SRCS
    ${Virtlyst_SRCS}
    ${TEMPLATES_SRC}
)

# Create the application
add_library(Virtlyst SHARED ${Virtlyst_SRCS})

# Make sure we link with libvirt installed in weird places, cough macos
target_include_directories(Virtlyst PRIVATE ${LIBVIRT_LIBRARY_DIRS})

# Link to Cutelyst
target_link_libraries(Virtlyst
    Cutelyst::Core
    Cutelyst::View::Grantlee
    Cutelyst::Utils::Sql
    Cutelyst::Authentication
    Cutelyst::StatusMessage
    Cutelyst::Session
    Grantlee5::Templates
    Qt5::Core
    Qt5::Network
    Qt5::Sql
    Qt5::Xml
    ${LIBVIRT_LIBRARIES}
)

