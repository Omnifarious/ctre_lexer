// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include "../tokens.hpp"

#include <catch2/catch_all.hpp>

SCENARIO("Fixed string generates expected list of tokens.")
{
    using Tokens::tokenize_input;
    GIVEN("A simple mathematical expression string")
    {
        std::string input = "x = 42 + 3 * y;";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("The correct tokens are generated")
            {
                REQUIRE(tokens.size() == 8);
                
                // Check identifier "x"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[0]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[0]).value_ == "x");
                
                // Check equal sign "="
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[1]));
                REQUIRE(::std::get<Tokens::Operator>(tokens[1]).value_ == Tokens::Operator::Equal);
                
                // Check decimal number "42"
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[2]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[2]).value_ == 42);
                
                // Check plus operator "+"
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[3]));
                REQUIRE(std::get<Tokens::Operator>(tokens[3]).value_ == Tokens::Operator::Plus);
                
                // Check decimal number "3"
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[4]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[4]).value_ == 3);
                
                // Check multiply operator "*"
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[5]));
                REQUIRE(std::get<Tokens::Operator>(tokens[5]).value_ == Tokens::Operator::Multiply);
                
                // Check identifier "y"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[6]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[6]).value_ == "y");

                // Check semicolon
                REQUIRE(std::holds_alternative<Tokens::Semicolon>(tokens[7]));
            }
        }
    }
    
    GIVEN("A string with different number formats")
    {
        std::string input = "0x1A + 0755 + 123";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("Different number formats are recognized with correct values")
            {
                REQUIRE(tokens.size() == 5);
                
                // Check hexadecimal "0x1A" -> 26
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[0]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[0]).value_ == 0x1A);
                
                // Check plus operator "+"
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[1]));
                REQUIRE(std::get<Tokens::Operator>(tokens[1]).value_ == Tokens::Operator::Plus);
                
                // Check octal "0755" -> 493
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[2]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[2]).value_ == 0755);
                
                // Check plus operator "+"
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[3]));
                REQUIRE(std::get<Tokens::Operator>(tokens[3]).value_ == Tokens::Operator::Plus);
                
                // Check decimal "123"
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[4]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[4]).value_ == 123);
            }
        }
    }
    
    GIVEN("A string with parentheses and semicolon")
    {
        std::string input = "(a - b);";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("Parentheses and semicolon are correctly tokenized")
            {
                REQUIRE(tokens.size() == 6);
                
                // Check open paren "("
                REQUIRE(std::holds_alternative<Tokens::Paren>(tokens[0]));
                REQUIRE(std::get<Tokens::Paren>(tokens[0]).value_ == Tokens::Paren::Open);
                
                // Check identifier "a"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[1]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[1]).value_ == "a");
                
                // Check minus operator "-"
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[2]));
                REQUIRE(std::get<Tokens::Operator>(tokens[2]).value_ == Tokens::Operator::Minus);
                
                // Check identifier "b"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[3]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[3]).value_ == "b");
                
                // Check close paren ")"
                REQUIRE(std::holds_alternative<Tokens::Paren>(tokens[4]));
                REQUIRE(std::get<Tokens::Paren>(tokens[4]).value_ == Tokens::Paren::Close);

                // Check semicolon
                REQUIRE(std::holds_alternative<Tokens::Semicolon>(tokens[5]));
            }
        }
    }
    
    GIVEN("A string with invalid tokens")
    {
        std::string input = "x = 42 @ y;";  // @ is not a valid token
        
        WHEN("The string is tokenized")
        {
            THEN("A parse_error exception is thrown")
            {
                using Tokens::tokenize_error;
                REQUIRE_THROWS_AS(tokenize_input(input.begin(), input.end()), tokenize_error);
            }
            
            AND_THEN("The exception message contains the unexpected token")
            {
                REQUIRE_THROWS_WITH(
                    tokenize_input(input.begin(), input.end()),
                    Catch::Matchers::ContainsSubstring("Unexpected token:  @")
                );
            }
        }
    }

    GIVEN("A string with a boolean operators")
    {
        std::string input = "x = true && false || true;";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The correct tokens are generated")
            {
                REQUIRE(tokens.size() == 8);

                // Check identifier "x"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[0]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[0]).value_ == "x");

                // Check equal sign "="
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[1]));
                REQUIRE(::std::get<Tokens::Operator>(tokens[1]).value_ == Tokens::Operator::Equal);

                // Check identifer "true"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[2]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[2]).value_ == "true");

                // Check boolean operator BoolAnd
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[3]));
                REQUIRE(std::get<Tokens::Operator>(tokens[3]).value_ == Tokens::Operator::BoolAnd);

                // Check identifer "false"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[4]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[4]).value_ == "false");

                // Check boolean operator BoolOr
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[5]));
                REQUIRE(std::get<Tokens::Operator>(tokens[5]).value_ == Tokens::Operator::BoolOr);

                // Check identifer "true"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[6]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[6]).value_ == "true");

                // Check semicolon
                REQUIRE(std::holds_alternative<Tokens::Semicolon>(tokens[7]));
            }
        }
    }

    GIVEN("A string with curly brackets for code blocks")
    {
        std::string input = "{ x = 42; }";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The curly brackets are correctly tokenized")
            {
                REQUIRE(tokens.size() == 6);

                // Check open curly bracket "{"
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[0]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[0]).value_ == Tokens::CurlyBracket::Open);

                // Check identifier "x"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[1]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[1]).value_ == "x");

                // Check equal sign "="
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[2]));
                REQUIRE(::std::get<Tokens::Operator>(tokens[2]).value_ == Tokens::Operator::Equal);

                // Check decimal number "42"
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[3]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[3]).value_ == 42);

                // Check semicolon
                REQUIRE(std::holds_alternative<Tokens::Semicolon>(tokens[4]));

                // Check close curly bracket "}"
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[5]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[5]).value_ == Tokens::CurlyBracket::Close);
            }
        }
    }

    GIVEN("A string with if-else keywords")
    {
        std::string input = "if (x) y = 1; else y = 0;";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The keywords are correctly tokenized")
            {
                REQUIRE(tokens.size() == 13);

                // Check "if" keyword
                REQUIRE(std::holds_alternative<Tokens::Keyword>(tokens[0]));
                REQUIRE(std::get<Tokens::Keyword>(tokens[0]).value_ == Tokens::Keyword::If);

                // Check open paren "("
                REQUIRE(std::holds_alternative<Tokens::Paren>(tokens[1]));
                REQUIRE(std::get<Tokens::Paren>(tokens[1]).value_ == Tokens::Paren::Open);

                // Check identifier "x"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[2]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[2]).value_ == "x");

                // Check close paren ")"
                REQUIRE(std::holds_alternative<Tokens::Paren>(tokens[3]));
                REQUIRE(std::get<Tokens::Paren>(tokens[3]).value_ == Tokens::Paren::Close);

                // Check identifier "y"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[4]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[4]).value_ == "y");

                // Check equal sign "="
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[5]));
                REQUIRE(::std::get<Tokens::Operator>(tokens[5]).value_ == Tokens::Operator::Equal);

                // Check decimal number "1"
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[6]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[6]).value_ == 1);

                // Check semicolon
                REQUIRE(std::holds_alternative<Tokens::Semicolon>(tokens[7]));

                // Check "else" keyword
                REQUIRE(std::holds_alternative<Tokens::Keyword>(tokens[8]));
                REQUIRE(std::get<Tokens::Keyword>(tokens[8]).value_ == Tokens::Keyword::Else);

                // Check identifier "y"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[9]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[9]).value_ == "y");

                // Check equal sign "="
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[10]));
                REQUIRE(::std::get<Tokens::Operator>(tokens[10]).value_ == Tokens::Operator::Equal);

                // Check decimal number "0"
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[11]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[11]).value_ == 0);

                // Check semicolon
                REQUIRE(std::holds_alternative<Tokens::Semicolon>(tokens[12]));
            }
        }
    }

    GIVEN("A string with keywords that should not be confused with identifiers")
    {
        std::string input = "ifx = 1; elsif = 2; if_var = 3;";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("Keywords with additional characters are treated as identifiers")
            {
                REQUIRE(tokens.size() == 12);

                // Check identifier "ifx" (not keyword)
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[0]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[0]).value_ == "ifx");

                // Skip equal and number...

                // Check identifier "elsif" (not keyword)
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[4]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[4]).value_ == "elsif");

                // Skip equal and number...

                // Check identifier "if_var" (not keyword)
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[8]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[8]).value_ == "if_var");
            }
        }
    }

    GIVEN("A string with nested curly brackets")
    {
        std::string input = "{ { x = 1; } }";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("Nested curly brackets are correctly tokenized")
            {
                REQUIRE(tokens.size() == 8);

                // Check outer open curly bracket "{"
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[0]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[0]).value_ == Tokens::CurlyBracket::Open);

                // Check inner open curly bracket "{"
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[1]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[1]).value_ == Tokens::CurlyBracket::Open);

                // Check identifier "x"
                REQUIRE(std::holds_alternative<Tokens::Identifier>(tokens[2]));
                REQUIRE(std::get<Tokens::Identifier>(tokens[2]).value_ == "x");

                // Check equal sign "="
                REQUIRE(std::holds_alternative<Tokens::Operator>(tokens[3]));
                REQUIRE(::std::get<Tokens::Operator>(tokens[3]).value_ == Tokens::Operator::Equal);

                // Check decimal number "1"
                REQUIRE(std::holds_alternative<Tokens::UnsignedInteger>(tokens[4]));
                REQUIRE(std::get<Tokens::UnsignedInteger>(tokens[4]).value_ == 1);

                // Check semicolon
                REQUIRE(std::holds_alternative<Tokens::Semicolon>(tokens[5]));

                // Check inner close curly bracket "}"
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[6]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[6]).value_ == Tokens::CurlyBracket::Close);

                // Check outer close curly bracket "}"
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[7]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[7]).value_ == Tokens::CurlyBracket::Close);
            }
        }
    }

    GIVEN("A complete if-else statement with blocks")
    {
        std::string input = "if (x + 0) { result = 1; } else { result = 0; }";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("All tokens including keywords and brackets are correctly identified")
            {
                REQUIRE(tokens.size() == 19);

                // Check "if" keyword
                REQUIRE(std::holds_alternative<Tokens::Keyword>(tokens[0]));
                REQUIRE(std::get<Tokens::Keyword>(tokens[0]).value_ == Tokens::Keyword::If);

                // Skip condition tokens...

                // Check open curly bracket for if-block
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[6]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[6]).value_ == Tokens::CurlyBracket::Open);

                // Skip assignment tokens...

                // Check close curly bracket for if-block
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[11]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[11]).value_ == Tokens::CurlyBracket::Close);

                // Check "else" keyword
                REQUIRE(std::holds_alternative<Tokens::Keyword>(tokens[12]));
                REQUIRE(std::get<Tokens::Keyword>(tokens[12]).value_ == Tokens::Keyword::Else);

                // Check open curly bracket for else-block
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[13]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[13]).value_ == Tokens::CurlyBracket::Open);

                // Skip assignment tokens...

                // Check close curly bracket for else-block
                REQUIRE(std::holds_alternative<Tokens::CurlyBracket>(tokens[18]));
                REQUIRE(std::get<Tokens::CurlyBracket>(tokens[18]).value_ == Tokens::CurlyBracket::Close);
            }
        }
    }

    GIVEN("An empty string")
    {
        std::string input = "";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("No tokens are generated")
            {
                REQUIRE(tokens.empty());
            }
        }
    }

    GIVEN("A string with only whitespace")
    {
        std::string input = "   \t  \n  ";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("No tokens are generated")
            {
                REQUIRE(tokens.empty());
            }
        }
    }
}