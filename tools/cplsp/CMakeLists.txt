# CPLSP — CP 语言语义级 LSP 服务器
add_executable(cplsp
    tools/cplsp/cplsp_main.cpp
    tools/cplsp/main.cpp
    tools/cplsp/document_manager.cpp
    tools/cplsp/lsp_diagnostics.cpp
    tools/cplsp/lsp_completion.cpp
    tools/cplsp/lsp_definition.cpp
    tools/cplsp/lsp_hover.cpp
    tools/cplsp/lsp_references.cpp
)

target_include_directories(cplsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/cplsp
)

target_link_libraries(cplsp PRIVATE
    cplang_lexer
    cplang_parser
    cplang_semantic
    cplang_ast
    cplang_token
    cplang_common
)

set_target_warnings(cplsp)
