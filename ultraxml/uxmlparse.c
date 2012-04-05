#include "ultraxml.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
//=============================================================================
//= WIN32 =====================================================================
//=============================================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef UINT8 UTF8;
typedef UINT16 UTF16;
typedef UINT32 UTF32;

#else
//=============================================================================
//= POSIX =====================================================================
//=============================================================================
#include <sys/types.h>
typedef u_int8_t UTF8;
typedef u_int16_t UTF16;
typedef u_int32_t UTF32;
#endif

//=============================================================================
//= Definitions ===============================================================
//=============================================================================

//=============================================================================
//= Helper macros =============================================================
//=============================================================================
#define VALIDATE(_x)(parser->validation & (_x))
#define CURRCHAR()(*(parser->offset))
#define NEXTCHAR()(parser->offset++)

//=============================================================================
//= Forward declarations ======================================================
//=============================================================================
static void setError(struct UXMLPARSER *parser, int code, const char *message);
static UXMLNODE parseElement(struct UXMLPARSER *parser, UXMLNODE parent, int *type);

#define UINLINE __forceinline

static void setError(struct UXMLPARSER *parser, int code, const char *message)
{
  parser->errorMessage = message;
  parser->errorCode = code;
}

UINLINE static int stringCompare(const UXMLCHAR *s1, const UXMLCHAR *s2, int len)
{
  while (*(s1++) == *(s2++))
  {
    len --;

    if (len == 0)
    {
      return 1;
    }
  }

  return 0;
}

UINLINE static int skipWhiteSpace(struct UXMLPARSER *parser)
{
  for (;;)
  {
    switch (*parser->offset)
    {
    case '\0':
      return 0;

    case ' ':
    case '\r':
    case '\n':
    case '\t':
      break;

    default:
      return 1;
    }

    parser->offset ++;
  }
}

UINLINE static int skipName(struct UXMLPARSER *parser)
{
  for (;;)
  {
    switch(*parser->offset)
    {
    case '\0':
      return 0;

      case ' ':
      case '\r':
      case '\n':
      case '\t':
      case '/':
      case '>':
      case '=':
        return 1;

      default:
        break;
    }

    parser->offset ++;
  }
}

UINLINE static int skipUntilWhiteSpaceOrEq(struct UXMLPARSER *parser)
{
  for (;;)
  {
    switch(*parser->offset)
    {
      case '\0':
        return 0;

      case ' ':
      case '\r':
      case '\n':
      case '\t':
      case '=':
        return 1;

      default:
        break;
    }

    parser->offset ++;
  }  

  return 0;
}

UINLINE static int skipUntilEq(struct UXMLPARSER *parser)
{
  for (;;)
  {
    switch(*parser->offset)
    {
    case '\0':
      return 0;
    case '=':
      return 1;
    default:
      break;
    }

    parser->offset ++;
  }

  return 0;
}

UINLINE static int skipUntilDoubleQuote(struct UXMLPARSER *parser)
{
  for (;;)
  {
    switch(*parser->offset)
    {
    case '\0':
      return 0;
    case '\"':
      return 1;
    default:
      break;
    }

    parser->offset ++;
  }
}


UINLINE static int skipUntilSingleQuote(struct UXMLPARSER *parser)
{
  for (;;)
  {
    switch(*parser->offset)
    {
    case '\0':
      return 0;
    case '\'':
      return 1;
    default:
      break;
    }

    parser->offset ++;
  }
}

UINLINE static int skipUntilLt(struct UXMLPARSER *parser)
{
  for (;;)
  {
    switch(*parser->offset)
    {
    case '\0':
      return 0;
    case '<':
      return 1;
    default:
      break;
    }

    parser->offset ++;
  }
}

UINLINE static int parseAttributes(struct UXMLPARSER *parser, UXMLNODE node, int *isEmpty)
{
  for (;;)
  {
    UXMLCHAR *attrName;
    UXMLCHAR *attrValue;
    size_t attrNameLen;

    if (!skipWhiteSpace(parser))
    {
      setError(parser, 0, "Unexpected end of input looking for attribute or stag end");
      return 0;
    }

    if (*parser->offset == '/')
    {
      *isEmpty = 1;
      return 1;
    }
    
    if (*parser->offset == '>')
    {
      *isEmpty = 0;
      return 1;
    }

    attrName = parser->offset;

    if (!skipName(parser))
    {
      setError(parser, 0, "Unexpected end of input parsing attribute name");
      return 0;
    }

    attrNameLen = parser->offset - attrName;

    if (!skipWhiteSpace(parser))
    {
      setError(parser, 0, "Unexpected end of input parsing attribute name");
      return 0;
    }

    if (*(parser->offset++) != '=')
    {
      setError(parser, 0, "Unexpected character when looking for '='");
      return 0;
    }


    if (!skipWhiteSpace(parser))
    {
      setError(parser, 0, "Unexpected end of input parsing attribute name");
      return 0;
    }

    switch (*(parser->offset++))
    {
    case '\"':
      attrValue = parser->offset;
      if (!skipUntilDoubleQuote(parser))
      {
        setError(parser, 0, "Unexpected character when looking for double quote");
        return 0;
      }
      break;
    case '\'':
      attrValue = parser->offset;
      if (!skipUntilSingleQuote(parser))
      {
        setError(parser, 0, "Unexpected character when looking for double quote");
        return 0;
      }
      break;    
    default:
      setError(parser, 0, "Unexpected character when looking for attribute name");
      return 0;
    }

    parser->setAttribute(parser, node, attrName, attrNameLen, attrValue, parser->offset - attrValue);
    parser->offset ++;
  }
}

UINLINE static int parseContent(struct UXMLPARSER *parser, UXMLNODE node, const UXMLCHAR *name, size_t nameLength)
{
  UXMLCHAR *content;
  int type;
  size_t cchContent;

  for (;;)
  {
    if (!skipWhiteSpace(parser))
    {
      setError(parser, 0, "Unexpected end of input");
      return 0;
    }
    
    content = parser->offset;

    if (!skipUntilLt(parser))
    {
      setError(parser, 0, "Unexpected end of input");
      return 0;
    }
    
    cchContent = parser->offset - content;

    if (cchContent > 0)
    {
      parser->createContentNode(parser, node, content, cchContent);
    }

    parser->offset ++;

    //TODO: Validate ahead of offset here, we must not pass beyond parser->end

    if (*(parser->offset) == '/')
    {
      parser->offset ++;
      if (!stringCompare(parser->offset, name, nameLength))
      {
        setError(parser, 0, "Unmatched end tag of element");
        return 0;
      }

      if (!skipName(parser))
      {
        setError(parser, 0, "Unexpected end of input while parsing end tag");
        return 0;
      }

      if (!skipWhiteSpace(parser))
      {
        setError(parser, 0, "Unexpected end of input while parsing end tag");
        return 0;
      }

      if (*(parser->offset) != '>')
      {
        setError(parser, 0, "Unwanted character instead of end of end tag");
        return 0;
      }

      parser->offset ++;
      return 1;
    }

    if (!parseElement(parser, node, &type))
    {
      return 0;
    }
  }
}

UINLINE static UXMLNODE parseComment(struct UXMLPARSER *parser, UXMLNODE parent)
{
  UXMLCHAR *offset = parser->offset;
  UXMLNODE node;

  for (;;)
  {
    if (*offset == '-')
    {
      offset ++;
      if (*offset == '-')
      {
        offset ++;
        if (*offset == '>')
        {
          offset ++;
          node = parser->createCommentNode(parser, parent, parser->offset, offset - parser->offset - 3);
          parser->offset = offset;
          return node;
        }
      }
    }

    if (*offset == '\0')
    {
      break;
    }
  }

  return NULL;
}


UINLINE static UXMLNODE parseCDATA(struct UXMLPARSER *parser, UXMLNODE parent)
{
  UXMLCHAR *offset = parser->offset;
  UXMLNODE node;
  
  for(;;)
  {
    if (*(offset) == ']')
    {
      offset ++;
      if (*(offset) == ']')
      {
        offset ++;
        if (*(offset) == '>')
        {
          offset ++;
          node = parser->createCDATANode(parser, parent, parser->offset, offset - parser->offset - 3);
          parser->offset = offset;
          return node;
        }
      }
    } 

    if (*offset == '\0')
    {
      break;
    }

    offset ++;
  }

  return NULL;
}

UINLINE static UXMLNODE parseProlog(struct UXMLPARSER *parser, UXMLNODE parent)
{
  UXMLNODE node = parser->createElement(parser, parent, NULL, 0);

  for (;;)
  {
    switch (*parser->offset)
    {
    case '\0':
      //TODO: Destroy node here?
      return NULL;
    
    case '>':
      parser->offset ++;
      return node;
    
    default:
      break;
    }

    parser->offset++;
  }
}

UINLINE static UXMLNODE parseDocType(struct UXMLPARSER *parser, UXMLNODE parent)
{
  UXMLNODE node = parser->createElement(parser, parent, NULL, 0);

  for (;;)
  {
    switch (*parser->offset)
    {
    case '\0':
      //TODO: Destroy node here?
      return NULL;
      
    case '>':
      parser->offset ++;
      return node;
    default:
      break;
    }

    parser->offset ++;
  }

}

static UXMLNODE parseElement(struct UXMLPARSER *parser, UXMLNODE parent, int *type)
{
  UXMLNODE node;
  UXMLCHAR *name = parser->offset;
  size_t nameLength;
  int isEmpty;

  if (!skipName(parser))
  {
    setError(parser, 0, "Unexpected end of input parsing stag name");
    return NULL;
  }

  nameLength = (parser->offset - name);

  switch (*name)
  {
  case '!':
    {
      if (stringCompare(name, "![CDATA[", 8))
      {
        *type = UXML_NT_CDATA;
        return parseCDATA(parser, parent);
      }
      else if (stringCompare(name, "!--", 3))
      {
        *type = UXML_NT_COMMENT;
        return parseComment(parser, parent);
      }
      else if (stringCompare(name, "!DOCTYPE", 8))
      {
        *type = UXML_NT_DOCTYPE;
        return parseDocType(parser, parent);
      }
      else
      {
        setError(parser, 0, "Unexpected ! tag");
        return NULL;
      }
      break;
    }

  case '?':
    {
      if (stringCompare(name, "?xml", 4))
      {
        *type = UXML_NT_PROLOG;
        return parseProlog(parser, parent);
      }
      else
      {
        setError(parser, 0, "Unexpected prolog tag");
        return NULL;
      }
      break;
    }

  default:
    {
      *type = UXML_NT_ELEMENT;
      node = parser->createElement(parser, parent, name, nameLength);

      if (!parseAttributes(parser, node, &isEmpty))
      {
        goto ERROR_DESTROY_NODE;
      }

      if (isEmpty)
      {
        parser->offset ++;
        if (*(parser->offset) != '>')
        {
          setError(parser, 0, "Unexpected end of input parsing stag name");
          goto ERROR_DESTROY_NODE;
        }

        parser->offset ++;
      }
      else
      {
        parser->offset ++;

        if (!parseContent(parser, node, name, nameLength))
        {
          goto ERROR_DESTROY_NODE;
        }
      }
      break;
    }
  }

  return node;

ERROR_DESTROY_NODE:
  parser->destroyNode(parser, node);
  return NULL;
}

static UXMLDOCUMENT parseDocument(struct UXMLPARSER *parser)
{
  UXMLNODE node;
  UXMLDOCUMENT document;
  int type;

  document = parser->createDocument(parser);

  //FIXME: Document object might become lost here

  for (;;)
  {
    if (!skipWhiteSpace(parser))
    {
      setError(parser, 0, "End of stream while looking for element start tag '<'");
      return NULL;
    }

    if (*parser->offset != '<')
    {
      setError(parser, 0, "Unexpected input while looking for element start tag '<'");
      return NULL;
    }

    parser->offset ++;

    node = parseElement(parser, NULL, &type);

    if (!node)
    {
      return NULL;
    }

    if (type != UXML_NT_ELEMENT)
    {
      continue;
    }

    parser->setRootNode(parser, document, node);
    break;
  } 

  return document;
}

UXMLDOCUMENT UXML_parse(const UXMLCHAR *stream, struct UXMLPARSER *parser)
{
  parser->offset = (UXMLCHAR *) stream; 
  return parseDocument(parser);
}