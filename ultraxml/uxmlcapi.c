#include "ultraxmlc.h"
#include <string.h>
#include <malloc.h>
#include <crtdbg.h>

#ifdef _WIN32
#include <Windows.h>
#define ALLOCATE(_len) \
  malloc(_len)
  //HeapAlloc(GetProcessHeap(), 0, _len);
  //VirtualAlloc(NULL, _len, MEM_COMMIT, PAGE_READWRITE);
#define FREE(_ptr) \
  free (_ptr)
  //HeapFree(GetProcessHeap(), 0, _ptr);
  //VirtualFree(_ptr, 0, MEM_RELEASE);
#else


#error "Fix for POSIX"
#endif

void *reserve(struct UXMLPARSER *parser, size_t len)
{
  void *ptr;
  UXMLState *state = (UXMLState *) parser->userdata;
  UXMLSlab *slab = state->slab;

  size_t oldLen = (slab->end - slab->start);
  size_t newLen = oldLen * 2;

  if (slab->offset + len >= slab->end)
  {
    while (newLen < len)
    {
      newLen *= 2;
    }

    slab = (UXMLSlab *) ALLOCATE(newLen + sizeof(UXMLSlab));

    slab->start = (char *) (slab+1);
    slab->offset = slab->start;
    slab->end = slab->start + newLen;
    slab->owner = 1;
    slab->next = state->slab;
    state->slab = slab;
  }

  ptr = (void *) slab->offset;
  slab->offset += len;

  return ptr;
}

__forceinline static UXMLNode *allocNode(struct UXMLPARSER *parser, UXMLNODE _parent, int type)
{
  UXMLNode *node = (UXMLNode *) reserve(parser, sizeof(UXMLNode));
  UXMLNode *parent = (UXMLNode *) _parent;
  
  node->type = type;
  node->attributes = NULL;
  node->name.len = 0;
  node->content.len = 0;
  node->parent = parent;
  node->children = NULL;
  
  if (parent)
  {
    UXMLChildRef *ref = (UXMLChildRef *) reserve(parser, sizeof(UXMLChildRef));
    
    ref->child = node;
    ref->next = parent->children;
    parent->children = ref;
  }

  return node;
}

__forceinline static void setString(UXMLString *str, const UXMLCHAR *value, size_t cchValue)
{
  str->ptr = value;
  str->len = cchValue;
}

static UXMLNODE UXMLCAPI_createElement(struct UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *name, size_t cchName)
{
  UXMLNode *node = allocNode(parser, parent, UXML_NT_ELEMENT);
  setString(&node->name, name, cchName);
  return (UXMLNODE) node;
}

static UXMLNODE UXMLCAPI_createContentNode(struct UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *content, size_t cchContent)
{
  UXMLNode *node = allocNode(parser, parent, UXML_NT_CONTENT);
  setString(&node->content, content, cchContent);
  return (UXMLNODE) node;
}

static UXMLNODE UXMLCAPI_createCDATANode(struct UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *content, size_t cchContent)
{
  UXMLNode *node = allocNode(parser, parent, UXML_NT_CDATA);
  setString(&node->content, content, cchContent);
  return (UXMLNODE) node;
}

static UXMLNODE UXMLCAPI_createCommentNode(struct UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *content, size_t cchContent)
{
  UXMLNode *node = allocNode(parser, parent, UXML_NT_COMMENT);
  setString(&node->content, content, cchContent);
  return (UXMLNODE) node;
}

void UXMLCAPI_setAttribute(struct UXMLPARSER *parser, UXMLNODE _node, const UXMLCHAR *_name, size_t cchName, const UXMLCHAR *_value, size_t cchValue)
{
  UXMLNode *node = (UXMLNode *) _node;
  UXMLAttribute *attr = (UXMLAttribute *) reserve(parser, sizeof(UXMLAttribute));

  attr->next = node->attributes;
  setString(&attr->name, _name, cchName);
  setString(&attr->value, _value, cchValue);
  node->attributes = attr;
}

void UXMLCAPI_destroyNode(struct UXMLPARSER *parser, UXMLNODE node)
{

}

void UXMLCAPI_setRootNode(struct UXMLPARSER *parser, UXMLDOCUMENT document, UXMLNODE node)
{
  ((UXMLDocument*) document)->root = (UXMLNode *) node;
}

UXMLDOCUMENT UXMLCAPI_createDocument(struct UXMLPARSER *parser)
{
  return (UXMLDOCUMENT) reserve(parser, sizeof(UXMLDocument));
}

UXMLDocument *UXMLC_parse(UXMLState *state, const UXMLCHAR *stream, size_t cchStream, void *buffer, size_t cbBuffer)
{
  struct UXMLPARSER parser;
  UXMLSlab *slab;

  parser.createDocument = UXMLCAPI_createDocument;
  parser.createCDATANode = UXMLCAPI_createCDATANode;
  parser.createCommentNode = UXMLCAPI_createCommentNode;
  parser.createContentNode = UXMLCAPI_createContentNode;
  parser.createElement = UXMLCAPI_createElement;
  parser.destroyNode = UXMLCAPI_destroyNode;
  parser.setAttribute = UXMLCAPI_setAttribute;
  parser.setRootNode = UXMLCAPI_setRootNode;

  if (cbBuffer < sizeof(UXMLSlab))
  {
    //FIXME: Report error here
    return NULL;
  }

  slab = (UXMLSlab *) buffer;

  slab->start = (char *) (slab+1);
  slab->offset = slab->start;
  slab->end = slab->start + (cbBuffer-sizeof(UXMLSlab));
  slab->next = NULL;
  slab->owner = 0;

  state->slab = slab;
  parser.userdata = (void *) state;
    
  return (UXMLDocument *) UXML_parse(stream, cchStream, &parser);
}

void UXMLC_destroy(UXMLState *state)
{
  UXMLSlab *slab = state->slab;

  while (slab)
  {
    UXMLSlab *next;
    next = slab->next;
    if (slab->owner)
      FREE(slab);
    slab = next;
  }

}

