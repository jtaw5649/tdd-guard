project('tdd-guard-cpp', 'cpp',
    version: '0.1.0',
    license: 'MIT',
    default_options: [
        'cpp_std=c++2c',
        'warning_level=3',
        'werror=true',
        'b_lto=true',
    ]
)

nlohmann_json_dep = dependency('nlohmann_json',
    fallback: ['nlohmann_json', 'nlohmann_json_dep'],
    required: true
)

src_files = files(
    'src/main.cpp',
    'src/error_parser.cpp',
    'src/parser.cpp',
    'src/transformer.cpp',
)

tdd_guard_cpp = executable('tdd-guard-cpp',
    src_files,
    dependencies: [nlohmann_json_dep],
    install: true,
)

if get_option('tests')
    catch2_dep = dependency('catch2-with-main',
        fallback: ['catch2', 'catch2_with_main_dep'],
        required: true
    )

    test_files = files(
        'test/main_test.cpp',
        'test/error_parser_test.cpp',
        'test/parser_test.cpp',
        'test/transformer_test.cpp',
    )

    src_without_main = files(
        'src/error_parser.cpp',
        'src/parser.cpp',
        'src/transformer.cpp',
    )

    test_exe = executable('tdd-guard-cpp-tests',
        [test_files, src_without_main],
        dependencies: [catch2_dep, nlohmann_json_dep],
    )

    test('unit-tests', test_exe)
endif
