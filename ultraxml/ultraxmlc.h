#ifndef __ULTRAXMLC_H__
#define __ULTRAXMLC_H__

#include "ultraxml.h"

struct __UXMLNode;

typedef struct __UXMLString
{
  const UXMLCHAR *ptr;
  size_t len;
} UXMLString;

typedef struct __UXMLAttribute
{
  UXMLString name;
  UXMLString value;
  struct __UXMLAttribute *next;
} UXMLAttribute;

typedef struct __UXMLChildRef
{
  struct __UXMLNode *child;
  struct __UXMLChildRef *next;
} UXMLChildRef;


typedef struct __UXMLNode
{
  UXMLString name;
  UXMLString content;
  int type;
  struct __UXMLNode *parent;
  struct __UXMLChildRef *children;
  struct __UXMLAttribute *attributes;

} UXMLNode;

typedef struct __UXMLDocument
{
  UXMLNode *prolog;
  UXMLNode *doctype;
  UXMLNode *root;
} UXMLDocument;

typedef struct __UXMLSlab
{
  char *start;
  char *offset;
  char *end;
  int owner;
  struct __UXMLSlab *next;
} UXMLSlab;

typedef struct __UXMLState
{
  UXMLSlab *slab;
} UXMLState;

UXMLDocument *UXMLC_parse(UXMLState *state, const UXMLCHAR *stream, void *buffer, size_t cbBuffer);
void UXMLC_destroy(UXMLState *state);

#endif
