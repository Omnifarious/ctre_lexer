// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include "../parser.hpp"
#include <catch2/catch_all.hpp>
#include <fstream>
#include <filesystem>

SCENARIO("Function declarations and calls")
{
    using Tokens::tokenize_input;
    using Parser::parse_top;

    GIVEN("A simple function declaration and call")
    {
        std::string input = "def add(a, b) { a + b; } add(3, 4);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse succeeds and the call evaluates correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 7);
            }
        }
    }

    GIVEN("Nested function calls")
    {
        std::string input = "def square(x) { x * x; } def cube(x) { x * square(x); } cube(3);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse succeeds and nested calls evaluate correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 27);
            }
        }
    }

    GIVEN("A recursive function (factorial)")
    {
        std::string input =
            "def fact(n) {"
            "  if (n) {"
            "    n * fact(n - 1);"
            "  } else {"
            "    1;"
            "  }"
            "}"
            "fact(5);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The recursive function evaluates correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 120);
            }
        }
    }

    GIVEN("A function identifier used in an expression")
    {
        std::string input = "def f() { 1; } f + 1;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse might succeed but evaluation should handle it (currently asserts)")
            {
                REQUIRE(result != nullptr);
                // In a debug build, this would assert(false) in SimpleEvaluator::operator()(Identifier)
                // because it expects a uintmax_t.
            }
        }
    }

    GIVEN("Assigning to a function identifier")
    {
        std::string input = "def f() { 1; } f = 2; f;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse succeeds and evaluation overwrites the function")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // Current implementation allows overwriting.
                CHECK(result->evaluate() == 2);
            }
        }
    }

    GIVEN("Calling a variable as a function")
    {
        std::string input = "var x = 1; x(2);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse might succeed but evaluation should handle it (currently asserts)")
            {
                REQUIRE(result != nullptr);
                // In a debug build, this would assert in SimpleEvaluator::operator()(FuncCall)
                // because it expects a FuncDeclaration*.
            }
        }
    }

    GIVEN("Function parameter shadowing a global variable")
    {
        std::string input = "var x = 10; def f(x) { x; } f(5);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parameter x should shadow the global x")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 5);
            }
        }
    }

    GIVEN("Function redeclaring a variable in the same scope")
    {
        // This fails because declare_var checks for redeclarations.
        std::string input = "var f = 1; def f() { 2; } f();";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse should fail at the redeclaration")
            {
                REQUIRE(result != nullptr);
                // It should have parsed 'var f = 1;' but stopped at 'def f'
                CHECK(remainder != tokens.end());
            }
        }
    }

    GIVEN("Variable redeclaring a function in the same scope")
    {
        std::string input = "def f() { 1; } var f = 2; f;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse should fail at the redeclaration")
            {
                REQUIRE(result != nullptr);
                CHECK(remainder != tokens.end());
            }
        }
    }

    GIVEN("Function call with mismatched argument count (too many)")
    {
        std::string input = "def f(a) { a; } f(1, 2);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse should fail because of arity mismatch")
            {
                REQUIRE(result != nullptr);
                // It should have parsed the function declaration, but failed at the call
                CHECK(remainder != tokens.end());
            }
        }
    }

    GIVEN("Function call with mismatched argument count (too few)")
    {
        std::string input = "def f(a, b) { a + b; } f(1);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse should fail because of arity mismatch")
            {
                REQUIRE(result != nullptr);
                CHECK(remainder != tokens.end());
            }
        }
    }

    GIVEN("Mutually recursive functions")
    {
        // This currently fails because is_odd is not declared when is_even is parsed.
        std::string input =
            "def is_even(n) { if (n) { is_odd(n - 1); } else { 1; } }"
            "def is_odd(n) { if (n) { is_even(n - 1); } else { 0; } }"
            "is_even(4);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse fails due to forward reference")
            {
                REQUIRE(result != nullptr);
                CHECK(remainder != tokens.end());
            }
        }
    }

    GIVEN("Nested function declarations")
    {
        std::string input =
            "def outer(x) {"
            "  def inner(y) { x + y; }"
            "  inner(x);"
            "}"
            "outer(5);";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Nested functions are NOT supported (must be top level)")
            {
                REQUIRE(result != nullptr);
                // parse_statement has a check: if (ctx.block_stack_.size() >= 2) { cerr << "Function declaration must be at the top level!"; return {nullptr, finish}; }
                CHECK(remainder != tokens.end());
            }
        }
    }
}

SCENARIO("Running sample programs")
{
    namespace fs = std::filesystem;
    const fs::path sample_dir = "../sample_programs";

    const std::vector<std::string> target_programs = {
        "fibiter.tyl",
        "first_func.tyl",
        "nested_calls.tyl",
        "recpow.tyl",
        "simple_loop.tyl"
    };

    for (const auto& filename : target_programs) {
        fs::path path = sample_dir / filename;
        GIVEN("Sample program: " + filename)
        {
            std::ifstream file(path);
            REQUIRE(file.is_open());
            std::string input((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            auto tokens = Tokens::tokenize_input(input.begin(), input.end());

            WHEN("The program is parsed")
            {
                auto [result, remainder] = Parser::parse_top(tokens.begin(), tokens.end());

                THEN("It should parse without errors")
                {
                    REQUIRE(result != nullptr);
                    CHECK(remainder == tokens.end());

                    // We don't necessarily know the expected result for all sample programs,
                    // but we can at least ensure they evaluate without crashing.
                    // Some may have known results we can check.
                    if (filename == "fibiter.tyl") {
                       CHECK(result->evaluate() == 1346269);
                    } else if (filename == "first_func.tyl") {
                       CHECK(result->evaluate() == 11);
                    } else if (filename == "recpow.tyl") {
                       CHECK(result->evaluate() == 531441);
                    } else if (filename == "nested_calls.tyl") {
                       // f(g(5)) = f(125) = 15625
                       // g(f(5)) = g(25) = 15625
                       // f(g(n)) = g(f(n)) evaluates to 1 if equal
                       CHECK(result->evaluate() == 1);
                    } else if (filename == "simple_loop.tyl") {
                       CHECK(result->evaluate() == 0);
                    } else {
                       FAIL("Unrecognized sample program: " + filename);
                    }
                }
            }
        }
    }
}
