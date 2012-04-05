#ifndef __ULTRAXML_H__
#define __ULTRAXML_H__

#include <stdio.h>
#include <malloc.h>

typedef char UXMLCHAR;
typedef void * UXMLNODE;
typedef void * UXMLDOCUMENT;

enum UXML_NODETYPES
{
  UXML_NT_ELEMENT,
  UXML_NT_COMMENT,
  UXML_NT_PROLOG,
  UXML_NT_DOCTYPE,
  UXML_NT_CDATA,
  UXML_NT_CONTENT,
};

struct UXMLPARSER
{
  /* Private */
  UXMLCHAR *offset;

  /* ABI */
  UXMLDOCUMENT (*createDocument)(struct UXMLPARSER *parser);

  UXMLNODE (*createElement)(struct UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *name, size_t cchName);
  UXMLNODE (*createContentNode)(struct UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *content, size_t cchContent);
  UXMLNODE (*createCDATANode)(struct UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *content, size_t cchContent);
  UXMLNODE (*createCommentNode)(struct UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *content, size_t cchContent);

  void (*setAttribute)(struct UXMLPARSER *parser, UXMLNODE node, const UXMLCHAR *_name, size_t cchName, const UXMLCHAR *_value, size_t cchValue);
  void (*destroyNode)(struct UXMLPARSER *parser, UXMLNODE node);

  void (*setRootNode)(struct UXMLPARSER *parser, UXMLDOCUMENT document, UXMLNODE node);

  /* Options */
  int validation;

  /* Private */
  UXMLCHAR *start;
  UXMLCHAR *end;

  const char *errorMessage;
  int errorCode;

  /* User */
  void *userdata;
  UXMLDOCUMENT document;
};

UXMLDOCUMENT UXML_parse(const UXMLCHAR *stream, size_t cbStream, struct UXMLPARSER *parser);

#endif