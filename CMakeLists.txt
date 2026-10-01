cmake_minimum_required(VERSION 3.15)
project(Classix LANGUAGES C CXX OBJC OBJCXX)

# Definição do padrão C++ moderno suportado pelo Clang/Darling
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Definições globais herdadas do projeto original
add_compile_definitions(TODOS=1 ENABLE_ANTIALIASING=1)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG=1 DEBUG_DISASSEMBLE=1)
endif()

# Variáveis de caminhos para Instalação no Darling/Linux
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_compile_options(-fPIC -Wno-objc-root-class -Wno-deprecated-declarations)
    
    # Caminhos padrão do GNUstep no ecossistema Darling
    set(GNUSTEP_ROOT "/usr/GNUstep" CACHE PATH "Caminho base do GNUstep")
    include_directories(${GNUSTEP_ROOT}/System/Library/Headers /usr/include/GNUstep /usr/local/include)
    link_directories(${GNUSTEP_ROOT}/System/Library/Libraries)
    
    set(CMAKE_INSTALL_BINDIR "bin")
    set(CMAKE_INSTALL_LIBDIR "lib")
else()
    include(GNUInstallDirs)
endif()

# ==============================================================================
# MACRO: Compilação Automática de Interface (.xib -> .nib) via ibtool
# ==============================================================================
macro(DARLING_COMPILE_XIB XIB_FILE OUTPUT_NIB_DIR)
    get_filename_component(XIB_NAME ${XIB_FILE} NAME_WE)
    set(NIB_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NIB_DIR}/${XIB_NAME}.nib")
    
    find_program(IBTOOL_EXECUTABLE NAMES ibtool)
    if(IBTOOL_EXECUTABLE)
        add_custom_command(
            OUTPUT ${NIB_OUTPUT_PATH}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NIB_DIR}"
            COMMAND ${IBTOOL_EXECUTABLE} --errors --warnings --notices --output-format human-readable-text --compile ${NIB_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/${XIB_FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${XIB_FILE}
            COMMENT "Compilando interface gráfica: ${XIB_FILE} -> ${XIB_NAME}.nib"
        )
    endif()
endmacro()

# ==============================================================================
# 1. ALVO: ClassixCore (Incluindo a nova pasta MachO)
# ==============================================================================
# O GLOB_RECURSE vai apanhar automaticamente todos os *.cpp e *.h de ClassixCore/MachO/*
file(GLOB_RECURSE CLASSIX_CORE_SRCS 
    "ClassixCore/*.cpp"
    "ClassixCore/*.h"
)
add_library(ClassixCore SHARED ${CLASSIX_CORE_SRCS})

# Expõe as diretorias de inclusão para o resto do projeto
target_include_directories(ClassixCore PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/ClassixCore
    ${CMAKE_CURRENT_SOURCE_DIR}/ClassixCore/MachO
)

if(APPLE OR NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_library(CORE_FOUNDATION CoreFoundation)
    target_link_libraries(ClassixCore PRIVATE ${CORE_FOUNDATION})
else()
    target_link_libraries(ClassixCore PRIVATE -lgnustep-base)
endif()

# ==============================================================================
# 2. ALVO: Bibliotecas Auxiliares (StdCLib, MathLib, ThreadsLib, ControlStripLib)
# ==============================================================================
add_library(StdCLib SHARED Libraries/StdCLib/StdCLib.cpp Libraries/StdCLib/StdCLibSymbols.cpp)
target_link_libraries(StdCLib PRIVATE ClassixCore)

add_library(MathLib SHARED Libraries/MathLib/MathLib.cpp Libraries/MathLib/MathLibSymbols.cpp)
target_link_libraries(MathLib PRIVATE ClassixCore)

add_library(ThreadsLib SHARED Libraries/ThreadsLib/ThreadsLib.cpp Libraries/ThreadsLib/ThreadsLibSymbols.cpp)
target_link_libraries(ThreadsLib PRIVATE ClassixCore)

add_library(ControlStripLib MODULE Libraries/ControlStripLib/ControlStripLib.cpp Libraries/ControlStripLib/ControlStripLibSymbols.cpp)
target_link_libraries(ControlStripLib PRIVATE ClassixCore)

# ==============================================================================
# NOVA PASTA NA RAIZ: OpenTransportLib
# ==============================================================================
file(GLOB OPEN_TRANSPORT_SRCS 
    "OpenTransportLib/*.cpp"
    "OpenTransportLib/*.h"
)
add_library(OpenTransportLib SHARED ${OPEN_TRANSPORT_SRCS})
target_include_directories(OpenTransportLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/OpenTransportLib)
target_link_libraries(OpenTransportLib PRIVATE ClassixCore)

# ==============================================================================
# 3. ALVO: CarbonLib
# ==============================================================================
file(GLOB INTERFACE_LIB_SRCS 
    "CarbonLib/CarbonLib.cpp"
    "CarbonLib/AEDataModel.cpp"
    "CarbonLib/AEInteraction.cpp"
    "CarbonLib/Aliases.cpp"
    "CarbonLib/AppleEvents.cpp"
    "CarbonLib/GrafPortManager.cpp"
    "CarbonLib/UIChannel.cpp"
    "CarbonLib/ResourceTypes.cpp"
)
add_library(CarbonLib SHARED ${INTERFACE_LIB_SRCS})
target_include_directories(CarbonLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/CarbonLib)
target_link_libraries(CarbonLib PUBLIC ClassixCore)

# ==============================================================================
# 4. ALVO: Classix (Ferramenta CLI Principal)
# ==============================================================================
file(GLOB CLASSIX_TOOL_SRCS 
    "Classix/main.cpp"
    "Classix/VirtualMachine.cpp"
    "Classix/CompareTrace.cpp"
    "Classix/DebugStub.cpp"
    "Classix/ThreadContext.cpp"
    "Classix/Breakpoint.cpp"
)
add_executable(Classix ${CLASSIX_TOOL_SRCS})
# Adicionada a OpenTransportLib à árvore de dependências do binário final
target_link_libraries(Classix PRIVATE 
    ClassixCore 
    StdCLib 
    MathLib 
    ThreadsLib 
    OpenTransportLib 
    CarbonLib 
    ControlStripLib
)

# ==============================================================================
# 5. ALVO: ClassixMenuServer (Servidor Cocoa Gráfico)
# ==============================================================================
file(GLOB COCOA_SERVER_SRCS
    "CarbonLibHead/main.m"
    "CarbonLibHead/CXILApplication.mm"
    "CarbonLibHead/CXILWindowDelegate.mm"
    "CarbonLibHead/CXIOSurfaceView.m"
)

# Compilar o MainMenu.xib
DARLING_COMPILE_XIB("CarbonLibHead/en.lproj/MainMenu.xib" "Resources")

add_executable(ClassixMenuServer ${COCOA_SERVER_SRCS} "${CMAKE_CURRENT_BINARY_DIR}/Resources/MainMenu.nib")

if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOSURFACE_FRAMEWORK IOSurface)
    target_link_libraries(ClassixMenuServer PRIVATE ${COCOA_FRAMEWORK} ${IOSURFACE_FRAMEWORK} CarbonLib)
else()
    target_link_libraries(ClassixMenuServer PRIVATE 
        -lobjc            
        -lgnustep-gui     
        -lgnustep-base    
        CarbonLib      
    )
endif()

# Injeta a pasta de recursos na diretoria local após a compilação
add_custom_command(
    TARGET ClassixMenuServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ClassixMenuServer>/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/Resources/MainMenu.nib" "$<TARGET_FILE_DIR:ClassixMenuServer>/Resources/MainMenu.nib"
    COMMENT "A injetar recursos locais do Cocoa (.nib) na pasta de compilação"
)

# ==============================================================================
# 6. CONFIGURAÇÃO DE INSTALAÇÃO NO SISTEMA (Darling /usr/local)
# ==============================================================================
install(TARGETS Classix ClassixMenuServer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS ClassixCore CarbonLib StdCLib MathLib ThreadsLib OpenTransportLib ControlStripLib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Resources/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/Resources"
    FILES_MATCHING PATTERN "*.nib"
)
