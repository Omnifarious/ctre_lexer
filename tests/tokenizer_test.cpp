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
                REQUIRE(std::holds_alternative<Tokens::Equal>(tokens[1]));
                
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