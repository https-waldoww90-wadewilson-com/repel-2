
#include <vector>
#include <iostream>
#include <string>
#include "follexer.h"
#include "foltoken.h"

std::vector<FOLToken> FOLParse::tokenize(std::istream* input) {
  std::vector<FOLToken> tokens; 
  while (!input->eof()) {
    int c = input->get();
    if (input->eof()) {
        break;
    }
    FOLToken token; 
    // first check for identifiers
    if (c >= 'A' && c <= 'Z'
        || c >= 'a' && c <= 'z'
        || c == '_') {
      std::string ident;
      ident.push_back(c);
      while (!input->eof()) {
        c = input->peek();
        if (c >= 'A' && c <= 'Z'
          || c >= 'a' && c <= 'z'
          || c >= '0' && c <= '9'
          || c == '_') {
          input->get();
          ident.push_back(c);
        } else {
          break;
        }
      }
      token.setType(FOLParse::IDENT);
      token.setContents(ident);
      tokens.push_back(token);
    } else if (c >= '0' && c <= '9') {
      std::string num;
      num.push_back(c);
      while (!input->eof()) {
        c = input->peek();
        if (c >= '0' && c <= '9') {
          input->get();
          num.push_back(c);
        } else {
          break;
        }
      }
      token.setType(FOLParse::NUMBER);
      token.setContents(num);
      tokens.push_back(token); 
    } else {
      switch (c) {
        case '[':
          token.setType(FOLParse::OPEN_BRACKET);
          token.setContents("[");
          tokens.push_back(token);
          break;
        case ']':
          token.setType(FOLParse::CLOSE_BRACKET);
          token.setContents("]");
          tokens.push_back(token);
          break;
        case '(':
          token.setType(FOLParse::OPEN_PAREN);
          token.setContents("(");
          tokens.push_back(token);
          break;
        case ')':
          token.setType(FOLParse::CLOSE_PAREN);
          token.setContents(")");
          tokens.push_back(token);
          break;
        case ',':
          token.setType(FOLParse::COMMA);
          token.setContents(",");
          tokens.push_back(token);
          break;
        case ':':
          token.setType(FOLParse::COLON);
          token.setContents(":");
          tokens.push_back(token);
          break;
        case '@':
          token.setType(FOLParse::AT);
          token.setContents("@");
          tokens.push_back(token);
          break;
        case '\n':
          token.setType(FOLParse::ENDL);
          token.setContents("\n");
          tokens.push_back(token);
        case '#':
          // comment, do nothing until we get to endl
          while (!input->eof()) {
            c = input->peek();
            if (c != '\n') {
              input->get();
            } else {
              break;
            }
          }
        case ' ':
        case '\t':
        case '\r':
          // do nothing!
          break;
        default:
          std::cerr << "dont know what " << c << " is" << std::endl;
          // error!
      }
    }
  }

  return tokens;
}