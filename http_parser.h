typedef enum {
  hpt_NONE,
  hpt_REQUEST_LINE,
  hpt_HEADER,
  hpt_BODY,
  hpt_END
} http_parser_token_type_t;

typedef struct {
  http_parser_token_type_t type;
  int index;
  int length;
} http_parser_token_t;

typedef enum {
  hpis_COMPLETE,
  hpis_HEADER,
  hpis_HEADER_ALMOST_DONE,
  hpis_HEADER_DONE,
  hpis_HEADERS_ALMOST_COMPLETE,
  hpis_BODY_START,
  hpis_BODY,
  hpis_ERROR
} http_parser_internal_state_t;

typedef enum {
  hphs_START,
  hphs_GENERIC,
  hphs_MATCHING_CONTENT_LENGTH,
  hphs_CONTENT_LENGTH_ALMOST_DONE,
  hphs_CONTENT_LENGTH_WS,
  hphs_CONTENT_LENGTH_VALUE,
  hphs_CONTENT_LENGTH_END_WS
} http_parser_header_state_t;

typedef struct {
  unsigned long header_index;
  http_parser_header_state_t header_state;
  int content_length;
  int body_index;
  http_parser_internal_state_t state;
  http_parser_token_t tokens[64];
  int token_count;
  int token_index;
  int index;
  int request_line;
} http_parser_t;

void http_parser_parse(http_parser_t* parser, char* buffer, int size);
http_parser_token_t* http_parser_tokens(http_parser_t* parser);
void http_parser_init(http_parser_t* parser);
