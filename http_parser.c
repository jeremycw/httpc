#include <string.h>
#include "http_parser.h"

#define CHANGE_STATE(new) \
  do { \
    parser->state = new; \
    if (new == hpis_HEADER) { \
      parser->header_index = 0; \
      parser->header_state = hphs_START; \
    } else if (new == hpis_HEADER_DONE) { \
      if (parser->request_line) { \
        parser->request_line = 0; \
        SET_TOKEN(hpt_REQUEST_LINE, parser->header_index); \
      } else { \
        SET_TOKEN(hpt_HEADER, parser->header_index); \
      } \
    } else if (new == hpis_COMPLETE) { \
      SET_TOKEN(hpt_END, 0) \
    } \
  } while (0)

#define SET_TOKEN(tok_type, size) \
  parser->tokens[parser->token_count].type = tok_type; \
  parser->tokens[parser->token_count].index = parser->token_index; \
  parser->tokens[parser->token_count].length = size; \
  parser->token_count++; \

#define CASE_DIGIT \
  case '0': \
  case '1': \
  case '2': \
  case '3': \
  case '4': \
  case '5': \
  case '6': \
  case '7': \
  case '8': \
  case '9': 

#define CASE_OWS \
  case ' ': \
  case '\t': \
    break;

#define END_HEADER \
  case '\r': \
    CHANGE_STATE(hpis_HEADER_ALMOST_DONE); \
    break; \
  case '\n': \
    CHANGE_STATE(hpis_HEADER_DONE); \
    break; \

#define CONTENT_LENGTH "Content-Length"
http_parser_token_t* http_parser_tokens(http_parser_t* parser) {
  return parser->tokens;
}

void http_parser_init(http_parser_t* parser) {
  memset(parser, 0, sizeof(http_parser_t));
}

void http_parser_parse(http_parser_t* parser, char* buffer, int size) {
  parser->token_count = 0;
  for (int i = 0; i < size; i++) {
    char c = buffer[i];
    switch (parser->state) {

    case hpis_HEADER:
      switch (parser->header_state) {

      case hphs_START:
START:
        parser->token_index = parser->index;
        if (c == 'C') {
          parser->header_state = hphs_MATCHING_CONTENT_LENGTH;
        } else {
          parser->header_state = hphs_GENERIC;
        }
        break;

      case hphs_GENERIC:
        switch (c) {
          END_HEADER
        }
        break;

      case hphs_MATCHING_CONTENT_LENGTH:
        if (c != CONTENT_LENGTH[parser->header_index]
            || parser->header_index > sizeof(CONTENT_LENGTH)-1)
        {
          parser->header_state = hphs_GENERIC;
        } else if (parser->header_index == sizeof(CONTENT_LENGTH)-2) {
          parser->header_state = hphs_CONTENT_LENGTH_ALMOST_DONE;
        }
        break;

      case hphs_CONTENT_LENGTH_ALMOST_DONE:
        switch (c) {
        case ':':
          parser->header_state = hphs_CONTENT_LENGTH_WS;
          break;
        CASE_OWS
        default:
          parser->header_state = hphs_GENERIC;
        }
        break;

      case hphs_CONTENT_LENGTH_WS:
        switch (c) {
          CASE_OWS
          CASE_DIGIT
            parser->content_length = c - '0';
            parser->header_state = hphs_CONTENT_LENGTH_VALUE;
            break;
          default:
            CHANGE_STATE(hpis_ERROR);
        }
        break;

      case hphs_CONTENT_LENGTH_VALUE:
        switch (c) {
          CASE_DIGIT
            parser->content_length *= 10;
            parser->content_length += c - '0';
            break;
          END_HEADER
          case ' ':
          case '\t':
            parser->header_state = hphs_CONTENT_LENGTH_END_WS;
            break;
          default:
            CHANGE_STATE(hpis_ERROR);
        }
        break;

      case hphs_CONTENT_LENGTH_END_WS:
        switch (c) {
          CASE_OWS
          END_HEADER
        }
      }
      parser->header_index++;
      break;

    case hpis_HEADER_ALMOST_DONE:
      if (c == '\n') {
        CHANGE_STATE(hpis_HEADER_DONE);
      } else {
        CHANGE_STATE(hpis_HEADER);
      }
      break;

    case hpis_HEADER_DONE:
      if (c == '\n') {
        if (parser->content_length > 0) {
          CHANGE_STATE(hpis_BODY_START);
        } else {
          CHANGE_STATE(hpis_COMPLETE);
        }
      } else if (c == '\r') {
        CHANGE_STATE(hpis_HEADERS_ALMOST_COMPLETE);
      } else {
        CHANGE_STATE(hpis_HEADER);
        parser->header_state = hphs_START;
        goto START;
      }
      break;

    case hpis_HEADERS_ALMOST_COMPLETE:
      if (c == '\n') {
        if (parser->content_length > 0) {
          CHANGE_STATE(hpis_BODY_START);
        } else {
          CHANGE_STATE(hpis_COMPLETE);
        }
      } else {
        CHANGE_STATE(hpis_HEADER);
        parser->header_state = hphs_START;
        goto START;
      }
      break;

    case hpis_BODY_START:
      parser->token_index = parser->index;
      parser->body_index = parser->content_length - 1;
      CHANGE_STATE(hpis_BODY);
      if (parser->body_index == 0) {
        SET_TOKEN(hpt_BODY, parser->content_length);
        CHANGE_STATE(hpis_COMPLETE);
      }
      break;

    case hpis_BODY:
      parser->body_index--;
      if (parser->body_index == 0) {
        SET_TOKEN(hpt_BODY, parser->content_length);
        CHANGE_STATE(hpis_COMPLETE);
      }
      break;

    case hpis_COMPLETE:
      switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
          break;
        default:
          parser->content_length = 0;
          parser->request_line = 1;
          CHANGE_STATE(hpis_HEADER);
          parser->header_state = hphs_START;
          goto START;
      }
      break;

    default:
      break;
    }
    parser->index++;
  }
}

#ifdef HTTP_TEST

#include <stdio.h>

#define REQ "  GET /test HTTP/1.1 \r\nHost: www.foo.com\r\nContent-Length: 11 \r\n\r\nHi123456789\n\nPOST /foo HTTP/1.1\nHost: www.example.com\nContent-Length: 2\n\nHi"
int main() {
  char* req = REQ;
  http_parser_t parser;
  memset(&parser, 0, sizeof(http_parser_t));
  http_parser_parse(&parser, REQ, sizeof(REQ)-22);
  for (int i = 0; i < parser.token_count; i++) {
    http_parser_token_t tok = parser.tokens[i];
    printf("%d %.*s\n", tok.type, tok.length, req + tok.index);
  }
  http_parser_parse(&parser, req + sizeof(REQ)-22, 21);
  for (int i = 0; i < parser.token_count; i++) {
    http_parser_token_t tok = parser.tokens[i];
    printf("%d %.*s\n", tok.type, tok.length, req + tok.index);
  }
}

#endif
