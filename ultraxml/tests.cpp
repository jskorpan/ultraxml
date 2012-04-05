#include <string>
#include <list>
#include <map>
#include <stdio.h>
#include <Windows.h>

extern "C"
{
  #include "ultraxml.h"
  #include "ultraxmlc.h"
}

#include <string.h>

using namespace std;

class Element
{
public:
  static int m_numAttributes;
  static int m_numElements;

public:

  Element(Element *parent, int type, const string &_name)
  {
    m_parent = parent;
    m_name = _name;
    m_type = type;
  }

  ~Element(void)
  {
  }

  void destroy()
  {
    for (list<Element *>::iterator iter = m_children.begin(); iter != m_children.end(); iter ++)
    {
      (*iter)->destroy();
    }

    delete this;
  }
  

  void setAttribute(const string &_name, const string &_value)
  {
    m_attributes[_name] = _value;

#ifdef __PRINT_ELEMENTS__
    Element *parent = m_parent;

    while (parent)
    {
      fprintf (stderr, "  ");
      parent = parent->getParent();
    }

    fprintf (stderr, "%s = %s\n", _name.c_str(), _value.c_str ());
#endif
  }

  void addChild(Element *child)
  {
    m_children.push_back(child);
  }

  static void printPrefix(int level)
  {
    for (int index = 0; index < level * 2; index ++)
      fprintf (stderr, " ");
  }

  void print(int level)
  {
    printPrefix(level);
    if (m_type == UXML_NT_ELEMENT)
    {
      fprintf (stderr, "<%s>\n", m_name.c_str ());

      for (map<string, string>::iterator iter = m_attributes.begin(); iter != m_attributes.end(); iter ++)
      {
        printPrefix(level);
        fprintf (stderr, "%s=%s\n", iter->first.c_str(), iter->second.c_str ());
      }
    }
    else
    {
      const char *strType = "";
      switch (m_type)
      {
        case UXML_NT_COMMENT: strType = "COMMENT"; break;
        case UXML_NT_PROLOG: strType = "PROLOG"; break;
        case UXML_NT_DOCTYPE: strType = "DOCTYPE"; break;
        case UXML_NT_CDATA: strType = "CDATA"; break;
        case UXML_NT_CONTENT: strType = "CONTENT"; break;
      }

      fprintf (stderr, "%s>>>%s<<<\n", strType, m_name.c_str ());
    }

    for (list<Element *>::iterator iter = m_children.begin(); iter != m_children.end(); iter ++)
    {
      (*iter)->print(level + 1);
    }
  }

  Element *getParent(void)
  {
    return m_parent;
  }

private:
  string m_name;
  Element *m_parent;
  map<string, string> m_attributes;
  list<Element *> m_children;
  int m_type;
};

int Element::m_numElements = 0;
int Element::m_numAttributes = 0;

void API_setAttribute(UXMLPARSER *parser, UXMLNODE node, const UXMLCHAR *_name, size_t cchName, const UXMLCHAR *_value, size_t cchValue)
{
  Element::m_numAttributes ++;
  string name(_name, cchName);
  string value(_value, cchValue);

  Element *elm = (Element *) node;
  elm->setAttribute(name, value);
}

void API_destroyNode(UXMLPARSER *parser, UXMLNODE node)
{
  Element *elm = (Element *) node;
  delete elm;
}

UXMLNODE API_createContentNode(UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *_content, size_t cchContent)
{
  Element::m_numElements ++;

  string content(_content, cchContent);

  Element *par = (Element *) parent;
  Element *elm = new Element(par, UXML_NT_CONTENT, content);
  
  if (par)
    par->addChild(elm);

  return (UXMLNODE) elm;
}

UXMLNODE API_createCommentNode(UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *_content, size_t cchContent)
{
  Element::m_numElements ++;

  string content(_content, cchContent);

  Element *par = (Element *) parent;
  Element *elm = new Element(par, UXML_NT_COMMENT, content);
  
  if (par)
    par->addChild(elm);

  return (UXMLNODE) elm;
}

UXMLNODE API_createCDATANode(UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *_content, size_t cchContent)
{
  Element::m_numElements ++;

  string content(_content, cchContent);

  Element *par = (Element *) parent;
  Element *elm = new Element(par, UXML_NT_CDATA, content);
  
  if (par)
    par->addChild(elm);

  return (UXMLNODE) elm;
}

UXMLNODE API_createDocTypeNode(UXMLPARSER *parser, UXMLNODE parent)
{
  Element::m_numElements ++;

  Element *par = (Element *) parent;
  Element *elm = new Element(par, UXML_NT_DOCTYPE, "");
  
  if (par)
    par->addChild(elm);

  return (UXMLNODE) elm;
}


UXMLNODE API_createElement(UXMLPARSER *parser, UXMLNODE parent, const UXMLCHAR *_name, size_t cchName)
{
  Element::m_numElements ++;

  string name(_name, cchName);

  Element *par = (Element *) parent;
  Element *elm = new Element(par, UXML_NT_ELEMENT, name);

  if (par)
    par->addChild(elm);

  return (UXMLNODE) elm;
}

void printString(FILE *file, const char *str, size_t len)
{
  while (len > 0)
  {
    fputc((*str++), file);
  }
}

void iterNode(UXMLState *state, UXMLNode *node)
{
  string name (node->name.ptr, node->name.len);

  if (!name.empty())
    fprintf (stderr, "%s\n", name.c_str());

  for (UXMLAttribute *attr = node->attributes; attr != NULL; attr = attr->next)
  {
    string attrName(attr->name.ptr, attr->name.len);
    string attrValue(attr->value.ptr, attr->value.len);

    fprintf (stderr, "%s = %s\n", attrName.c_str (), attrValue.c_str());
  }

  for (UXMLChildRef *ref = node->children; ref != NULL; ref = ref->next)
  {
    iterNode(state, ref->child);
  }
}
int main (int argc, char **argv)
{
  /*
  struct UXMLPARSER parser;
  parser.createElement = API_createElement;
  parser.createCDATANode = API_createCDATANode;
  parser.createContentNode = API_createContentNode;
  parser.createCommentNode = API_createCommentNode;
  parser.createDocTypeNode = API_createDocTypeNode;
  parser.setAttribute = API_setAttribute;
  parser.destroyNode = API_destroyNode;
  */

  UXMLCHAR *buffer = new UXMLCHAR[1024 * 1024 * 10];

  FILE *file = fopen (argv[1], "rb");
  size_t len = fread (buffer, 1, 1024 * 1024 * 10, file);
  
  int count = 0;
  Element::m_numElements = 0;
  Element::m_numAttributes = 0;
  DWORD tsStart = GetTickCount();
  UXMLState state;
  
  size_t cbHeap = 1024 * 1024;
  void *heap = (void *) malloc(cbHeap);

  buffer[len] = '\0';

  while (GetTickCount () - tsStart < 1000)
  {
    UXMLDocument *document = UXMLC_parse(&state, buffer, heap, cbHeap);

    //iterNode(&state, document->root);
    UXMLC_destroy(&state);
    count ++;
  }

  free (heap);

  fprintf (stderr, "%d\n", count);


  getchar();
  return 0;
}