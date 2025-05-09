/*
 *  hddm-py :    tool that reads in a HDDM document (Hall D Data Model)
 *        and writes a c++ class library that expresses the model as a
 *        python extension module. It does this by wrapping the classes
 *        of the c++ API as python classes, adding convenience methods
 *        to provide natural pythonic semantics for handling hddm files
 *        and objects.
 *
 *  author: richard.t.jones at uconn.edu
 *  version: june 24, 2016 - original release.
 *
 *  Version 1.1 - Richard Jones, February 10, 2021.
 *  - Modified to be able to accept a hddm file as a valid hddm template.
 *    This simplifies the documentation by eliminating the false distinction
 *    between a hddm template and the text header that appears at the top of
 *    every hddm file. It also gets rid of the unnecessary step of having
 *    to delete the binary data following the header in a hddm file before
 *    it can be used as a template.
 */

#include "VersionConfig.hpp"
#include "XString.hpp"
#include "XParsers.hpp"
#include <xercesc/util/XMLUri.hpp>

#include <particleType.h>
#include <errno.h>
#ifdef _WIN32
#include <unistd_win32.h>
#else
#include <unistd.h>
#endif

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

#define X(str) XString(str).unicode_str()
#define S(str) str.c_str()

using namespace xercesc;

XString classPrefix;

void usage()
{
   std::cerr
        << "\nUsage:\n"
        << "    hddm-py [-v | -o <filename>] {HDDM file}\n\n"
        << "Options:\n"
        <<  "    -v            validate only\n"
        <<  "    -o <filename>    write to <filename>.cpy"
        << "Version: " << HDDM_VERSION_MAJOR << "." << HDDM_VERSION_MINOR
        << std::endl;
}

std::string guessType(const std::string &literal);
Particle_t lookupParticle(const std::string &name);

class XtString : public XString
{
/* XString class with a few extra methods for creating type
 * strings that are useful in creating class names
 */
 public:
   XtString() {};
   XtString(const char* s): XString(s) {};
   XtString(const XMLCh* p): XString(p) {};
   XtString(const std::string& s): XString(s) {};
   XtString(const XString& x): XString(x) {};
   XtString(const XtString& t): XString((XString&)t) {};
   ~XtString() {};

   XtString plural();
   XtString simpleType();
   XtString listType();
   XtString linkType();
};

class CodeBuilder
{
/* The methods in this class are used to write the c++ code
 * that implements the hddm python extension library.
 */
 public:
   std::ofstream pyFile;

   CodeBuilder() {};
   ~CodeBuilder() {};

   void checkConsistency(DOMElement* el, DOMElement* elref);
   void writeClassdef(DOMElement* el);
   void writeClassimp(DOMElement* el);
   void constructDocument(DOMElement* el);
   void constructGroup(DOMElement* el);
   void constructIOstreams(DOMElement* el);
   void constructMethods(DOMElement* el);
   void constructStreamers(DOMElement* el);
   void writeStreamers(DOMElement* el);

   typedef struct {
      std::string name;
      std::string args;
      std::string docs;
   } method_descr;

   std::map<XtString,XtString> typesList;

 private:
   std::vector<DOMElement*> tagList;
   typedef std::vector<DOMNode*> parentList_t;
   typedef std::map<const XtString,parentList_t> parentTable_t;
   parentList_t parentList;
   parentTable_t parents;
   parentTable_t children;
   int element_in_list(XtString &name, parentList_t list);
};


int main(int argC, char* argV[])
{
   try
   {
      XMLPlatformUtils::Initialize();
   }
   catch (const XMLException* toCatch)
   {
      XtString msg(toCatch->getMessage());
      std::cerr
           << "hddm-py: Error during initialization! :\n"
           << msg << std::endl;
      return 1;
   }

   if (argC < 2)
   {
      usage();
      return 1;
   }
   else if ((argC == 2) && (strcmp(argV[1], "-?") == 0))
   {
      usage();
      return 2;
   }

   XtString xmlFile;
   XtString pyFilename;
   bool verifyOnly = false;
   int argInd;
   for (argInd = 1; argInd < argC; argInd++)
   {
      if (argV[argInd][0] != '-')
      {
         break;
      }
      if (strcmp(argV[argInd],"-v") == 0)
      {
         verifyOnly = true;
      }
      else if (strcmp(argV[argInd],"-o") == 0)
      {
         pyFilename = XtString(argV[++argInd]);
      }
      else
      {
         std::cerr
              << "Unknown option \'" << argV[argInd]
              << "\', ignoring it\n" << std::endl;
      }
   }

   if (argInd != argC - 1)
   {
      usage();
      return 1;
   }
   xmlFile = XtString(argV[argInd]);
   std::ifstream ifs(xmlFile.c_str());
   if (!ifs.good())
   {
      std::cerr
           << "hddm-py: Error opening hddm template " << xmlFile << std::endl;
      exit(1);
   }
   std::ostringstream tmpFileStr;
   
   tmpFileStr << "tmp" << getpid();
   std::ofstream ofs(tmpFileStr.str().c_str());
   if (! ofs.is_open())
   {
      std::cerr
           << "hddm-py: Error opening temp file " << tmpFileStr.str() << std::endl;
      exit(2);
   }

   XString xmlPreamble("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
   XString xmlHeader;
   XString line;
   while (getline(ifs,line))
   {
      if (line.find("<?xml") != line.npos)
      {
         xmlPreamble = line + "\n";
      }
      else if (line.find("<!DOCTYPE HDDM>") != line.npos)
      {
         xmlPreamble += line + "\n";
      }
      else if (line.size() == 0)
      {
         xmlPreamble += "\n";
      }
      else if (line.find("<HDDM ") != line.npos)
      {
         xmlHeader = line + "\n";
         ofs << xmlPreamble << line;
         break;
      }
      else
      {
         std::cerr
              << "hddm-py: Template does not contain valid hddm header"
              << std::endl;
         exit(1);
      }
   }
   if (xmlHeader.size() == 0)
   {
      std::cerr
           << "hddm-py: Error reading from hddm template " << xmlFile 
           << std::endl;
      exit(1);
   }
   while (getline(ifs,line))
   {
      ofs << line;
      if (line == "</HDDM>")
      {
         break;
      }
   }
   ofs.close();
   ifs.close();

#if defined OLD_STYLE_XERCES_PARSER
   xercesc::DOMDocument* document = parseInputDocument(tmpFileStr.str().c_str(),false);
#else
   xercesc::DOMDocument* document = buildDOMDocument(tmpFileStr.str().c_str(),false);
#endif
   if (document == 0)
   {
      std::cerr
           << "hddm-py : Error parsing HDDM document, "
           << "cannot continue" << std::endl;
      return 1;
   }
   unlink(tmpFileStr.str().c_str());

   DOMElement* rootEl = document->getDocumentElement();
   XtString rootS(rootEl->getTagName());
   if (rootS != "HDDM")
   {
      std::cerr
           << "hddm-py error: root element of input document is "
           << "\"" << rootS << "\", expected \"HDDM\""
           << std::endl;
      return 1;
   }

   XtString classS(rootEl->getAttribute(X("class")));
   classPrefix = classS;

   XtString pyname;
   if (verifyOnly)
   {
      pyname = "/dev/null";
   }
   else if (pyFilename.size())
   {
      pyname = pyFilename + ".cpy";
   }
   else
   {
      pyname = "pyhddm_" + classPrefix + ".cpy";
   }

   CodeBuilder builder;
   builder.pyFile.open(pyname.c_str());
   if (! builder.pyFile.is_open())
   {
      std::cerr
           << "hddm-py error: unable to open output file "
           << pyname << std::endl;
      return 1;
   }

   builder.pyFile <<
   "/*\n"
   " * pyhddm_" << classPrefix << ".cpy - DO NOT EDIT THIS FILE\n"
   " *\n"
   " * This file was generated automatically by hddm-py from the file\n"
   << " * " << xmlFile << std::endl <<
   "\n"
   " * This source file contains the Python/C++ API wrappers that\n"
   " * provide a python interface to the hddm classes and methods\n"
   " * generated by hddm-cpp from " << xmlFile << ".\n"
   " *\n"
   " * The hddm data model tool set was written by\n"
   " * Richard Jones, University of Connecticut.\n"
   " *\n"
   " * For more information see the documentation at\n"
   " * http://github.com/rjones30/HDDM\n"
   " *\n"
   " */\n"
   "\n"
   "#include <Python.h>\n"
   "#include <structmember.h>\n"
   "\n"
   "#include <hddm_" << classPrefix << ".hpp>\n"
   "#include <fstream>\n"
   "#include <iostream>\n"
   "#include <exception>\n"
   "#include <particleType.h>\n"
   ;

#ifdef ENABLE_ISTREAM_OVER_HTTP
   builder.pyFile << "#define ISTREAM_OVER_HTTP 1\n";
#endif
#ifdef ENABLE_ISTREAM_OVER_XROOTD
   builder.pyFile << "#define ISTREAM_OVER_XROOTD 1\n";
#endif

   builder.pyFile <<
   "#ifdef ISTREAM_OVER_HTTP\n"
   "#include <httpstream.hpp>\n"
   "#endif\n"
   "#ifdef ISTREAM_OVER_XROOTD\n"
   "#include <xrootdstream.hpp>\n"
   "#endif\n"
   "\n"
   "using namespace hddm_" << classPrefix << ";\n"
   "\n"
   "#if PY_MAJOR_VERSION >= 3\n"
   "   #define PyInt_FromLong PyLong_FromLong\n"
   "   #define PyInt_AsLong PyLong_AsLong\n"
   "#endif\n"
   "\n"
   "\n"
   "inline void LOG_NEW(PyTypeObject *t, PyTypeObject *subt=0, int own=0) {\n"
   "#if 0\n"
   "   if (subt == 0)\n"
   "      std::cout << \"creating a new element of \" << t->tp_name\n"
   "                << \" \" << ((own == 0)? \"(borrowed)\" : \"(owned)\")\n"
   "                << std::endl;\n"
   "   else\n"
   "      std::cout << \"creating a new list of \" << subt->tp_name\n"
   "                << \" \" << ((own == 0)? \"(borrowed)\" : \"(owned)\")\n"
   "                << std::endl;\n"
   "#endif\n"
   "}\n"
   "\n"
   "inline void LOG_DEALLOC(PyTypeObject *t, PyTypeObject *subt=0, int own=0) {\n"
   "#if 0\n"
   "   if (subt == 0)\n"
   "      std::cout << \"destroying an element of \" << t->tp_name\n"
   "                << \" \" << ((own == 0)? \"(borrowed)\" : \"(owned)\")\n"
   "                << std::endl;\n"
   "   else\n"
   "      std::cout << \"destroying a list of \" << subt->tp_name\n"
   "                << \" \" << ((own == 0)? \"(borrowed)\" : \"(owned)\")\n"
   "                << std::endl;\n"
   "#endif\n"
   "}\n"
   "\n"
   "inline void My_INCREF(PyObject *o) {\n"
   "   //std::cout << \"incrementing reference at \" << o << std::endl;\n"
   "   Py_INCREF(o);\n"
   "}\n"
   "\n"
   "inline void My_DECREF(PyObject *o) {\n"
   "   //std::cout << \"decrementing reference at \" << o << std::endl;\n"
   "   Py_DECREF(o);\n"
   "}\n"
   "\n"
   "// wrap base class hddm_" << classPrefix << "::HDDM_Element"
   " as hddm_" << classPrefix << ".HDDM_Element\n"
   "\n"
   "typedef struct {\n"
   "   PyObject_HEAD\n"
   "   HDDM_Element *elem;\n"
   "   PyObject *host;\n"
   "} _HDDM_Element;\n"
   "\n"
   "static void\n"
   "_HDDM_Element_dealloc(_HDDM_Element* self)\n"
   "{\n"
   "   if (self->elem != 0) {\n"
   "      LOG_DEALLOC(Py_TYPE(self), 0, self->host == (PyObject*)self);\n"
   "      if (self->host == (PyObject*)self)\n"
   "         delete self->elem;\n"
   "      else\n"
   "         My_DECREF(self->host);\n"
   "   }\n"
   "   Py_TYPE(self)->tp_free((PyObject*)self);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_HDDM_Element_new(PyTypeObject *type, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   _HDDM_Element *self;\n"
   "   self = (_HDDM_Element*)type->tp_alloc(type, 0);\n"
   "   if (self != NULL) {\n"
   "      self->elem = 0;\n"
   "      self->host = 0;\n"
   "   }\n"
   "   return (PyObject*)self;\n"
   "}\n"
   "\n"
   "static int\n"
   "_HDDM_Element_init(_HDDM_Element *self, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   PyErr_SetString(PyExc_RuntimeError, \"illegal constructor\");\n"
   "   return -1;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_HDDM_Element_getAttribute(PyObject *self, PyObject *args)\n"
   "{\n"
   "   char *attr;\n"
   "   if (! PyArg_ParseTuple(args, \"s\", &attr)) {\n"
   "      return NULL;\n"
   "   }\n"
   "   _HDDM_Element *me = (_HDDM_Element*)self;\n"
   "   if (me->elem == 0) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, \"lookup attempted on invalid"
   " element\");\n"
   "      return NULL;\n"
   "   }\n"
   "   hddm_type atype;\n"
   "   void *val((int*)me->elem->getAttribute(std::string(attr),&atype));\n"
   "   if (val == 0) {\n"
   "      Py_INCREF(Py_None);\n"
   "      return Py_None;\n"
   "   }\n"
   "   else if (atype == k_hddm_int) {\n"
   "      return PyLong_FromLong(*(int*)val);\n"
   "   }\n"
   "   else if (atype == k_hddm_long) {\n"
   "      return PyLong_FromLongLong(*(long long*)val);\n"
   "   }\n"
   "   else if (atype == k_hddm_float) {\n"
   "      return PyFloat_FromDouble(double(*(float*)val));\n"
   "   }\n"
   "   else if (atype == k_hddm_double) {\n"
   "      return PyFloat_FromDouble(*(double*)val);\n"
   "   }\n"
   "   else if (atype == k_hddm_boolean) {\n"
   "      if (*(bool*)val == 0) {\n"
   "         Py_INCREF(Py_False);\n"
   "         return Py_False;\n"
   "      }\n"
   "      else {\n"
   "         Py_INCREF(Py_True);\n"
   "         return Py_True;\n"
   "      }\n"
   "   }\n"
   "   else if (atype == k_hddm_string) {\n"
   "      return PyUnicode_FromString(((std::string*)val)->c_str());\n"
   "   }\n"
   "   else if (atype == k_hddm_anyURI) {\n"
   "      return PyUnicode_FromString(((std::string*)val)->c_str());\n"
   "   }\n"
   "   else if (atype == k_hddm_Particle_t) {\n"
   "      return PyUnicode_FromString(ParticleType(*(Particle_t*)val));\n"
   "   }\n"
   "   return PyUnicode_FromString(((std::string*)val)->c_str());\n"
   "}\n\n"
   "static PyMemberDef _HDDM_Element_members[] = {\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyMethodDef _HDDM_Element_methods[] = {\n"
   "   {\"getAttribute\", _HDDM_Element_getAttribute, METH_VARARGS,\n"
   "    \"look up named attribute in this element\"},\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyTypeObject _HDDM_Element_type = {\n"
   "    PyVarObject_HEAD_INIT(NULL,0)\n"
   "    \"hddm_" << classPrefix << ".HDDM_Element\",     /*tp_name*/\n"
   "    sizeof(_HDDM_Element),     /*tp_basicsize*/\n"
   "    0,                         /*tp_itemsize*/\n"
   "    (destructor)_HDDM_Element_dealloc, /*tp_dealloc*/\n"
   "    0,                         /*tp_print*/\n"
   "    0,                         /*tp_getattr*/\n"
   "    0,                         /*tp_setattr*/\n"
   "    0,                         /*tp_compare*/\n"
   "    0,                         /*tp_repr*/\n"
   "    0,                         /*tp_as_number*/\n"
   "    0,                         /*tp_as_sequence*/\n"
   "    0,                         /*tp_as_mapping*/\n"
   "    0,                         /*tp_hash */\n"
   "    0,                         /*tp_call*/\n"
   "    0,                         /*tp_str*/\n"
   "    0,                         /*tp_getattro*/\n"
   "    0,                         /*tp_setattro*/\n"
   "    0,                         /*tp_as_buffer*/\n"
   "    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/\n"
   "    \"hddm_" << classPrefix << " basic element\",    /* tp_doc */\n"
   "    0,                         /* tp_traverse */\n"
   "    0,                         /* tp_clear */\n"
   "    0,                         /* tp_richcompare */\n"
   "    0,                         /* tp_weaklistoffset */\n"
   "    0,                         /* tp_iter */\n"
   "    0,                         /* tp_iternext */\n"
   "    _HDDM_Element_methods,     /* tp_methods */\n"
   "    _HDDM_Element_members,     /* tp_members */\n"
   "    0,                         /* tp_getset */\n"
   "    0,                         /* tp_base */\n"
   "    0,                         /* tp_dict */\n"
   "    0,                         /* tp_descr_get */\n"
   "    0,                         /* tp_descr_set */\n"
   "    0,                         /* tp_dictoffset */\n"
   "    (initproc)_HDDM_Element_init, /* tp_init */\n"
   "    0,                         /* tp_alloc */\n"
   "    _HDDM_Element_new,         /* tp_new */\n"
   "};\n"
   "\n"
   "\n"
   "// wrap base class hddm_" << classPrefix << "::HDDM_ElementList"
   " as hddm_" << classPrefix << ".HDDM_ElementList\n"
   "\n"
   "typedef struct {\n"
   "   PyObject_HEAD\n"
   "   PyTypeObject *subtype; // type of wrapper derived from _HDDM_Element\n"
   "   HDDM_ElementList<HDDM_Element> *list;\n"
   "   PyObject *host;\n"
   "   int borrowed;\n"
   "} _HDDM_ElementList;\n"
   "\n"
   "static void\n"
   "_HDDM_ElementList_dealloc(_HDDM_ElementList* self)\n"
   "{\n"
   "   if (self->list != 0) {\n"
   "      LOG_DEALLOC(Py_TYPE(self), self->subtype, self->borrowed == 0);\n"
   "      if (self->borrowed == 0)\n"
   "         delete self->list;\n"
   "      My_DECREF(self->host);\n"
   "   }\n"
   "   Py_TYPE(self)->tp_free((PyObject*)self);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_HDDM_ElementList_new(PyTypeObject *type, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   _HDDM_ElementList *self;\n"
   "   self = (_HDDM_ElementList*)type->tp_alloc(type, 0);\n"
   "   if (self != NULL) {\n"
   "      self->subtype = 0;\n"
   "      self->borrowed = 0;\n"
   "      self->host = 0;\n"
   "   }\n"
   "   return (PyObject*)self;\n"
   "}\n"
   "\n"
   "static int\n"
   "_HDDM_ElementList_init(_HDDM_ElementList *self, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   PyErr_SetString(PyExc_RuntimeError, \"illegal constructor\");\n"
   "   return -1;\n"
   "}\n"
   "\n"
   "static Py_ssize_t\n"
   "_HDDM_ElementList_size(_HDDM_ElementList *self, void *closure)\n"
   "{\n"
   "   if (self->list == 0) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, \"size attempted on invalid list\");\n"
   "      return -1;\n"
   "   }\n"
   "   return self->list->size();\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_HDDM_ElementList_item(_HDDM_ElementList *self, Py_ssize_t i)\n"
   "{\n"
   "   if (self->list == 0)\n"
   "      return NULL;\n"
   "   int len = self->list->size();\n"
   "   if (i < 0 || i >= len) {\n"
   "      PyErr_Format(PyExc_IndexError, \"index %ld out of bounds.\", i);\n"
   "      return NULL;\n"
   "   }\n"
   "   PyObject *elem_obj = _HDDM_Element_new(self->subtype, 0, 0);\n"
   "   ((_HDDM_Element*)elem_obj)->elem = &(HDDM_Element&)(*self->list)((int)i);\n"
   "   ((_HDDM_Element*)elem_obj)->host = self->host;\n"
   "   My_INCREF(self->host);\n"
   "   LOG_NEW(self->subtype);\n"
   "   return elem_obj;\n"
   "}\n"
   "\n"
   "extern PyTypeObject _HDDM_ElementList_type;\n"
   "\n"
   "static PyObject *\n"
   "_HDDM_ElementList_add(PyObject *self, PyObject *args)\n"
   "{\n"
   "   int count=0;\n"
   "   int start=-1;\n"
   "   if (! PyArg_ParseTuple(args, \"i|i\", &count, &start)) {\n"
   "      return NULL;\n"
   "   }\n"
   "   _HDDM_ElementList *me = (_HDDM_ElementList*)self;\n"
   "   if (me->list == 0) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, \"add attempted on invalid list\");\n"
   "      return NULL;\n"
   "   }\n"
   "   PyObject *list = _HDDM_ElementList_new(&_HDDM_ElementList_type, 0, 0);\n"
   "   ((_HDDM_ElementList*)list)->subtype = me->subtype;\n"
   "   ((_HDDM_ElementList*)list)->list = (HDDM_ElementList<HDDM_Element>*)\n"
   "    new HDDM_ElementList<HDDM_Element>(me->list->add(count, start));\n"
   "   ((_HDDM_ElementList*)list)->borrowed = 0;\n"
   "   ((_HDDM_ElementList*)list)->host = me->host;\n"
   "   My_INCREF(me->host);\n"
   "   LOG_NEW(Py_TYPE(self), me->subtype, 1);\n"
   "   return list;\n"
   "}\n"
   "\n"
   "static PyObject *\n"
   "_HDDM_ElementList_del(PyObject *self, PyObject *args)\n"
   "{\n"
   "   int start=0;\n"
   "   int count=-1;\n"
   "   if (! PyArg_ParseTuple(args, \"|ii\", &count, &start)) {\n"
   "      return NULL;\n"
   "   }\n"
   "   _HDDM_ElementList *list_obj;\n"
   "   list_obj = (_HDDM_ElementList*)self;\n"
   "   if (list_obj->list == 0) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, \"del attempted on invalid list\");\n"
   "      return NULL;\n"
   "   }\n"
   "   list_obj->list->del(count, start);\n"
   "   Py_INCREF(self);\n"
   "   return self;\n"
   "}\n"
   "\n"
   "static PyObject *\n"
   "_HDDM_ElementList_clear(PyObject *self, PyObject *args)\n"
   "{\n"
   "   _HDDM_ElementList *list_obj;\n"
   "   list_obj = (_HDDM_ElementList*)self;\n"
   "   if (list_obj->list == 0) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, \"clear attempted on invalid list\");\n"
   "      return NULL;\n"
   "   }\n"
   "   list_obj->list->clear();\n"
   "   Py_INCREF(self);\n"
   "   return self;\n"
   "}\n"
   "\n"
   "static PyMemberDef _HDDM_ElementList_members[] = {\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyMethodDef _HDDM_ElementList_methods[] = {\n"
   "   {\"add\",  _HDDM_ElementList_add, METH_VARARGS,\n"
   "    \"add (or insert) a new element to the list.\"},\n"
   "   {\"del\",  _HDDM_ElementList_del, METH_VARARGS,\n"
   "    \"delete an existing element from the list.\"},\n"
   "   {\"clear\",  _HDDM_ElementList_clear, METH_NOARGS,\n"
   "    \"reset the list to zero elements.\"},\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PySequenceMethods _HDDM_ElementList_as_sequence = {\n"
   "    (lenfunc)_HDDM_ElementList_size,            /* sq_length */\n"
   "    0,                                          /* sq_concat */\n"
   "    0,                                          /* sq_repeat */\n"
   "    (ssizeargfunc)_HDDM_ElementList_item,       /* sq_item */\n"
   "    0,                                          /* sq_slice */\n"
   "    0,                                          /* sq_ass_item */\n"
   "    0,                                          /* sq_ass_slice */\n"
   "    0,                                          /* sq_contains */\n"
   "    0,                                          /* sq_inplace_concat */\n"
   "    0,                                          /* sq_inplace_repeat */\n"
   "};\n"
   "\n"
   "PyTypeObject _HDDM_ElementList_type = {\n"
   "    PyVarObject_HEAD_INIT(NULL,0)\n"
   "    \"hddm_" << classPrefix << ".HDDM_ElementList\", /*tp_name*/\n"
   "    sizeof(_HDDM_ElementList), /*tp_basicsize*/\n"
   "    0,                         /*tp_itemsize*/\n"
   "    (destructor)_HDDM_ElementList_dealloc, /*tp_dealloc*/\n"
   "    0,                         /*tp_print*/\n"
   "    0,                         /*tp_getattr*/\n"
   "    0,                         /*tp_setattr*/\n"
   "    0,                         /*tp_compare*/\n"
   "    0,                         /*tp_repr*/\n"
   "    0,                         /*tp_as_number*/\n"
   "    &_HDDM_ElementList_as_sequence, /*tp_as_sequence*/\n"
   "    0,                         /*tp_as_mapping*/\n"
   "    0,                         /*tp_hash */\n"
   "    0,                         /*tp_call*/\n"
   "    0,                         /*tp_str*/\n"
   "    0,                         /*tp_getattro*/\n"
   "    0,                         /*tp_setattro*/\n"
   "    0,                         /*tp_as_buffer*/\n"
   "    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/\n"
   "    \"hddm_" << classPrefix << " element list\",    /* tp_doc */\n"
   "    0,                         /* tp_traverse */\n"
   "    0,                         /* tp_clear */\n"
   "    0,                         /* tp_richcompare */\n"
   "    0,                         /* tp_weaklistoffset */\n"
   "    0,                         /* tp_iter */\n"
   "    0,                         /* tp_iternext */\n"
   "    _HDDM_ElementList_methods, /* tp_methods */\n"
   "    _HDDM_ElementList_members, /* tp_members */\n"
   "    0,                         /* tp_getset */\n"
   "    0,                         /* tp_base */\n"
   "    0,                         /* tp_dict */\n"
   "    0,                         /* tp_descr_get */\n"
   "    0,                         /* tp_descr_set */\n"
   "    0,                         /* tp_dictoffset */\n"
   "    (initproc)_HDDM_ElementList_init,   /* tp_init */\n"
   "    0,                         /* tp_alloc */\n"
   "    _HDDM_ElementList_new,     /* tp_new */\n"
   "};\n"
   ;

   builder.constructGroup(rootEl);
   builder.constructIOstreams(rootEl);
   builder.constructMethods(rootEl);
   builder.constructStreamers(rootEl);

   builder.typesList["HDDM_Element"] = "_HDDM_Element_type";
   builder.typesList["HDDM_ElementList"] = "_HDDM_ElementList_type";
   builder.typesList["streamposition"] = "_streamposition_type";
   builder.typesList["ostream"] = "_ostream_type";
   builder.typesList["istream"] = "_istream_type";

   builder.pyFile <<
   "\n"
   "\n"
   "// wrap class hddm_" << classPrefix << "::streamposition"
   " as hddm_" << classPrefix << ".streamposition\n"
   "\n"
   "typedef struct {\n"
   "   PyObject_HEAD\n"
   "   streamposition *streampos;\n"
   "} _streamposition;\n"
   "\n"
   "static void\n"
   "_streamposition_dealloc(_streamposition* self)\n"
   "{\n"
   "   if (self->streampos != 0)\n"
   "      delete self->streampos;\n"
   "   Py_TYPE(self)->tp_free((PyObject*)self);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_streamposition_new(PyTypeObject *type, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   _streamposition *self;\n"
   "   self = (_streamposition*)type->tp_alloc(type, 0);\n"
   "   if (self != NULL)\n"
   "      self->streampos = 0;\n"
   "   return (PyObject*)self;\n"
   "}\n"
   "\n"
   "static int\n"
   "_streamposition_init(_streamposition *self, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   const char *kwlist[] = {\"start\", \"offset\", \"status\", NULL};\n"
   "   uint64_t start = 0;\n"
   "   uint32_t offset = 0;\n"
   "   uint32_t status = 0;\n"
   "   if (PyArg_ParseTuple(args, \"\") ||\n"
   "       PyArg_ParseTupleAndKeywords(args, kwds, \"kII\", (char**)kwlist, \n"
   "                                   &start, &offset, &status))\n"
   "   {\n"
   "      PyErr_Clear();\n"
   "      if (self->streampos != 0)\n"
   "         delete self->streampos;\n"
   "      self->streampos = new streamposition(start, offset, status);\n"
   "      return 0;\n"
   "   }\n"
   "   return -1; \n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_streamposition_richcompare(PyObject *a, PyObject *b, int op)\n"
   "{\n"
   "   int res = 0;\n"
   "   streamposition *apos = ((_streamposition*)a)->streampos;\n"
   "   streamposition *bpos = ((_streamposition*)b)->streampos;\n"
   "   if (op == Py_LT)\n"
   "      res = (*apos < *bpos);\n"
   "   else if (op == Py_LE)\n"
   "      res = (*apos <= *bpos);\n"
   "   else if (op == Py_EQ)\n"
   "      res = (*apos == *bpos);\n"
   "   else if (op == Py_NE)\n"
   "      res = (*apos != *bpos);\n"
   "   else if (op == Py_GT)\n"
   "      res = (*apos > *bpos);\n"
   "   else if (op == Py_GE)\n"
   "      res = (*apos >= *bpos);\n"
   "   if (res) {\n"
   "      Py_INCREF(Py_True);\n"
   "      return Py_True;\n"
   "   }\n"
   "   else {\n"
   "      Py_INCREF(Py_False);\n"
   "      return Py_False;\n"
   "   }\n"
   "}\n"
   "static PyObject*\n"
   "_streamposition_toString(PyObject *self, PyObject *args=0)\n"
   "{\n"
   "   std::stringstream ostr;\n"
   "   ostr << \"hddm_" << classPrefix << ".streamposition(\"\n"
   "        << ((_streamposition*)self)->streampos->block_start << \",\"\n"
   "        << ((_streamposition*)self)->streampos->block_offset << \",\"\n"
   "        << ((_streamposition*)self)->streampos->block_status\n"
   "        << \")\";\n"
   "   return PyUnicode_FromString(ostr.str().c_str());\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_streamposition_toRepr(PyObject *self, PyObject *args=0)\n"
   "{\n"
   "   std::stringstream ostr;\n"
   "   ostr << \"\\\'\";\n"
   "   ostr << \"hddm_" << classPrefix << ".streamposition(\"\n"
   "        << ((_streamposition*)self)->streampos->block_start << \",\"\n"
   "        << ((_streamposition*)self)->streampos->block_offset << \",\"\n"
   "        << ((_streamposition*)self)->streampos->block_status\n"
   "        << \")\";\n"
   "   ostr << \"\\\'\";\n"
   "   return PyUnicode_FromString(ostr.str().c_str());\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_streamposition_getstart(_streamposition *self, void *closure)\n"
   "{\n"
   "   return Py_BuildValue(\"k\", self->streampos->block_start);\n"
   "}\n"
   "\n"
   "static int\n"
   "_streamposition_setstart(_streamposition *self, PyObject *value, void *closure)\n"
   "{\n"
   "   if (value == NULL) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected null argument\");\n"
   "      return -1;\n"
   "   }\n"
   "   long start = PyInt_AsLong(value);\n"
   "   if (start < 0 && PyErr_Occurred()) {\n"
   "      return -1;\n"
   "   }\n"
   "   self->streampos->block_start = start;\n"
   "   return 0;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_streamposition_getoffset(_streamposition *self, void *closure)\n"
   "{\n"
   "   return Py_BuildValue(\"I\", self->streampos->block_offset);\n"
   "}\n"
   "\n"
   "static int\n"
   "_streamposition_setoffset(_streamposition *self, PyObject *value, void *closure)\n"
   "{\n"
   "   if (value == NULL) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected null argument\");\n"
   "      return -1;\n"
   "   }\n"
   "   long offset = PyInt_AsLong(value);\n"
   "   if (offset < 0 && PyErr_Occurred()) {\n"
   "      return -1;\n"
   "   }\n"
   "   self->streampos->block_offset = offset;\n"
   "   return 0;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_streamposition_getstatus(_streamposition *self, void *closure)\n"
   "{\n"
   "   return Py_BuildValue(\"I\", self->streampos->block_status);\n"
   "}\n"
   "\n"
   "static int\n"
   "_streamposition_setstatus(_streamposition *self, PyObject *value, void *closure)\n"
   "{\n"
   "   if (value == NULL) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected null argument\");\n"
   "      return -1;\n"
   "   }\n"
   "   long status = PyInt_AsLong(value);\n"
   "   if (status == -1 && PyErr_Occurred()) {\n"
   "      return -1;\n"
   "   }\n"
   "   self->streampos->block_status = status;\n"
   "   return 0;\n"
   "}\n"
   "\n"
   "static PyGetSetDef _streamposition_getsetters[] = {\n"
   "   {(char*)\"start\", \n"
   "    (getter)_streamposition_getstart, (setter)_streamposition_setstart,\n"
   "    (char*)\"block start position\",\n"
   "    NULL},\n"
   "   {(char*)\"offset\", \n"
   "    (getter)_streamposition_getoffset, (setter)_streamposition_setoffset,\n"
   "    (char*)\"block offset position\",\n"
   "    NULL},\n"
   "   {(char*)\"status\", \n"
   "    (getter)_streamposition_getstatus, (setter)_streamposition_setstatus,\n"
   "    (char*)\"block status flags\",\n"
   "    NULL},\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyMemberDef _streamposition_members[] = {\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyMethodDef _streamposition_methods[] = {\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyTypeObject _streamposition_type = {\n"
   "   PyVarObject_HEAD_INIT(NULL,0)\n"
   "   \"hddm_" << classPrefix << ".streamposition\",   /*tp_name*/\n"
   "   sizeof(_streamposition),   /*tp_basicsize*/\n"
   "   0,                         /*tp_itemsize*/\n"
   "   (destructor)_streamposition_dealloc, /*tp_dealloc*/\n"
   "   0,                         /*tp_print*/\n"
   "   0,                         /*tp_getattr*/\n"
   "   0,                         /*tp_setattr*/\n"
   "   0,                         /*tp_compare*/\n"
   "   (reprfunc)_streamposition_toRepr, /*tp_repr*/\n"
   "   0,                         /*tp_as_number*/\n"
   "   0,                         /*tp_as_sequence*/\n"
   "   0,                         /*tp_as_mapping*/\n"
   "   0,                         /*tp_hash */\n"
   "   0,                         /*tp_call*/\n"
   "   (reprfunc)_streamposition_toString, /*tp_str*/\n"
   "   0,                         /*tp_getattro*/\n"
   "   0,                         /*tp_setattro*/\n"
   "   0,                         /*tp_as_buffer*/\n"
   "   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/\n"
   "   \"hddm_" << classPrefix << " streamposition objects\", /* tp_doc */\n"
   "   0,                         /* tp_traverse */\n"
   "   0,                         /* tp_clear */\n"
   "   _streamposition_richcompare, /* tp_richcompare */\n"
   "   0,                         /* tp_weaklistoffset */\n"
   "   0,                         /* tp_iter */\n"
   "   0,                         /* tp_iternext */\n"
   "   _streamposition_methods,   /* tp_methods */\n"
   "   _streamposition_members,   /* tp_members */\n"
   "   _streamposition_getsetters, /* tp_getset */\n"
   "   0,                         /* tp_base */\n"
   "   0,                         /* tp_dict */\n"
   "   0,                         /* tp_descr_get */\n"
   "   0,                         /* tp_descr_set */\n"
   "   0,                         /* tp_dictoffset */\n"
   "   (initproc)_streamposition_init, /* tp_init */\n"
   "   0,                         /* tp_alloc */\n"
   "   _streamposition_new,       /* tp_new */\n"
   "};\n"
   "\n"
   "\n"
   "// wrap class hddm_" << classPrefix << "::ostream"
   " as hddm_" << classPrefix << ".ostream\n"
   "\n"
   "typedef struct {\n"
   "   PyObject_HEAD\n"
   "   std::string *fname;\n"
   "   std::ofstream *fstr;\n"
   "   ostream *ostr;\n"
   "} _ostream;\n"
   "\n"
   "static void\n"
   "_ostream_dealloc(_ostream* self)\n"
   "{\n"
   "   if (self->fname != 0)\n"
   "      delete self->fname;\n"
   "   if (self->ostr != 0)\n"
   "      delete self->ostr;\n"
   "   if (self->fstr != 0)\n"
   "      delete self->fstr;\n"
   "   Py_TYPE(self)->tp_free((PyObject*)self);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   _ostream *self;\n"
   "   self = (_ostream*)type->tp_alloc(type, 0);\n"
   "   if (self != NULL) {\n"
   "      self->fname = 0;\n"
   "      self->fstr = 0;\n"
   "      self->ostr = 0;\n"
   "   }\n"
   "   return (PyObject*)self;\n"
   "}\n"
   "\n"
   "static int\n"
   "_ostream_init(_ostream *self, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   const char *kwlist[] = {\"file\", NULL};\n"
   "   char *filename;\n"
   "   if (PyArg_ParseTupleAndKeywords(args, kwds, \"s\", (char**)kwlist, &filename))\n"
   "   {\n"
   "      if (self->fname != 0)\n"
   "         delete self->fname;\n"
   "      if (self->ostr != 0)\n"
   "         delete self->ostr;\n"
   "      if (self->fstr != 0)\n"
   "         delete self->fstr;\n"
   "      self->fname = new std::string(filename);\n"
   "      self->fstr = new std::ofstream(filename);\n"
   "      if (! self->fstr->good()) {\n"
   "         PyErr_Format(PyExc_IOError, \"Cannot open output file %s\", filename);\n"
   "         return -1;\n"
   "      }\n"
   "      try {\n"
   "         self->ostr = new ostream(*self->fstr);\n"
   "      }\n"
   "      catch (std::exception& e) {\n"
   "         PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "         return -1;\n"
   "      }\n"
   "      return 0;\n"
   "   }\n"
   "   return -1; \n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_getCompression(_ostream *self, void *closure)\n"
   "{\n"
   "   return Py_BuildValue(\"i\", self->ostr->getCompression());\n"
   "}\n"
   "\n"
   "static int\n"
   "_ostream_setCompression(_ostream *self, PyObject *value, void *closure)\n"
   "{\n"
   "   if (value == NULL) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected null argument\");\n"
   "      return -1;\n"
   "   }\n"
   "   long flags = PyInt_AsLong(value);\n"
   "   if (flags == -1 && PyErr_Occurred()) {\n"
   "      return -1;\n"
   "   }\n"
   "   try {\n"
   "      self->ostr->setCompression(flags);\n"
   "   }\n"
   "   catch (std::exception& e) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "      return -1;\n"
   "   }\n"
   "   return 0;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_getIntegrityChecks(_ostream *self, void *closure)\n"
   "{\n"
   "   PyObject *flags = Py_BuildValue(\"i\", self->ostr->getIntegrityChecks());\n"
   "   return flags;\n"
   "}\n"
   "\n"
   "static int\n"
   "_ostream_setIntegrityChecks(_ostream *self, PyObject *value, void *closure)\n"
   "{\n"
   "   if (value == NULL) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected null argument\");\n"
   "      return -1;\n"
   "   }\n"
   "   long flags = PyInt_AsLong(value);\n"
   "   if (flags == -1 && PyErr_Occurred()) {\n"
   "      return -1;\n"
   "   }\n"
   "   try {\n"
   "      self->ostr->setIntegrityChecks(flags);\n"
   "   }\n"
   "   catch (std::exception& e) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "      return -1;\n"
   "   }\n"
   "   return 0;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_getPosition(_ostream *self, void *closure)\n"
   "{\n"
   "   streamposition *pos = new streamposition();\n"
   "   if (self->ostr != 0)\n"
   "      *pos = self->ostr->getPosition();\n"
   "   PyObject *pos_obj = _streamposition_new(&_streamposition_type, 0, 0);\n"
   "   ((_streamposition*)pos_obj)->streampos = pos;\n"
   "   return pos_obj;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_getRecordsWritten(_ostream *self, void *closure)\n"
   "{\n"
   "   size_t records = 0;\n"
   "   if (self->ostr != 0)\n"
   "      try {\n"
   "         records = self->ostr->getRecordsWritten();\n"
   "      }\n"
   "      catch (std::exception& e) {\n"
   "         PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "         return NULL;\n"
   "      }\n"
   "   return PyLong_FromLongLong(records);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_getBytesWritten(_ostream *self, void *closure)\n"
   "{\n"
   "   size_t bytes = 0;\n"
   "   if (self->ostr != 0)\n"
   "      try {\n"
   "         bytes = self->ostr->getBytesWritten();\n"
   "      }\n"
   "      catch (std::exception& e) {\n"
   "         PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "         return NULL;\n"
   "      }\n"
   "   return PyLong_FromLongLong(bytes);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_write(PyObject *self, PyObject *args)\n"
   "{\n"
   "   _HDDM *record_obj;\n"
   "   if (! PyArg_ParseTuple(args, \"O!\", &_HDDM_type, (PyObject*)&record_obj))\n"
   "       return NULL;\n"
   "   ostream *ostr = ((_ostream*)self)->ostr;\n"
   "   try {\n"
   "      Py_BEGIN_ALLOW_THREADS\n"
   "      *ostr << *record_obj->elem;\n"
   "      Py_END_ALLOW_THREADS\n"
   "   }\n"
   "   catch (std::exception& e) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "      return NULL;\n"
   "   }\n"
   "   Py_INCREF(Py_None);\n"
   "   return Py_None;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_toString(PyObject *self, PyObject *args=0)\n"
   "{\n"
   "   std::stringstream ostr;\n"
   "   if (((_ostream*)self)->fname != 0)\n"
   "      ostr << \"hddm_" << classPrefix << ".ostream(\\\"\"\n"
   "           << *((_ostream*)self)->fname << \"\\\")\";\n"
   "   else\n"
   "      ostr << \"hddm_" << classPrefix << ".ostream(NULL)\";\n"
   "   return PyUnicode_FromString(ostr.str().c_str());\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_ostream_toRepr(PyObject *self, PyObject *args=0)\n"
   "{\n"
   "   std::stringstream ostr;\n"
   "   ostr << \"\\\'\";\n"
   "   if (((_ostream*)self)->fname != 0)\n"
   "      ostr << \"hddm_" << classPrefix << ".ostream(\\\"\"\n"
   "           << *((_ostream*)self)->fname << \"\\\")\";\n"
   "   else\n"
   "      ostr << \"hddm_" << classPrefix << ".ostream()\";\n"
   "   ostr << \"\\\'\";\n"
   "   return PyUnicode_FromString(ostr.str().c_str());\n"
   "}\n"
   "\n"
   "static PyGetSetDef _ostream_getsetters[] = {\n"
   "   {(char*)\"compression\", \n"
   "    (getter)_ostream_getCompression, (setter)_ostream_setCompression,\n"
   "    (char*)\"ostream compression mode (k_no_compression, k_z_compression, ...)\",\n"
   "    NULL},\n"
   "   {(char*)\"integrityChecks\", \n"
   "    (getter)_ostream_getIntegrityChecks, (setter)_ostream_setIntegrityChecks,\n"
   "    (char*)\"ostream data integrity checking mode (k_no_integrity, ...)\",\n"
   "    NULL},\n"
   "   {(char*)\"position\", \n"
   "    (getter)_ostream_getPosition, 0,\n"
   "    (char*)\"output stream position\",\n"
   "    NULL},\n"
   "   {(char*)\"recordsWritten\", \n"
   "    (getter)_ostream_getRecordsWritten, 0,\n"
   "    (char*)\"total records written to ostream\",\n"
   "    NULL},\n"
   "   {(char*)\"bytesWritten\", \n"
   "    (getter)_ostream_getBytesWritten, 0,\n"
   "    (char*)\"total bytes written to ostream\",\n"
   "    NULL},\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyMemberDef _ostream_members[] = {\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyMethodDef _ostream_methods[] = {\n"
   "   {\"write\",  _ostream_write, METH_VARARGS,\n"
   "    \"write a HDDM record to the output stream.\"},\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyTypeObject _ostream_type = {\n"
   "    PyVarObject_HEAD_INIT(NULL,0)\n"
   "    \"hddm_" << classPrefix << ".ostream\",          /*tp_name*/\n"
   "    sizeof(_ostream),          /*tp_basicsize*/\n"
   "    0,                         /*tp_itemsize*/\n"
   "    (destructor)_ostream_dealloc, /*tp_dealloc*/\n"
   "    0,                         /*tp_print*/\n"
   "    0,                         /*tp_getattr*/\n"
   "    0,                         /*tp_setattr*/\n"
   "    0,                         /*tp_compare*/\n"
   "    (reprfunc)_ostream_toRepr, /*tp_repr*/\n"
   "    0,                         /*tp_as_number*/\n"
   "    0,                         /*tp_as_sequence*/\n"
   "    0,                         /*tp_as_mapping*/\n"
   "    0,                         /*tp_hash */\n"
   "    0,                         /*tp_call*/\n"
   "    (reprfunc)_ostream_toString, /*tp_str*/\n"
   "    0,                         /*tp_getattro*/\n"
   "    0,                         /*tp_setattro*/\n"
   "    0,                         /*tp_as_buffer*/\n"
   "    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/\n"
   "    \"hddm_" << classPrefix << " output stream\",    /* tp_doc */\n"
   "    0,                         /* tp_traverse */\n"
   "    0,                         /* tp_clear */\n"
   "    0,                         /* tp_richcompare */\n"
   "    0,                         /* tp_weaklistoffset */\n"
   "    0,                         /* tp_iter */\n"
   "    0,                         /* tp_iternext */\n"
   "    _ostream_methods,          /* tp_methods */\n"
   "    _ostream_members,          /* tp_members */\n"
   "    _ostream_getsetters,       /* tp_getset */\n"
   "    0,                         /* tp_base */\n"
   "    0,                         /* tp_dict */\n"
   "    0,                         /* tp_descr_get */\n"
   "    0,                         /* tp_descr_set */\n"
   "    0,                         /* tp_dictoffset */\n"
   "    (initproc)_ostream_init,   /* tp_init */\n"
   "    0,                         /* tp_alloc */\n"
   "    _ostream_new,              /* tp_new */\n"
   "};\n"
   "\n"
   "\n"
   "// wrap class hddm_" << classPrefix << "::istream"
   " as hddm_" << classPrefix << ".istream\n"
   "\n"
   "typedef struct {\n"
   "   PyObject_HEAD\n"
   "   std::string *fname;\n"
   "   std::ifstream *fstr;\n"
   "#ifdef ISTREAM_OVER_HTTP\n"
   "   httpIstream *tstr;\n"
   "#endif\n"
   "#ifdef ISTREAM_OVER_XROOTD\n"
   "   xrootdIstream *rstr;\n"
   "#endif\n"
   "   istream *istr;\n"
   "} _istream;\n"
   "\n"
   "static void\n"
   "_istream_dealloc(_istream* self)\n"
   "{\n"
   "   if (self->fname != 0)\n"
   "      delete self->fname;\n"
   "   if (self->istr != 0)\n"
   "      delete self->istr;\n"
   "   if (self->fstr != 0)\n"
   "      delete self->fstr;\n"
   "#ifdef ISTREAM_OVER_HTTP\n"
   "   if (self->tstr != 0)\n"
   "      delete self->tstr;\n"
   "#endif\n"
   "#ifdef ISTREAM_OVER_XROOTD\n"
   "   if (self->rstr != 0)\n"
   "      delete self->rstr;\n"
   "#endif\n"
   "   Py_TYPE(self)->tp_free((PyObject*)self);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   _istream *self;\n"
   "   self = (_istream*)type->tp_alloc(type, 0);\n"
   "   if (self != NULL) {\n"
   "      self->fname = 0;\n"
   "      self->fstr = 0;\n"
   "#ifdef ISTREAM_OVER_HTTP\n"
   "      self->tstr = 0;\n"
   "#endif\n"
   "#ifdef ISTREAM_OVER_XROOTD\n"
   "      self->rstr = 0;\n"
   "#endif\n"
   "      self->istr = 0;\n"
   "   }\n"
   "   return (PyObject*)self;\n"
   "}\n"
   "\n"
   "static int\n"
   "_istream_init(_istream *self, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   const char *kwlist[] = {\"file\", NULL};\n"
   "   char *filename;\n"
   "   if (PyArg_ParseTupleAndKeywords(args, kwds, \"s\", (char**)kwlist, &filename))\n"
   "   {\n"
   "      if (self->fname != 0)\n"
   "         delete self->fname;\n"
   "      if (self->istr != 0)\n"
   "         delete self->istr;\n"
   "      if (self->fstr != 0)\n"
   "         delete self->fstr;\n"
   "#ifdef ISTREAM_OVER_HTTP\n"
   "      if (self->tstr != 0)\n"
   "         delete self->tstr;\n"
   "#endif\n"
   "#ifdef ISTREAM_OVER_XROOTD\n"
   "      if (self->rstr != 0)\n"
   "         delete self->rstr;\n"
   "#endif\n"
   "      self->fname = new std::string(filename);\n"
   "      if (strncmp(filename, \"http://\", 7) == 0 || strncmp(filename, \"https://\", 8) == 0) {\n"
   "#ifdef ISTREAM_OVER_HTTP\n"
   "         try {\n"
   "            self->tstr = new httpIstream(filename);\n"
   "         }\n"
   "         catch (const std::exception& e) {\n"
   "            PyErr_Format(PyExc_IOError, e.what());\n"
   "            return -1;\n"
   "         }\n"
   "         if (! self->tstr->good()) {\n"
   "            PyErr_Format(PyExc_IOError, \"Cannot open input url %s\", filename);\n"
   "            return -1;\n"
   "         }\n"
   "#else\n"
   "         PyErr_Format(PyExc_IOError, \"Input streaming over http[s] disabled, see build options\");\n"
   "         return -1;\n"
   "#endif\n"
   "      }\n"
   "      else if (strncmp(filename, \"root://\", 7) == 0 || strncmp(filename, \"xrootd://\", 9) == 0) {\n"
   "#ifdef ISTREAM_OVER_XROOTD\n"
   "         try {\n"
   "            self->rstr = new xrootdIstream(filename);\n"
   "         }\n"
   "         catch (const std::exception& e) {\n"
   "            PyErr_Format(PyExc_IOError, e.what());\n"
   "            return -1;\n"
   "         }\n"
   "         if (! self->rstr->good()) {\n"
   "            PyErr_Format(PyExc_IOError, \"Cannot open input url %s\", filename);\n"
   "            return -1;\n"
   "         }\n"
   "#else\n"
   "         PyErr_Format(PyExc_IOError, \"Input streaming over xrootd disabled, see build options\");\n"
   "         return -1;\n"
   "#endif\n"
   "      }\n"
   "      else {\n"
   "         self->fstr = new std::ifstream(filename);\n"
   "         if (! self->fstr->good()) {\n"
   "            PyErr_Format(PyExc_IOError, \"Cannot open input file %s\", filename);\n"
   "            return -1;\n"
   "         }\n"
   "      }\n"
   "      try {\n"
   "         if (self->fstr)\n"
   "            self->istr = new istream(*self->fstr);\n"
   "#ifdef ISTREAM_OVER_HTTP\n"
   "         else if (self->tstr)\n"
   "            self->istr = new istream(*self->tstr);\n"
   "#endif\n"
   "#ifdef ISTREAM_OVER_XROOTD\n"
   "         else if (self->rstr)\n"
   "            self->istr = new istream(*self->rstr);\n"
   "#endif\n"
   "         else\n"
   "            PyErr_Format(PyExc_IOError, \"Cannot access input file %s\", filename);\n"
   "      }\n"
   "      catch (std::exception& e) {\n"
   "         PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "         return -1;\n"
   "      }\n"
   "      return 0;\n"
   "   }\n"
   "   return -1; \n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_getCompression(_istream *self, void *closure)\n"
   "{\n"
   "   return Py_BuildValue(\"i\", self->istr->getCompression());\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_getIntegrityChecks(_istream *self, void *closure)\n"
   "{\n"
   "   return Py_BuildValue(\"i\", self->istr->getIntegrityChecks());\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_getPosition(_istream *self, void *closure)\n"
   "{\n"
   "   streamposition *pos = new streamposition();\n"
   "   if (self->istr != 0)\n"
   "      try {\n"
   "         *pos = self->istr->getPosition();\n"
   "      }\n"
   "      catch (std::exception& e) {\n"
   "         PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "         return NULL;\n"
   "      }\n"
   "   PyObject *pos_obj = _streamposition_new(&_streamposition_type, 0, 0);\n"
   "   ((_streamposition*)pos_obj)->streampos = pos;\n"
   "   return pos_obj;\n"
   "}\n"
   "\n"
   "static int\n"
   "_istream_setPosition(_istream *self, PyObject *value, void *closure)\n"
   "{\n"
   "   if (Py_TYPE(value) != &_streamposition_type)\n"
   "   {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected argument type\");\n"
   "      return -1;\n"
   "   }\n"
   "   streamposition *pos = ((_streamposition*)value)->streampos;\n"
   "   if (pos == 0) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected null argument\");\n"
   "      return -1;\n"
   "   }\n"
   "   try {\n"
   "      self->istr->setPosition(*pos);\n"
   "   }\n"
   "   catch (std::exception& e) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "      return -1;\n"
   "   }\n"
   "   return 0;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_getRecordsRead(_istream *self, void *closure)\n"
   "{\n"
   "   size_t records = 0;\n"
   "   if (self->istr != 0)\n"
   "      try {\n"
   "         records = self->istr->getRecordsRead();\n"
   "      }\n"
   "      catch (std::exception& e) {\n"
   "         PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "         return NULL;\n"
   "      }\n"
   "   return PyLong_FromLongLong(records);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_getBytesRead(_istream *self, void *closure)\n"
   "{\n"
   "   size_t bytes = 0;\n"
   "   if (self->istr != 0)\n"
   "      try {\n"
   "         bytes = self->istr->getBytesRead();\n"
   "      }\n"
   "      catch (std::exception& e) {\n"
   "         PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "         return NULL;\n"
   "      }\n"
   "   return PyLong_FromLongLong(bytes);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_skip(PyObject *self, PyObject *args)\n"
   "{\n"
   "   int count=0;\n"
   "   if (! PyArg_ParseTuple(args, \"I\", &count)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"missing argument in skip\");\n"
   "      return NULL;\n"
   "   }\n"
   "   else if (count < 0) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"skip count cannot be negative\");\n"
   "      return NULL;\n"
   "   }\n"
   "   istream *istr = ((_istream*)self)->istr;\n"
   "   if (istr == 0) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected null istream ptr\");\n"
   "      return NULL;\n"
   "   }\n"
   "   istr->skip(count);\n"
   "   return PyLong_FromLong(0);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_read(PyObject *self, PyObject *args)\n"
   "{\n"
   "   istream *istr = ((_istream*)self)->istr;\n"
   "   if (istr == 0) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"unexpected null input stream\");\n"
   "      return NULL;\n"
   "   }\n"
   "   _HDDM *record_obj = (_HDDM*)_HDDM_new(&_HDDM_type, 0, 0);\n"
   "   record_obj->elem = new HDDM();\n"
   "   record_obj->host = (PyObject*)record_obj;\n"
   "   try {\n"
   "      Py_BEGIN_ALLOW_THREADS\n"
   "      *istr >> *record_obj->elem;\n"
   "      Py_END_ALLOW_THREADS\n"
   "   }\n"
   "   catch (std::exception& e) {\n"
   "      PyErr_SetString(PyExc_RuntimeError, e.what());\n"
   "      return NULL;\n"
   "   }\n"
   "   if (*istr) {\n"
   "      LOG_NEW(Py_TYPE(record_obj), 0, 1);\n"
   "      return (PyObject*)record_obj;\n"
   "   }\n"
   "   return NULL;\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_toString(PyObject *self, PyObject *args=0)\n"
   "{\n"
   "   std::stringstream ostr;\n"
   "   if (((_ostream*)self)->fname != 0)\n"
   "      ostr << \"hddm_" << classPrefix << ".istream(\\\"\"\n"
   "           << *((_istream*)self)->fname << \"\\\")\";\n"
   "   else\n"
   "      ostr << \"hddm_" << classPrefix << ".istream(NULL)\";\n"
   "   return PyUnicode_FromString(ostr.str().c_str());\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_toRepr(PyObject *self, PyObject *args=0)\n"
   "{\n"
   "   std::stringstream ostr;\n"
   "   ostr << \"\\\'\";\n"
   "   if (((_ostream*)self)->fname != 0)\n"
   "      ostr << \"hddm_" << classPrefix << ".istream(\\\"\"\n"
   "           << *((_istream*)self)->fname << \"\\\")\";\n"
   "   else\n"
   "      ostr << \"hddm_" << classPrefix << ".istream()\";\n"
   "   ostr << \"\\\'\";\n"
   "   return PyUnicode_FromString(ostr.str().c_str());\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_istream_iter(PyObject *self)\n"
   "{\n"
   "   Py_INCREF(self);\n"
   "   return self;\n"
   "}\n"
   "static PyObject*\n"
   "_istream_next(PyObject *self)\n"
   "{\n"
   "   PyObject *rec = _istream_read(self, 0);\n"
   "   if (rec == NULL)\n"
   "      PyErr_SetString(PyExc_StopIteration, \"no more data on input stream\");\n"
   "   return rec;\n"
   "}\n"
   "\n"
   "static PyGetSetDef _istream_getsetters[] = {\n"
   "   {(char*)\"compression\", \n"
   "    (getter)_istream_getCompression, 0,\n"
   "    (char*)\"istream compression mode (k_no_compression, k_z_compression, ...)\",\n"
   "    NULL},\n"
   "   {(char*)\"integrityChecks\", \n"
   "    (getter)_istream_getIntegrityChecks, 0,\n"
   "    (char*)\"istream data integrity checking mode (k_no_integrity, ...)\",\n"
   "    NULL},\n"
   "   {(char*)\"position\", \n"
   "    (getter)_istream_getPosition, (setter)_istream_setPosition,\n"
   "    (char*)\"input stream position\",\n"
   "    NULL},\n"
   "   {(char*)\"recordsRead\", \n"
   "    (getter)_istream_getRecordsRead, 0,\n"
   "    (char*)\"total records read from istream\",\n"
   "    NULL},\n"
   "   {(char*)\"bytesRead\", \n"
   "    (getter)_istream_getBytesRead, 0,\n"
   "    (char*)\"total bytes read from istream\",\n"
   "    NULL},\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyMemberDef _istream_members[] = {\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyMethodDef _istream_methods[] = {\n"
   "   {\"read\",  _istream_read, METH_NOARGS,\n"
   "    \"read a HDDM record from the input stream.\"},\n"
   "   {\"skip\",  _istream_skip, METH_VARARGS,\n"
   "    \"skip ahead given number of HDDM records in the input stream.\"},\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "static PyTypeObject _istream_type = {\n"
   "    PyVarObject_HEAD_INIT(NULL,0)\n"
   "    \"hddm_" << classPrefix << ".istream\",          /*tp_name*/\n"
   "    sizeof(_istream),          /*tp_basicsize*/\n"
   "    0,                         /*tp_itemsize*/\n"
   "    (destructor)_istream_dealloc, /*tp_dealloc*/\n"
   "    0,                         /*tp_print*/\n"
   "    0,                         /*tp_getattr*/\n"
   "    0,                         /*tp_setattr*/\n"
   "    0,                         /*tp_compare*/\n"
   "    (reprfunc)_istream_toRepr, /*tp_repr*/\n"
   "    0,                         /*tp_as_number*/\n"
   "    0,                         /*tp_as_sequence*/\n"
   "    0,                         /*tp_as_mapping*/\n"
   "    0,                         /*tp_hash */\n"
   "    0,                         /*tp_call*/\n"
   "    (reprfunc)_istream_toString, /*tp_str*/\n"
   "    0,                         /*tp_getattro*/\n"
   "    0,                         /*tp_setattro*/\n"
   "    0,                         /*tp_as_buffer*/\n"
   "    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/\n"
   "    \"hddm_" << classPrefix << " input stream\",    /* tp_doc */\n"
   "    0,                         /* tp_traverse */\n"
   "    0,                         /* tp_clear */\n"
   "    0,                         /* tp_richcompare */\n"
   "    0,                         /* tp_weaklistoffset */\n"
   "    _istream_iter,             /* tp_iter */\n"
   "    _istream_next,             /* tp_iternext */\n"
   "    _istream_methods,          /* tp_methods */\n"
   "    _istream_members,          /* tp_members */\n"
   "    _istream_getsetters,       /* tp_getset */\n"
   "    0,                         /* tp_base */\n"
   "    0,                         /* tp_dict */\n"
   "    0,                         /* tp_descr_get */\n"
   "    0,                         /* tp_descr_set */\n"
   "    0,                         /* tp_dictoffset */\n"
   "    (initproc)_istream_init,   /* tp_init */\n"
   "    0,                         /* tp_alloc */\n"
   "    _istream_new,              /* tp_new */\n"
   "};\n"
   ;

   // special module functions providing support for
   // reading and writing hddm records to/from hdf5 files
 
   builder.pyFile <<
   "\n"
   "#ifdef HDF5_SUPPORT\n"
   "static PyObject*\n"
   "_HDDM_hdf5FileCreate(PyObject *self, PyObject *args)\n"
   "{\n"
   "   char *name;\n"
   "   int flags = 0;\n"
   "   if (! PyArg_ParseTuple(args, \"s|I\", &name, &flags)) {\n"
   "      return NULL;\n"
   "   }\n"
   "   flags = (flags == 0)? H5F_ACC_TRUNC : flags;\n"
   "   hid_t result = hddm_" << classPrefix <<
   "::HDDM::hdf5FileCreate(std::string(name), flags);\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5FileOpen(PyObject *self, PyObject *args)\n"
   "{\n"
   "   char *name;\n"
   "   int flags = 0;\n"
   "   if (! PyArg_ParseTuple(args, \"s|I\", &name, &flags)) {\n"
   "      return NULL;\n"
   "   }\n"
   "   flags = (flags == 0)? H5F_ACC_RDONLY : flags;\n"
   "   hid_t result=0;\n"
   "   try {\n"
   "      result = hddm_" << classPrefix <<
   "::HDDM::hdf5FileOpen(std::string(name), flags);\n"
   "   }\n"
   "   catch (...) {\n"
   "      result = -1;\n"
   "   }\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5FileStamp(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   PyObject *ptags = 0;\n"
   "   if (! PyArg_ParseTuple(args, \"k|O!\", &file_id, &PyList_Type, &ptags)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5FileStamp\");\n"
   "      return NULL;\n"
   "   }\n"
   "   std::vector<PyObject*> tags_unicode;\n"
   "   std::vector<PyObject*> tags_ascii;\n"
   "   std::vector<char*> tags;\n"
   "   if (ptags != 0) {\n"
   "      int ntags = PyList_Size(ptags);\n"
   "      for (int i=0; i < ntags; i++) {\n"
   "         PyObject *pitem = PyList_GetItem(ptags, i);\n"
   "         PyObject *pitem_str = PyObject_Str(pitem);\n"
   "         tags_unicode.push_back(pitem_str);\n"
   "         PyObject *pitem_ascii = pitem_str;\n"
   "         if (PyUnicode_Check(pitem_str)) {\n"
   "            pitem_ascii = PyUnicode_AsEncodedString(pitem_str, \"ASCII\", \"strict\");\n"
   "            tags_ascii.push_back(pitem_ascii);\n"
   "         }\n"
   "#if PY_MAJOR_VERSION < 3\n"
   "         tags.push_back(PyString_AsString(pitem_ascii));\n"
   "#else\n"
   "         tags.push_back(PyBytes_AsString(pitem_ascii));\n"
   "#endif\n"
   "      }\n"
   "   }\n"
   "   tags.push_back(0);\n"
   "   herr_t result = hddm_" << classPrefix <<
   "::HDDM::hdf5FileStamp(file_id, &tags[0]);\n"
   "   for (auto p : tags_unicode) {\n"
   "      Py_XDECREF(p);\n"
   "   }\n"
   "   for (auto p : tags_ascii) {\n"
   "      Py_XDECREF(p);\n"
   "   }\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5FileCheck(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   PyObject *ptags = 0;\n"
   "   if (! PyArg_ParseTuple(args, \"k|O!\", &file_id, &PyList_Type, &ptags)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5FileCheck\");\n"
   "      return NULL;\n"
   "   }\n"
   "   std::vector<PyObject*> tags_unicode;\n"
   "   std::vector<PyObject*> tags_ascii;\n"
   "   std::vector<char*> tags;\n"
   "   if (ptags != 0) {\n"
   "      int ntags = PyList_Size(ptags);\n"
   "      for (int i=0; i < ntags; i++) {\n"
   "         PyObject *pitem = PyList_GetItem(ptags, i);\n"
   "         PyObject *pitem_str = PyObject_Str(pitem);\n"
   "         tags_unicode.push_back(pitem_str);\n"
   "         PyObject *pitem_ascii = pitem_str;\n"
   "         if (PyUnicode_Check(pitem_str)) {\n"
   "            pitem_ascii = PyUnicode_AsEncodedString(pitem_str, \"ASCII\", \"strict\");\n"
   "            tags_ascii.push_back(pitem_ascii);\n"
   "         }\n"
   "#if PY_MAJOR_VERSION < 3\n"
   "         tags.push_back(PyString_AsString(pitem_ascii));\n"
   "#else\n"
   "         tags.push_back(PyBytes_AsString(pitem_ascii));\n"
   "#endif\n"
   "      }\n"
   "   }\n"
   "   tags.push_back(0);\n"
   "   herr_t result=0;\n"
   "   try {\n"
   "      result = hddm_" << classPrefix <<
   "::HDDM::hdf5FileCheck(file_id, &tags[0]);\n"
   "   }\n"
   "   catch (...) {\n"
   "      result = -1;\n"
   "   }\n"
   "   for (auto p : tags_unicode) {\n"
   "      Py_XDECREF(p);\n"
   "   }\n"
   "   for (auto p : tags_ascii) {\n"
   "      Py_XDECREF(p);\n"
   "   }\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5GetEntries(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   if (! PyArg_ParseTuple(args, \"k\", &file_id)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5GetEntries\");\n"
   "      return NULL;\n"
   "   }\n"
   "   long int entries = hddm_" << classPrefix <<
   "::HDDM::hdf5GetEntries(file_id);\n"
   "   return PyLong_FromLong(entries);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5FileClose(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   if (! PyArg_ParseTuple(args, \"k\", &file_id)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5FileClose\");\n"
   "      return NULL;\n"
   "   }\n"
   "   herr_t result = hddm_" << classPrefix <<
   "::HDDM::hdf5FileClose(file_id);\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5DocumentString(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   if (! PyArg_ParseTuple(args, \"k\", &file_id)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5DocumentString\");\n"
   "      return NULL;\n"
   "   }\n"
   "   std::string docstring = hddm_" << classPrefix <<
   "::HDDM::hdf5DocumentString(file_id);\n"
   "   return PyUnicode_FromString(docstring.c_str());\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5GetChunksize(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   if (! PyArg_ParseTuple(args, \"k\", &file_id)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5GetChunksize\");\n"
   "      return NULL;\n"
   "   }\n"
   "   hsize_t result = hddm_" << classPrefix <<
   "::HDDM::hdf5GetChunksize(file_id);\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5SetChunksize(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   hsize_t chunksize;\n"
   "   if (! PyArg_ParseTuple(args, \"kk\", &file_id, &chunksize)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5SetChunksize\");\n"
   "      return NULL;\n"
   "   }\n"
   "   herr_t result = hddm_" << classPrefix <<
   "::HDDM::hdf5SetChunksize(file_id, chunksize);\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5GetFilters(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   PyObject *pfilters;\n"
   "   if (! PyArg_ParseTuple(args, \"kO!\", &file_id, &PyList_Type, &pfilters)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5GetFilters\");\n"
   "      return NULL;\n"
   "   }\n"
   "   std::vector<H5Z_filter_t> filters;\n"
   "   herr_t result = hddm_" << classPrefix <<
   "::HDDM::hdf5GetFilters(file_id, filters);\n"
   "   PyList_SetSlice(pfilters, 0, PyList_Size(pfilters), 0);\n"
   "   int nfilters = filters.size();\n"
   "   for (int i=0; i < nfilters; i++) {\n"
   "      PyList_Append(pfilters, PyLong_FromLong(filters[i]));\n"
   "   }\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "static PyObject*\n"
   "_HDDM_hdf5SetFilters(PyObject *self, PyObject *args)\n"
   "{\n"
   "   hid_t file_id;\n"
   "   PyObject *pfilters;\n"
   "   if (! PyArg_ParseTuple(args, \"kO!\", &file_id, &PyList_Type, &pfilters)) {\n"
   "      PyErr_SetString(PyExc_TypeError, \"invalid argument in hdf5SetFilters\");\n"
   "      return NULL;\n"
   "   }\n"
   "   std::vector<H5Z_filter_t> filters;\n"
   "   int nfilters = PyList_Size(pfilters);\n"
   "   for (int i=0; i < nfilters; i++) {\n"
   "      PyObject *pitem = PyList_GetItem(pfilters, i);\n"
   "      filters.push_back(PyLong_AsLong(pitem));\n"
   "   }\n"
   "   herr_t result = hddm_" << classPrefix <<
   "::HDDM::hdf5SetFilters(file_id, filters);\n"
   "   return PyLong_FromLong(result);\n"
   "}\n"
   "#endif\n";

   builder.pyFile <<
   "\n"
   "\n"
   "// module declarations\n"
   "\n"
   "static PyMethodDef hddm_" << classPrefix << "_methods[] = {\n"
   "#ifdef HDF5_SUPPORT\n"
   "   {\"hdf5DocumentString\", _HDDM_hdf5DocumentString, METH_VARARGS,\n"
   "    \"read the hddm document string from this hdf5 file\"},\n"
   "   {\"hdf5FileCreate\", _HDDM_hdf5FileCreate, METH_VARARGS,\n"
   "    \"create a new hdf5 and open for writing hddm records\"},\n"
   "   {\"hdf5FileOpen\", _HDDM_hdf5FileOpen, METH_VARARGS,\n"
   "    \"open an existing hdf5 file for reading hddm records\"},\n"
   "   {\"hdf5FileClose\", _HDDM_hdf5FileClose, METH_VARARGS,\n"
   "    \"close an open hdf5 file and free its hdf5 resources\"},\n"
   "   {\"hdf5FileStamp\", _HDDM_hdf5FileStamp, METH_VARARGS,\n"
   "    \"this is implicitly called by hdf5FileCreate\"},\n"
   "   {\"hdf5FileCheck\", _HDDM_hdf5FileCheck, METH_VARARGS,\n"
   "    \"this is implicitly called by hdf5FileOpen\"},\n"
   "   {\"hdf5GetEntries\", _HDDM_hdf5GetEntries, METH_VARARGS,\n"
   "    \"returns the number of hddm records in a hdf5 file\"},\n"
   "   {\"hdf5GetFilters\", _HDDM_hdf5GetFilters, METH_VARARGS,\n"
   "    \"gets the list of filters active on a hdf5 file\"},\n"
   "   {\"hdf5SetFilters\", _HDDM_hdf5SetFilters, METH_VARARGS,\n"
   "    \"sets the list of filters active on a hdf5 file\"},\n"
   "   {\"hdf5GetChunksize\", _HDDM_hdf5GetChunksize, METH_VARARGS,\n"
   "    \"gets the hddm dataset chunksize on an open hdf5 file\"},\n"
   "   {\"hdf5SetChunksize\", _HDDM_hdf5SetChunksize, METH_VARARGS,\n"
   "    \"sets the hddm dataset chunksize on an open hdf5 file\"},\n"
   "#endif\n"
   "   {NULL}  /* Sentinel */\n"
   "};\n"
   "\n"
   "char hddm_" << classPrefix << "_doc[] = \"Python module for "
   "hddm_" << classPrefix << " i/o package\";\n"
   "\n"
   "#if PY_MAJOR_VERSION >= 3\n"
   "  static struct PyModuleDef moduledef = {\n"
   "    PyModuleDef_HEAD_INIT,\n"
   "    \"hddm_" << classPrefix << "\",            /* m_name */\n"
   "    hddm_" << classPrefix << "_doc,          /* m_doc */\n"
   "    -1,                  /* m_size */\n"
   "    hddm_" << classPrefix << "_methods,      /* m_methods */\n"
   "    NULL,                /* m_reload */\n"
   "    NULL,                /* m_traverse */\n"
   "    NULL,                /* m_clear */\n"
   "    NULL,                /* m_free */\n"
   "  };\n"
   "#endif\n"
   "\n"
   "static PyObject *\n"
   "hddm_" << classPrefix << "_init(void) \n"
   "{\n"
   "   PyObject* m;\n"
   "\n"
   "#if PY_MAJOR_VERSION >= 3\n"
   "   m = PyModule_Create(&moduledef);\n"
   "#else\n"
   "   m = Py_InitModule3(\"hddm_" << classPrefix << "\","
   " hddm_" << classPrefix << "_methods,"
   " hddm_" << classPrefix << "_doc);\n"
   "#endif\n"
   "\n"
   "   if (m == NULL)\n"
   "      return NULL;\n"
   "\n"
   ;

   std::map<XtString,XtString>::iterator titer;
   for (titer = builder.typesList.begin(); 
        titer != builder.typesList.end(); ++titer)
   {
      builder.pyFile <<
      "   if (PyType_Ready(&" << titer->second << ") < 0)\n"
      "      return NULL;\n"
      "   Py_INCREF(&" << titer->second << ");\n"
      "   PyModule_AddObject(m, \"" << titer->first << "\","
      " (PyObject*)&" << titer->second << ");\n"
      ;
   }

   builder.pyFile <<
   "\n"
   "   PyModule_AddIntConstant(m, \"k_default_status\", k_default_status);\n"
   "   PyModule_AddIntConstant(m, \"k_bits_compression\", k_bits_compression);\n"
   "   PyModule_AddIntConstant(m, \"k_no_compression\", k_no_compression);\n"
   "   PyModule_AddIntConstant(m, \"k_z_compression\", k_z_compression);\n"
   "   PyModule_AddIntConstant(m, \"k_bz2_compression\", k_bz2_compression);\n"
   "   PyModule_AddIntConstant(m, \"k_bits_integrity\", k_bits_integrity);\n"
   "   PyModule_AddIntConstant(m, \"k_no_integrity\", k_no_integrity);\n"
   "   PyModule_AddIntConstant(m, \"k_crc32_integrity\", k_crc32_integrity);\n"
   "   PyModule_AddIntConstant(m, \"k_bits_randomaccess\", k_bits_randomaccess);\n"
   "   PyModule_AddIntConstant(m, \"k_can_reposition\", k_can_reposition);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_unknown\", k_hddm_unknown);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_int\", k_hddm_int);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_long\", k_hddm_long);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_float\", k_hddm_float);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_double\", k_hddm_double);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_boolean\", k_hddm_boolean);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_string\", k_hddm_string);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_anyURI\", k_hddm_anyURI);\n"
   "   PyModule_AddIntConstant(m, \"k_hddm_Particle_t\", k_hddm_Particle_t);\n"
   "   std::string docstring = HDDM::DocumentString();\n"
   "   PyModule_AddStringConstant(m, \"DocumentString\", docstring.c_str());\n"
   "\n"
   "#ifdef HDF5_SUPPORT\n"
   "   PyModule_AddIntConstant(m, \"k_hdf5_gzip_filter\", k_hdf5_gzip_filter);\n"
   "   PyModule_AddIntConstant(m, \"k_hdf5_szip_filter\", k_hdf5_szip_filter);\n"
   "   PyModule_AddIntConstant(m, \"k_hdf5_bzip2_plugin\", k_hdf5_bzip2_plugin);\n"
   "   PyModule_AddIntConstant(m, \"k_hdf5_blosc_plugin\", k_hdf5_blosc_plugin);\n"
   "   PyModule_AddIntConstant(m, \"k_hdf5_bshuf_plugin\", k_hdf5_bshuf_plugin);\n"
   "   PyModule_AddIntConstant(m, \"k_hdf5_jpeg_plugin\", k_hdf5_jpeg_plugin);\n"
   "   PyModule_AddIntConstant(m, \"k_hdf5_lz4_plugin\", k_hdf5_lz4_plugin);\n"
   "   PyModule_AddIntConstant(m, \"k_hdf5_lzf_plugin\", k_hdf5_lzf_plugin);\n"
   "#endif\n"
   "\n"
   "   return m;\n"
   "}\n"
   "\n"
   "#if PY_MAJOR_VERSION < 3\n"
   "   PyMODINIT_FUNC\n"
   "   inithddm_" << classPrefix << "(void)\n"
   "   {\n"
   "      hddm_" << classPrefix << "_init();\n"
   "   }\n"
   "#else\n"
   "   PyMODINIT_FUNC\n"
   "   PyInit_hddm_" << classPrefix << "(void)\n"
   "   {\n"
   "      return hddm_" << classPrefix << "_init();\n"
   "   }\n"
   "#endif\n"
   ;

/* convert cpy filename "<dirpath>/hddm_X.cpy"
 * to setup filename "<dirpath>/setup_hddm_X.py"
 */
   size_t p1 = pyname.rfind("pyhddm_");
   pyname.erase(p1, 2);
   pyname.insert(p1, "setup_");
   size_t p2 = pyname.rfind("cpy");
   pyname.erase(p2, 1);

   std::ofstream pysetup(pyname.c_str());
   pysetup <<
   "#\n"
   "# This file was generated by the hddm-py utility\n"
   "# from the project https://github.com/rjones30/HDDM\n"
   "#\n"
   "import glob\n"
   "import sys\n"
   "import os\n"
   "import sysconfig\n"
   "from distutils.core import setup, Extension\n"
   "from shutil import copyfile, rmtree\n"
   "import re\n"
   "\n"
   "# Remove the \"-Wstrict-prototypes\" compiler options,\n"
   "# which isn't valid for C++\n"
   "import distutils.sysconfig\n"
   "cfg_vars = distutils.sysconfig.get_config_vars()\n"
   "for key,value in cfg_vars.items():\n"
   "   if type(value) == str:\n"
   "      cfg_vars[key] = value.replace(\"-Wstrict-prototypes\", \"\")\n"
   "\n"
   "os.environ['CC'] = 'g++'  # distutils uses compiler options unique to gcc\n"
   "\n"
   "build_dir = 'build_hddm_" + classPrefix + "'\n"
   "if len(sys.argv) == 1:\n"
   "   sys.argv += ['build', '-b', build_dir]\n"
   "\n"
   "source_dir = os.path.dirname(os.path.realpath(__file__))\n"
   "if os.environ.get('HDDM_DIR'):\n"
   "   hddm_dir = os.environ['HDDM_DIR']\n"
   "else:\n"
   "   print('HDDM_DIR not defined, module creation failed!')\n"
   "   print('environment is:')\n"
   "   for key in os.environ:\n"
   "     print('  {0}: {1}'.format(key, os.environ[key]))\n"
   "   sys.exit(1)\n"
   "source_file = 'pyhddm_" + classPrefix + ".cpp'\n"
   "source_files = [source_file, os.path.join(source_dir, 'hddm_" + classPrefix + "++.cpp')]\n"
   "copyfile(os.path.join(source_dir, 'pyhddm_" + classPrefix + ".cpy'), source_file)\n"
   "my_include_dirs = [source_dir, os.path.join(hddm_dir, 'include')]\n"
   "my_library_dirs = [os.path.join(hddm_dir, 'lib'),\n"
   "                   os.path.join(hddm_dir, 'lib64'),\n"
   "                   os.path.join(os.sep, 'usr', 'lib64'),\n"
   "                  ]\n"
   "my_libraries = [\n"
   "                'xstream',\n"
   "                'bz2',\n"
   "               ]\n"
   "for dir in my_library_dirs:\n"
   "   for libz in ['libz.a', 'libz.so']:\n"
   "      if os.path.exists(os.path.join(dir, libz)):\n"
   "          my_libraries.append('z')\n"
   "          break\n"
   "   for zlib in ['zlibstatic.lib']:\n"
   "      if os.path.exists(os.path.join(dir, zlib)):\n"
   "          my_libraries.append('zlibstatic')\n"
   "          break\n"
   "   for zlib in ['zlib.lib']:\n"
   "      if os.path.exists(os.path.join(dir, zlib)):\n"
   "          my_libraries.append('zlib')\n"
   "          break\n"
   "   for libpthread in ['libpthread.a', 'libpthread.so']:\n"
   "      if os.path.exists(os.path.join(dir, libpthread)):\n"
   "          my_libraries.append('pthread')\n"
   "          break\n"
   "   for libpthread in ['libpthreadVC3.lib', 'pthreadVC3.lib']:\n"
   "      if os.path.exists(os.path.join(dir, libpthread)):\n"
   "          my_libraries.append('libpthreadVC3')\n"
   "          my_libraries.append('Ws2_32')\n"
   "          break\n"
   "if os.environ.get('XSTREAM_SRC'):\n"
   "   xstream_src = os.environ['XSTREAM_SRC']\n"
   "   my_include_dirs += [os.path.join(xstream_src, 'include')]\n"
   "if os.environ.get('HDDM_SRC'):\n"
   "   hddm_src = os.environ['HDDM_SRC']\n"
   "   my_include_dirs += [hddm_src]\n"
   "if os.environ.get('XSTREAM_DIR'):\n"
   "   xstream_dir = os.environ['XSTREAM_DIR']\n"
   "   xstream_libdir = os.path.join(xstream_dir, 'src')\n"
   "   for lib in os.listdir(xstream_libdir):\n"
   "      if lib == 'libxstream.a':\n"
   "         my_library_dirs += [xstream_libdir]\n"
   "      elif lib == 'Release':\n"
   "         my_library_dirs += [os.path.join(xstream_libdir, 'Release')]\n"
   "if os.environ.get('COMPILER_STD_OPTION'):\n"
   "   my_extra_cxxflags = [os.environ['COMPILER_STD_OPTION']]\n"
   "else:\n"
   "   my_extra_cxxflags = ['-std=c++20']\n"
   "if os.environ.get('HDF5_INCLUDE_DIRS'):\n"
   "   my_include_dirs += os.environ['HDF5_INCLUDE_DIRS'].split(',')\n"
   "if os.environ.get('HDF5_LIBRARIES'):\n"
   "   for lib in os.environ['HDF5_LIBRARIES'].split(','):\n"
   "      my_libraries += [re.sub(r'\\.[^\\.]*$', '', re.sub('.*/lib/*', '', lib))]\n"
   "   my_extra_cxxflags += ['-DHDF5_SUPPORT']\n"
   "for lib in glob.glob(os.path.join(hddm_dir, 'lib*', 'libhddmstream*')):\n"
   "   my_libraries += [re.sub(r'\\.so$', '', re.sub('.*/lib', '', lib))]\n"
   "\n"
   "if os.environ.get('HTTP_ISTREAM'):\n"
   "   my_include_dirs.insert(0, os.path.join(hddm_src, '..', 'httpstream'))\n"
   "   for lib in os.listdir(os.environ['HTTP_ISTREAM']):\n"
   "      if lib == 'libhttpstream.a':\n"
   "         my_library_dirs.insert(0, os.environ['HTTP_ISTREAM'])\n"
   "      elif lib == 'Release':\n"
   "         my_library_dirs.insert(0, os.path.join(os.environ['HTTP_ISTREAM'], 'Release'))\n"
   "   my_extra_cxxflags += ['-DISTREAM_OVER_HTTP']\n"
   "   my_libraries += ['httpstream']\n"
   "   my_libraries += os.environ['HTTP_ISTREAM_LIBS'].split(',')\n"
   "if os.environ.get('XROOTD_ISTREAM'):\n"
   "   my_include_dirs.insert(0, os.path.join(os.path.sep, 'usr', 'include', 'xrootd'))\n"
   "   my_include_dirs.insert(0, os.path.join(hddm_dir, 'include', 'xrootd'))\n"
   "   my_include_dirs.insert(0, os.path.join(hddm_src, '..', 'xrootdstream'))\n"
   "   for lib in os.listdir(os.environ['XROOTD_ISTREAM']):\n"
   "      if lib == 'libxrootdstream.a':\n"
   "         my_library_dirs.insert(0, os.environ['XROOTD_ISTREAM'])\n"
   "      elif lib == 'Release':\n"
   "         my_library_dirs.insert(0, os.path.join(os.environ['XROOTD_ISTREAM'], 'Release'))\n"
   "   my_extra_cxxflags += ['-DISTREAM_OVER_XROOTD']\n"
   "   my_libraries += ['xrootdstream']\n"
   "   if os.environ.get('XROOTD_INCLUDE_DIRS'):\n"
   "      my_include_dirs += os.environ['XROOTD_INCLUDE_DIRS'].split(',')\n"
   "   if os.environ.get('XROOTD_LIBRARIES'):\n"
   "      for lib in os.environ['XROOTD_LIBRARIES'].split(','):\n"
   "         libdir = '/'.join(lib.split('/')[:-1])\n"
   "         libname = lib.split('/')[-1]\n"
   "         libroot = re.sub(r'lib(.*)\\..*', r'\\1', libname)\n"
   "         my_library_dirs.insert(0, libdir)\n"
   "         my_libraries.append(libroot)\n"
   "   else:\n"
   "      my_libraries += os.environ['XROOTD_ISTREAM_LIBS'].split(',')\n"
   "if 'macos' in sysconfig.get_platform():\n"
   "   my_extra_cxxflags += ['-mmacosx-version-min=10.15']\n"
   "my_include_dirs = [s for s in my_include_dirs if s]\n"
   "print('my_include_dirs are', my_include_dirs)\n"
   "my_library_dirs = [s for s in my_library_dirs if s]\n"
   "print('my_library_dirs are', my_library_dirs)\n"
   "my_libraries = [s for s in my_libraries if s]\n"
   "print('my_libraries are', my_libraries)\n"
   "module1 = Extension('hddm_" + classPrefix + "',\n"
   "                    include_dirs = my_include_dirs,\n"
   "                    library_dirs = my_library_dirs,\n"
   "                    libraries = my_libraries,\n"
   "                    extra_compile_args = my_extra_cxxflags,\n"
   "                    sources = source_files)\n"
   "\n"
   "setup (name = 'hddm_" << classPrefix << "',\n"
   "       version = '1.0',\n"
   "       description = 'HDDM data model i/o package',\n"
   "       ext_modules = [module1])\n"
   "\n"
   "os.remove(source_file)\n"
   "for dname in os.listdir(build_dir):\n"
   "    for dll in os.listdir(os.path.join(build_dir, dname)):\n"
   "        if re.match(r'.*\\.so', dll):\n"
   "            src = os.path.join(build_dir, dname, dll)\n"
   "            dest = os.path.join(source_dir, dll)\n"
   "            copyfile(src, dest)\n"
   "        elif re.match(r'.*\\.pyd', dll):\n"
   "            src = os.path.join(build_dir, dname, dll)\n"
   "            dest = os.path.join(source_dir, dll)\n"
   "            copyfile(src, dest)\n"
   "#rmtree(build_dir)\n"
   ;

   XMLPlatformUtils::Terminate();
   return 0;
}

XtString XtString::plural()
{
   XtString p(*this);
   XtString::size_type len = p.size();
   if (len > 3 && p.substr(len-3,3) == "tum")
   {
      p.replace(len-3,3,"ta");
   }
   else if (len > 1 && p.substr(len-3,3) == "ies")
   {
      p.replace(len-3,3,"iesList");
   }
   else if (len > 2 && p.substr(len-2,2) == "ex")
   {
      p.replace(len-2,2,"ices");
   }
   else if (len > 2 && p.substr(len-2,2) == "sh")
   {
      p.replace(len-2,2,"shes");
   }
   else if (len > 1 && p.substr(len-1,1) == "s")
   {
      p.replace(len-1,1,"ses");
   }
   else if (len > 1)
   {
      p += "s";
   }
   return p;
}

/* Map from tag name to name of the corresponding class
 * for the case of simple tags (those that do not repeat)
 */
XtString XtString::simpleType()
{
   XtString p(*this);
   p[0] = toupper(p[0]);
   return p;
}

/* Map from tag name to name of the corresponding class
 * for the case of list tags (those that may repeat)
 */
XtString XtString::listType()
{
   XtString r(*this);
   r[0] = toupper(r[0]);
   r = r + "List";
   return r;
}

/* Map from tag name to name of the corresponding class
 * for the case of link tags (those that do not repeat)
 */
XtString XtString::linkType()
{
   XtString r(*this);
   r[0] = toupper(r[0]);
   r = r + "Link";
   return r;
}

/* Look for a named element in a list of element pointers
 * and return index of first instance in the list if found,
 * otherwise return -1;
 */
int CodeBuilder::element_in_list(XtString &name, parentList_t list)
{
   int n=0;
   parentList_t::iterator iter;
   for (iter = list.begin(); iter != list.end(); ++iter, ++n)
   {
      DOMElement *el = (DOMElement*)(*iter);
      XtString cnameS(el->getTagName());
      if (cnameS == name) {
         return n;
      }
   }
   return -1;
}

/* Verify that the tag group under this element does not collide
 * with existing tag group elref, otherwise exit with fatal error
 */
void CodeBuilder::checkConsistency(DOMElement* el, DOMElement* elref)
{
   XtString tagS(el->getTagName());
   if (el->getParentNode() == elref->getParentNode())
   {
      std::cerr
           << "hddm-py error: tag " << "\"" << tagS 
           << "\" is duplicated within one context in xml document."
       << std::endl;
      exit(1);
   }

   DOMNamedNodeMap* oldAttr = elref->getAttributes();
   DOMNamedNodeMap* newAttr = el->getAttributes();
   size_t listLength = oldAttr->getLength();
   for (unsigned int n = 0; n < listLength; n++)
   {
      XtString nameS(oldAttr->item(n)->getNodeName());
      XtString oldS(elref->getAttribute(X(nameS)));
      XtString newS(el->getAttribute(X(nameS)));
      if (nameS == "minOccurs")
      {
         continue;
      }
      else if (nameS == "maxOccurs")
      {
         int maxold = (oldS == "unbounded")? INT_MAX : atoi(S(oldS));
         int maxnew = (newS == "unbounded")? INT_MAX : atoi(S(newS));
     if ((maxold < 2 && maxnew > 1) || (maxold > 1 && maxnew < 2))
         {
            std::cerr
                 << "hddm-py error: inconsistent maxOccurs usage by tag "
                 << "\"" << tagS << "\" in xml document." << std::endl;
            exit(1);
         }
      }
      else if (newS != oldS)
      {
         std::cerr
              << "hddm-py error: inconsistent usage of attribute "
              << "\"" << nameS << "\" in tag "
              << "\"" << tagS << "\" in xml document." << std::endl;
         exit(1);
      }
   }
   listLength = newAttr->getLength();
   for (unsigned int n = 0; n < listLength; n++)
   {
      XtString nameS(newAttr->item(n)->getNodeName());
      XtString oldS(elref->getAttribute(X(nameS)));
      XtString newS(el->getAttribute(X(nameS)));
      if (nameS == "minOccurs")
      {
         continue;
      }
      else if (nameS == "maxOccurs")
      {
         int maxold = (oldS == "unbounded")? INT_MAX : atoi(S(oldS));
         int maxnew = (newS == "unbounded")? INT_MAX : atoi(S(newS));
     if ((maxold < 2 && maxnew > 1) || (maxold > 1 && maxnew < 2))
         {
            std::cerr
                 << "hddm-py error: inconsistent maxOccurs usage by tag "
                 << "\"" << tagS << "\" in xml document." << std::endl;
            exit(1);
         }
      }
      else if (newS != oldS)
      {
         std::cerr
              << "hddm-py error: inconsistent usage of attribute "
              << "\"" << nameS << "\" in tag "
              << "\"" << tagS << "\" in xml document." << std::endl;
         exit(1);
      }
   }
   DOMNodeList* oldList = elref->getChildNodes();
   DOMNodeList* newList = el->getChildNodes();
   listLength = oldList->getLength();
   if (newList->getLength() != listLength)
   {
      std::cerr
           << "hddm-py error: inconsistent usage of tag "
           << "\"" << tagS << "\" in xml document." << std::endl;
   exit(1);
   }
   for (unsigned int n = 0; n < listLength; n++)
   {
      DOMNode* cont = oldList->item(n);
      XtString nameS(cont->getNodeName());
      short type = cont->getNodeType();
      if (type == DOMNode::ELEMENT_NODE)
      {
         DOMNodeList* contList = el->getElementsByTagName(X(nameS));
         if (contList->getLength() != 1)
         {
             std::cerr
                  << "hddm-py error: inconsistent usage of tag "
                  << "\"" << tagS << "\" in xml document." << std::endl;
             exit(1);
         }
      }
   }
}

/* Write declaration of the classes for this tag */

void CodeBuilder::writeClassdef(DOMElement* el)
{
   XtString tagS(el->getTagName());
   pyFile << 
   "\n\n"
   "// wrap element class hddm_" << classPrefix << "::" << tagS.simpleType() <<
   " as hddm_" << classPrefix << "." << tagS.simpleType() << "\n"
   "\n"
   "typedef struct {\n"
   "   PyObject_HEAD\n"
   "   " << tagS.simpleType() << " *elem;\n"
   "   PyObject *host;\n"
   "} _" << tagS.simpleType() << ";\n"
   "\n"
   "static void\n"
   "_" << tagS.simpleType() << "_dealloc(_" << tagS.simpleType() << "* self)\n"
   "{\n"
   "   if (self->elem != 0) {\n"
   "      LOG_DEALLOC(Py_TYPE(self), 0, self->host == (PyObject*)self);\n"
   "      if (self->host == (PyObject*)self)\n"
   "         delete self->elem;\n"
   "      else\n"
   "         My_DECREF(self->host);\n"
   "   }\n"
   "   Py_TYPE(self)->tp_free((PyObject*)self);\n"
   "}\n"
   "\n"
   "static PyObject*\n"
   "_" << tagS.simpleType() <<
   "_new(PyTypeObject *type, PyObject *args, PyObject *kwds)\n"
   "{\n"
   "   _" << tagS.simpleType() << " *self;\n"
   "   self = (_" << tagS.simpleType() << "*)type->tp_alloc(type, 0);\n"
   "   if (self != NULL) {\n"
   "      self->elem = 0;\n"
   "      self->host = 0;\n"
   "   }\n"
   "   return (PyObject*)self;\n"
   "}\n"
   "\n"
   ;

   if (tagS == "HDDM")
   {
      pyFile << 
      "static int\n"
      "_HDDM_init(_HDDM *self, PyObject *args, PyObject *kwds)\n"
      "{\n"
      "   LOG_NEW(Py_TYPE(self), 0, 1);\n"
      "   self->elem = new HDDM();\n"
      "   if (self->elem == 0) {\n"
      "      PyErr_SetString(PyExc_RuntimeError, \"HDDM new constructor failed\");\n"
      "      return -1;\n"
      "   }\n"
      "   self->host = (PyObject*)self;\n"
      "   return 0;\n"
      "}\n"
      "\n"
      ;
      pyFile << 
      "#ifdef HDF5_SUPPORT\n"
      "static PyObject*\n"
      "_HDDM_hdf5FileRead(PyObject *self, PyObject *args)\n"
      "{\n"
      "   hid_t file_id;\n"
      "   long int entry = -1;\n"
      "   if (! PyArg_ParseTuple(args, \"k|l\", &file_id, &entry)) {\n"
      "      return NULL;\n"
      "   }\n"
      "   _HDDM *me = (_HDDM*)self;\n"
      "   herr_t result = me->elem->hdf5FileRead(file_id, entry);\n"
      "   return PyLong_FromLong(result);\n"
      "}\n"
      "static PyObject*\n"
      "_HDDM_hdf5FileWrite(PyObject *self, PyObject *args)\n"
      "{\n"
      "   hid_t file_id;\n"
      "   long int entry = -1;\n"
      "   if (! PyArg_ParseTuple(args, \"k|l\", &file_id, &entry)) {\n"
      "      return NULL;\n"
      "   }\n"
      "   _HDDM *me = (_HDDM*)self;\n"
      "   herr_t result = me->elem->hdf5FileWrite(file_id, entry);\n"
      "   return PyLong_FromLong(result);\n"
      "}\n"
      "#endif\n"
      ;
   }
   else
   {
      pyFile << 
      "static int\n"
      "_" << tagS.simpleType() << "_init(_" << tagS.simpleType() << 
      " *self, PyObject *args, PyObject *kwds)\n"
      "{\n"
      "   PyErr_SetString(PyExc_RuntimeError, \"illegal constructor\");\n"
      "   return -1;\n"
      "}\n"
      "\n"
      ;
   }

   std::map<XtString,XtString> attrList;
   DOMNamedNodeMap *myAttr = el->getAttributes();
   for (unsigned int n = 0; n < myAttr->getLength(); n++)
   {
      XtString attrS(myAttr->item(n)->getNodeName());
      XtString typeS(el->getAttribute(X(attrS)));
      attrList[attrS] = typeS;
   }
   parentList_t::iterator iter;
   for (iter = parents[tagS].begin(); iter != parents[tagS].end(); ++iter)
   {
      DOMElement *hostEl = (DOMElement*)(*iter);
      XtString hostS(hostEl->getTagName());
      DOMNamedNodeMap *hostAttr = hostEl->getAttributes();
      for (unsigned int n = 0; n < hostAttr->getLength(); n++)
      {
         XtString attrS(hostAttr->item(n)->getNodeName());
         if (attrList.find(attrS) != attrList.end())
         {
            continue;
         }
         XtString typeS(hostEl->getAttribute(X(attrS)));
         attrList[attrS] = typeS;
         XtString getS("get" + attrS.simpleType());
         pyFile << "static PyObject*\n" <<
                   "_" << tagS.simpleType() << "_" << getS << 
                   "(_" << tagS.simpleType() << " *self, void *closure)\n"
                   "{\n";
         if (typeS == "int")
         {
            pyFile << "   return PyLong_FromLong(self->elem->" 
                   << getS << "());\n";
         }
         else if (typeS == "long")
         {
            pyFile << "   return PyLong_FromLongLong(self->elem->" 
                   << getS << "());\n";
         }
         else if (typeS == "float")
         {
            pyFile << "   return PyFloat_FromDouble(self->elem->" 
                   << getS << "());\n";
         }
         else if (typeS == "double")
         {
            pyFile << "   return PyFloat_FromDouble(self->elem->" 
                   << getS << "());\n";
         }
         else if (typeS == "boolean")
         {
            pyFile << "   return PyBool_FromLong(self->elem->" 
                   << getS << "());\n";
         }
         else if (typeS == "string")
         {
            pyFile << "   std::string val(self->elem->" 
                   << getS << "());\n"
                   << "   return PyUnicode_FromString(val.c_str());\n";
            attrList[attrS] = "string";
         }
         else if (typeS == "anyURI")
         {
            pyFile << "   std::string val(self->elem->" 
                   << getS << "());\n"
                   << "   return PyUnicode_FromString(val.c_str());\n";
            attrList[attrS] = "string";
         }
         else if (typeS == "Particle_t")
         {
            pyFile << "   Particle_t p(self->elem->"
                   << getS << "());\n"
                   << "   std::string val(ParticleType(p));\n"
                   << "   return PyUnicode_FromString(val.c_str());\n";
         }
         else if (guessType(typeS) == "int")
         {
            pyFile << "   return PyLong_FromLong(self->elem->" 
                   << getS << "());\n";
         }
         else if (guessType(typeS) == "long")
         {
            pyFile << "   return PyLong_FromLongLong(self->elem->" 
                   << getS << "());\n";
         }
         else if (guessType(typeS) == "float")
         {
            pyFile << "   return PyFloat_FromDouble(self->elem->" 
                   << getS << "());\n";
         }
         else if (guessType(typeS) == "double")
         {
            pyFile << "   return PyFloat_FromDouble(self->elem->" 
                   << getS << "());\n";
         }
         else if (guessType(typeS) == "boolean")
         {
            pyFile << "   return PyBool_FromLong(self->elem->" 
                   << getS << "());\n";
         }
         else if (guessType(typeS) == "Particle_t")
         {
            pyFile << "   Particle_t p(self->elem->"
                   << getS << "());\n"
                   << "   std::string val(ParticleType(p));\n"
                   << "   return PyUnicode_FromString(val.c_str());\n";
         }
         else /* any values not matching the above are strings */
         {
            pyFile << "   std::string val(self->elem->" 
                   << getS << "());\n"
                   << "   return PyUnicode_FromString(val.c_str());\n";
            attrList[attrS] = "string";
         }
         pyFile << "}\n\n";
      }
   }

   std::map<XtString,int> setters;
   myAttr = el->getAttributes();
   for (unsigned int n = 0; n < myAttr->getLength(); n++)
   {
      XtString attrS(myAttr->item(n)->getNodeName());
      XtString typeS(el->getAttribute(X(attrS)));
      XtString getS("get" + attrS.simpleType());
      pyFile << "static PyObject*\n" <<
                "_" << tagS.simpleType() << "_" << getS << 
                "(_" << tagS.simpleType() << " *self, void *closure)\n"
                "{\n";
      if (typeS == "int")
      {
         pyFile << "   return PyLong_FromLong(self->elem->" 
                << getS << "());\n";
      }
      else if (typeS == "long")
      {
         pyFile << "   return PyLong_FromLongLong(self->elem->" 
                << getS << "());\n";
      }
      else if (typeS == "float")
      {
         pyFile << "   return PyFloat_FromDouble(self->elem->" 
                << getS << "());\n";
      }
      else if (typeS == "double")
      {
         pyFile << "   return PyFloat_FromDouble(self->elem->" 
                << getS << "());\n";
      }
      else if (typeS == "boolean")
      {
         pyFile << "   return PyBool_FromLong(self->elem->" 
                << getS << "());\n";
      }
      else if (typeS == "string")
      {
         pyFile << "   std::string val(self->elem->" 
                << getS << "());\n"
                << "   return PyUnicode_FromString(val.c_str());\n";
      }
      else if (typeS == "anyURI")
      {
         pyFile << "   std::string val(self->elem->" 
                << getS << "());\n"
                << "   return PyUnicode_FromString(val.c_str());\n";
      }
      else if (typeS == "Particle_t")
      {
         pyFile << "   Particle_t p(self->elem->"
                << getS << "());\n"
                << "   std::string val(ParticleType(p));\n"
                << "   return PyUnicode_FromString(val.c_str());\n";
      }
      else if (guessType(typeS) == "int")
      {
         pyFile << "   return PyLong_FromLong(self->elem->" 
                << getS << "());\n";
      }
      else if (guessType(typeS) == "long")
      {
         pyFile << "   return PyLong_FromLongLong(self->elem->" 
                << getS << "());\n";
      }
      else if (guessType(typeS) == "float")
      {
         pyFile << "   return PyFloat_FromDouble(self->elem->" 
                << getS << "());\n";
      }
      else if (guessType(typeS) == "double")
      {
         pyFile << "   return PyFloat_FromDouble(self->elem->" 
                << getS << "());\n";
      }
      else if (guessType(typeS) == "boolean")
      {
         pyFile << "   return PyBool_FromLong(self->elem->" 
                << getS << "());\n";
      }
      else if (guessType(typeS) == "Particle_t")
      {
         pyFile << "   Particle_t p(self->elem->"
                << getS << "());\n"
                << "   std::string val(ParticleType(p));\n"
                << "   return PyUnicode_FromString(val.c_str());\n";
      }
      else /* any values not matching the above are strings */
      {
         pyFile << "   std::string val(self->elem->" 
                << getS << "());\n"
                << "   return PyUnicode_FromString(val.c_str());\n";
      }
      pyFile << "}\n\n";

      XtString setS("set" + attrS.simpleType());
      if (typeS == "int")
      {
         pyFile << "static int\n" <<
                   "_" << tagS.simpleType() << "_" << setS << 
                   "(_" << tagS.simpleType() << 
                   " *self, PyObject *value, void *closure)\n"
                   "{\n"
                   "   long var = PyInt_AsLong(value);\n"
                   "   if (var == -1 && PyErr_Occurred()) {\n"
                   "      return -1;\n"
                   "   }\n"
                   "   self->elem->" << setS << "(var);\n"
                   "   return 0;\n"
                   "}\n\n";
         setters[attrS] = 1;
      }
      else if (typeS == "long")
      {
         pyFile << "static int\n" <<
                   "_" << tagS.simpleType() << "_" << setS << 
                   "(_" << tagS.simpleType() << 
                   " *self, PyObject *value, void *closure)\n"
                   "{\n"
                   "   long var = PyInt_AsLong(value);\n"
                   "   if (var == -1 && PyErr_Occurred()) {\n"
                   "      return -1;\n"
                   "   }\n"
                   "   self->elem->" << setS << "(var);\n"
                   "   return 0;\n"
                   "}\n\n";
         setters[attrS] = 1;
      }
      else if (typeS == "float")
      {
         pyFile << "static int\n" <<
                   "_" << tagS.simpleType() << "_" << setS << 
                   "(_" << tagS.simpleType() << 
                   " *self, PyObject *value, void *closure)\n"
                   "{\n"
                   "   double var = PyFloat_AsDouble(value);\n"
                   "   if (var == -1 && PyErr_Occurred()) {\n"
                   "      return 1;\n"
                   "   }\n"
                   "   self->elem->" << setS << "((float)var);\n"
                   "   return 0;\n"
                   "}\n\n";
         setters[attrS] = 1;
      }
      else if (typeS == "double")
      {
         pyFile << "static int\n" <<
                   "_" << tagS.simpleType() << "_" << setS << 
                   "(_" << tagS.simpleType() << 
                   " *self, PyObject *value, void *closure)\n"
                   "{\n"
                   "   double var = PyFloat_AsDouble(value);\n"
                   "   if (var == -1 && PyErr_Occurred()) {\n"
                   "      return -1;\n"
                   "   }\n"
                   "   self->elem->" << setS << "(var);\n"
                   "   return 0;\n"
                   "}\n\n";
         setters[attrS] = 1;
      }
      else if (typeS == "boolean")
      {
         pyFile << "static int\n" <<
                   "_" << tagS.simpleType() << "_" << setS << 
                   "(_" << tagS.simpleType() << 
                   " *self, PyObject *value, void *closure)\n"
                   "{\n"
                   "   long var = PyInt_AsLong(value);\n"
                   "   if (var == -1 && PyErr_Occurred()) {\n"
                   "      return -1;\n"
                   "   }\n"
                   "   self->elem->" << setS << "((var==0)? false : true);\n"
                   "   return 0;\n"
                   "}\n\n";
         setters[attrS] = 1;
      }
      else if (typeS == "string")
      {
         pyFile << "static int\n" <<
                   "_" << tagS.simpleType() << "_" << setS << 
                   "(_" << tagS.simpleType() << 
                   " *self, PyObject *value, void *closure)\n"
                   "{\n"
                   "   PyObject *str=0;\n"
                   "   if (PyUnicode_Check(value))\n"
                   "      str = PyUnicode_AsEncodedString(value, \"ASCII\", \"strict\");\n"
                   "   else\n"
                   "      str = value;\n"
                   "#if PY_MAJOR_VERSION < 3\n"
                   "   char *var = PyString_AsString(str);\n"
                   "#else\n"
                   "   char *var = PyBytes_AsString(str);\n"
                   "#endif\n"
                   "   if (var == 0) {\n"
                   "      return -1;\n"
                   "   }\n"
                   "   self->elem->" << setS << "(std::string(var));\n"
                   "   if (str != value) {\n"
                   "      Py_DECREF(str);\n"
                   "   }\n"
                   "   return 0;\n"
                   "}\n\n";
         setters[attrS] = 1;
      }
      else if (typeS == "anyURI")
      {
         pyFile << "static int\n" <<
                   "_" << tagS.simpleType() << "_" << setS << 
                   "(_" << tagS.simpleType() << 
                   " *self, PyObject *value, void *closure)\n"
                   "{\n"
                   "   PyObject *str=0;\n"
                   "   if (PyUnicode_Check(value))\n"
                   "      str = PyUnicode_AsEncodedString(value, \"ASCII\", \"strict\");\n"
                   "   else\n"
                   "      str = value;\n"
                   "#if PY_MAJOR_VERSION < 3\n"
                   "   char *var = PyString_AsString(str);\n"
                   "#else\n"
                   "   char *var = PyBytes_AsString(str);\n"
                   "#endif\n"
                   "   if (var == 0) {\n"
                   "      return -1;\n"
                   "   }\n"
                   "   self->elem->" << setS << "(std::string(var));\n"
                   "   if (str != value) {\n"
                   "      Py_DECREF(str);\n"
                   "   }\n"
                   "   return 0;\n"
                   "}\n\n";
         setters[attrS] = 1;
      }
      else if (typeS == "Particle_t")
      {
         pyFile << "static int\n" <<
                   "_" << tagS.simpleType() << "_" << setS << 
                   "(_" << tagS.simpleType() << 
                   " *self, PyObject *value, void *closure)\n"
                   "{\n"
                   "   long var = PyInt_AsLong(value);\n"
                   "   if (var == -1 && PyErr_Occurred()) {\n"
                   "      return -1;\n"
                   "   }\n"
                   "   self->elem->" << setS << "((Particle_t)var);\n"
                   "   return 0;\n"
                   "}\n\n";
         setters[attrS] = 1;
      }
   }

   std::map<XtString,method_descr> methods;

   if (tagS == "HDDM") {
      parentTable_t::iterator piter;
      for (piter = parents.begin(); piter != parents.end(); ++piter)
      {
         XtString cnameS(piter->first);
         if (cnameS != "HDDM" && element_in_list(cnameS,children[tagS]) == -1)
         {
            XtString getS("_" + tagS.simpleType() + "_get" 
                              + cnameS.plural().simpleType());
            pyFile << "static PyObject*\n" << getS <<
                      "(PyObject *self, PyObject *args)\n"
                      "{\n"
                      "   _" << tagS.simpleType() << 
                      " *me = (_" << tagS.simpleType() << "*)self;\n"
                      "   if (me->elem == 0) {\n"
                      "      PyErr_SetString(PyExc_RuntimeError, "
                      "\"lookup attempted on invalid " << tagS << 
                      " element\");\n"
                      "      return NULL;\n"
                      "   }\n"
                      "   PyObject *list = _HDDM_ElementList"
                      "_new(&_HDDM_ElementList_type, 0, 0);\n"
                      "   ((_HDDM_ElementList*)list)->subtype = "
                      "&_" << cnameS.simpleType() << "_type;\n"
                      "   ((_HDDM_ElementList*)list)->list = "
                      "(HDDM_ElementList<HDDM_Element>*)\n" << "    "
                      "new " << cnameS.listType() << "("
                      "me->elem->get" << cnameS.plural().simpleType() << "());\n"
                      "   ((_HDDM_ElementList*)list)->borrowed = 0;\n"
                      "   ((_HDDM_ElementList*)list)->host = me->host;\n"
                      "   My_INCREF(me->host);\n"
                      "   LOG_NEW(Py_TYPE(list), "
                      "((_HDDM_ElementList*)list)->subtype, 1);\n"
                      "   return list;\n"
                      "}\n\n"
                      ;
            method_descr meth_getS = {"get" + cnameS.plural().simpleType(), 
                                      "METH_NOARGS", 
                                      "get complete list of " + cnameS +
                                      " objects for this record"};
            methods[getS] = meth_getS;
         }
      }
   }

   parentList_t::iterator citer;
   for (citer = children[tagS].begin(); citer != children[tagS].end(); ++citer)
   {
      DOMElement *childEl = (DOMElement*)(*citer);
      XtString cnameS(childEl->getTagName());
      XtString repS(childEl->getAttribute(X("maxOccurs")));
      int rep = (repS == "unbounded")? INT_MAX : atoi(S(repS));
      XtString getS("_" + tagS.simpleType() + "_get" + cnameS.simpleType());
      pyFile << "static PyObject*\n" << getS <<
                "(PyObject *self, PyObject *args)\n"
                "{\n"
                "   int index=0;\n"
                "   if (! PyArg_ParseTuple(args, \"|i\", &index)) {\n"
                "      return NULL;\n"
                "   }\n"
                "   _" << tagS.simpleType() << 
                " *me = (_" << tagS.simpleType() << "*)self;\n"
                "   if (me->elem == 0) {\n"
                "      PyErr_SetString(PyExc_RuntimeError, "
                "\"lookup attempted on invalid " << tagS << 
                " element\");\n"
                "      return NULL;\n"
                "   }\n"
                "   PyObject *obj = _" << cnameS.simpleType() <<
                "_new(&_" << cnameS.simpleType() << "_type, 0, 0);\n"
                "   ((_" << cnameS.simpleType() << 
                "*)obj)->elem = &me->elem->get" << cnameS.simpleType() 
                << ((rep > 1)? "(index)" : "()") << ";\n"
                "   ((_" << cnameS.simpleType() << "*)obj)->host = me->host;\n"
                "   My_INCREF(me->host);\n"
                "   LOG_NEW(Py_TYPE(obj));\n"
                "   return obj;\n"
                "}\n\n"
                ;
      method_descr meth_getS = {"get" + cnameS.simpleType(),
                                "METH_VARARGS",
                                "get an individual " + cnameS + 
                                " object from this " + tagS};
      methods[getS] = meth_getS;

      XtString gelS("_" + tagS.simpleType()
                        + "_get" + cnameS.plural().simpleType());
      pyFile << "static PyObject*\n" << gelS <<
                "(PyObject *self, PyObject *args)\n"
                "{\n"
                "   _" << tagS.simpleType() << 
                " *me = (_" << tagS.simpleType() << "*)self;\n"
                "   if (me->elem == 0) {\n"
                "      PyErr_SetString(PyExc_RuntimeError, "
                "\"lookup attempted on invalid " << tagS << 
                " element\");\n"
                "      return NULL;\n"
                "   }\n"
                "   PyObject *list = _HDDM_ElementList_new"
                "(&_HDDM_ElementList_type, 0, 0);\n"
                "   ((_HDDM_ElementList*)list)->subtype =" 
                " &_" << cnameS.simpleType() << "_type;\n"
                "   ((_HDDM_ElementList*)list)->list = " 
                "(HDDM_ElementList<HDDM_Element>*)\n" << "    "
                "&me->elem->get" << cnameS.plural().simpleType() << "();\n"
                "   ((_HDDM_ElementList*)list)->borrowed = 1;\n"
                "   ((_HDDM_ElementList*)list)->host = me->host;\n"
                "   My_INCREF(me->host);\n"
                "   LOG_NEW(Py_TYPE(list), "
                "((_HDDM_ElementList*)list)->subtype, 0);\n" 
                "   return list;\n"
                "}\n\n"
                ;
      method_descr meth_gelS = {"get" + cnameS.plural().simpleType(),
                                "METH_NOARGS",
                                "get list of " + cnameS +
                                " objects for this " + tagS};
      methods[gelS] = meth_gelS;

      XtString addS("_" + tagS.simpleType() 
                        + "_add" + cnameS.plural().simpleType());
      pyFile << "static PyObject*\n" << addS <<
                "(PyObject *self, PyObject *args)\n"
                "{\n"
                "   int count=1;\n"
                "   int start=-1;\n"
                "   if (! PyArg_ParseTuple(args, \"|ii\", &count, &start)) {\n"
                "      return NULL;\n"
                "   }\n"
                "   _" << tagS.simpleType() << 
                " *me = (_" << tagS.simpleType() << "*)self;\n"
                "   if (me->elem == 0) {\n"
                "      PyErr_SetString(PyExc_RuntimeError, "
                "\"add attempted on invalid " << tagS << 
                " element\");\n"
                "      return NULL;\n"
                "   }\n"
                "   PyObject *list = _HDDM_ElementList_new"
                "(&_HDDM_ElementList_type, 0, 0);\n"
                "   ((_HDDM_ElementList*)list)->subtype =" 
                " &_" << cnameS.simpleType() << "_type;\n"
                "   ((_HDDM_ElementList*)list)->list = " 
                "(HDDM_ElementList<HDDM_Element>*)\n" << "    "
                "new " << cnameS.listType() << "("
                "me->elem->add" << cnameS.plural().simpleType() << 
                "(count, start));\n"
                "   ((_HDDM_ElementList*)list)->borrowed = 0;\n"
                "   ((_HDDM_ElementList*)list)->host = me->host;\n"
                "   My_INCREF(me->host);\n"
                "   LOG_NEW(Py_TYPE(list), "
                "((_HDDM_ElementList*)list)->subtype, 1);\n" 
                "   return list;\n"
                "}\n\n"
                ;
      method_descr meth_addS = {"add" + cnameS.plural().simpleType(),
                                "METH_VARARGS",
                                "extend (or insert into) the list of " + cnameS +
                                " objects for this " + tagS};
      methods[addS] = meth_addS;

      XtString delS("_" + tagS.simpleType() 
                        + "_delete" + cnameS.plural().simpleType());
      pyFile << "static PyObject*\n" << delS <<
                "(PyObject *self, PyObject *args)\n"
                "{\n"
                "   int count=-1;\n"
                "   int start=0;\n"
                "   if (! PyArg_ParseTuple(args, \"|ii\", &count, &start)) {\n"
                "      return NULL;\n"
                "   }\n"
                "   _" << tagS.simpleType() << 
                " *me = (_" << tagS.simpleType() << "*)self;\n"
                "   if (me->elem == 0) {\n"
                "      PyErr_SetString(PyExc_RuntimeError, "
                "\"delete attempted on invalid " << tagS << 
                " element\");\n"
                "      return NULL;\n"
                "   }\n"
                "   me->elem->delete" << cnameS.plural().simpleType() << 
                "(count, start);\n"
                "   Py_INCREF(Py_None);\n"
                "   return Py_None;\n"
                "}\n\n"
                ;
      method_descr meth_delS = {"delete" + cnameS.plural().simpleType(),
                                "METH_VARARGS",
                                "delete " + cnameS + " objects for this " + tagS};
      methods[delS] = meth_delS;
   }

   if (tagS == "HDDM")
   {
      XtString clrS("_" + tagS.simpleType() + "_clear");
      pyFile << "static PyObject*\n" << clrS <<
                "(PyObject *self, PyObject *args)\n"
                "{\n"
                "   _" << tagS.simpleType() <<
                " *me = (_" << tagS.simpleType() << "*)self;\n"
                "   if (me->elem == 0) {\n"
                "      PyErr_SetString(PyExc_RuntimeError, "
                "\"lookup attempted on invalid " << tagS << 
                " element\");\n"
                "      return NULL;\n"
                "   }\n"
                "   me->elem->clear();\n"
                "   Py_INCREF(Py_None);\n"
                "   return Py_None;\n"
                "}\n\n"
                ;
      method_descr meth_clrS = {"clear", "METH_NOARGS",
                                "clear all contents from this " + tagS};
      methods[clrS] = meth_clrS;
   }

   XtString strS("_" + tagS.simpleType() + "_toString");
   pyFile << "static PyObject*\n" << strS <<
             "(PyObject *self, PyObject *args=0)\n"
             "{\n"
             "   _" << tagS.simpleType() <<
             " *me = (_" << tagS.simpleType() << "*)self;\n"
             "   if (me->elem == 0) {\n"
             "      PyErr_SetString(PyExc_RuntimeError, "
             "\"lookup attempted on invalid " << tagS << 
             " element\");\n"
             "      return NULL;\n"
             "   }\n"
             "   std::string str(me->elem->toString());\n"
             "   return PyUnicode_FromString(str.c_str());\n"
             "}\n\n"
             ;
   method_descr str_method = {"toString", "METH_NOARGS",
                              "show element as a human-readable string"};
   methods[strS] = str_method;

   XtString xmlS("_" + tagS.simpleType() + "_toXML");
   pyFile << "static PyObject*\n" << xmlS <<
             "(PyObject *self, PyObject *args=0)\n"
             "{\n"
             "   _" << tagS.simpleType() <<
             " *me = (_" << tagS.simpleType() << "*)self;\n"
             "   if (me->elem == 0) {\n"
             "      PyErr_SetString(PyExc_RuntimeError, "
             "\"lookup attempted on invalid " << tagS << 
             " element\");\n"
             "      return NULL;\n"
             "   }\n"
             "   std::string str(me->elem->toXML());\n"
             "   return PyUnicode_FromString(str.c_str());\n"
             "}\n\n"
             ;
   method_descr xml_method = {"toXML", "METH_NOARGS",
                              "show element as a XML fragment"};
   methods[xmlS] = xml_method;

   pyFile << "static PyGetSetDef _" << tagS.simpleType() 
          << "_getsetters[] = {\n";
   std::map<XtString,XtString>::iterator aiter;
   for (aiter = attrList.begin(); aiter != attrList.end(); ++aiter) {
      XtString attrS = aiter->first;
      XtString getterS("_" + tagS.simpleType() + "_" + "get" + attrS.simpleType());
      XtString setterS("_" + tagS.simpleType() + "_" + "set" + attrS.simpleType());
      pyFile << "   {(char*)\"" << attrS << "\",\n"
             << "    (getter)" << getterS << ", ";
      if (setters.find(attrS) != setters.end()) {
         pyFile << "(setter)" << setterS << ",\n";
      }
      else {
         pyFile << "0,\n";
      }
      if (aiter->second == "string") {
         pyFile << "    (char*)\"" << attrS << " string\",\n";
      }
      else {
         pyFile << "    (char*)\"" << attrS << " value\",\n";
      }
      pyFile << "    NULL},\n";
   }
   pyFile << "   {NULL}  /* Sentinel */\n"
             "};\n\n";

   pyFile << "static PyMemberDef _" << tagS.simpleType() 
          << "_members[] = {\n"
          << "   {NULL}  /* Sentinel */\n"
          << "};\n\n";

   pyFile << "static PyMethodDef _" << tagS.simpleType()
          << "_methods[] = {\n";
   std::map<XtString,method_descr>::iterator miter;
   for (miter = methods.begin(); miter != methods.end(); ++miter) {
      pyFile << "   {\"" << miter->second.name << "\", "
             << miter->first << ", " << miter->second.args << ",\n"
             << "    \"" << miter->second.docs << "\"},\n";
   }

   // add special methods for read/write to hdf5 files
   if (tagS == "HDDM")
   {
      pyFile <<
      "#ifdef HDF5_SUPPORT\n"
      "   {\"hdf5FileWrite\", _HDDM_hdf5FileWrite, METH_VARARGS,\n"
      "    \"random-access write this hddm record to an output hdf5 file\"},\n"
      "   {\"hdf5FileRead\", _HDDM_hdf5FileRead, METH_VARARGS,\n"
      "    \"random-access read this hddm record from an input hdf5 file\"},\n"
      "#endif\n";
   }
   pyFile <<
   "   {NULL}  /* Sentinel */\n"
   "};\n\n";

   typesList[tagS] = "_" + tagS.simpleType() + "_type";

   pyFile <<
   "static PyTypeObject _" << tagS.simpleType() << "_type = {\n"
   "    PyVarObject_HEAD_INIT(NULL,0)\n"
   "    \"hddm_" << classPrefix << "." << tagS.simpleType() << "\","
   "         /*tp_name*/\n"
   "    sizeof(_" << tagS.simpleType() << 
   "),          /*tp_basicsize*/\n"
   "    0,                         /*tp_itemsize*/\n"
   "    (destructor)_" << tagS.simpleType() << 
   "_dealloc, /*tp_dealloc*/\n"
   "    0,                         /*tp_print*/\n"
   "    0,                         /*tp_getattr*/\n"
   "    0,                         /*tp_setattr*/\n"
   "    0,                         /*tp_compare*/\n"
   "    0,                         /*tp_repr*/\n"
   "    0,                         /*tp_as_number*/\n"
   "    0,                         /*tp_as_sequence*/\n"
   "    0,                         /*tp_as_mapping*/\n"
   "    0,                         /*tp_hash */\n"
   "    0,                         /*tp_call*/\n"
   "    (reprfunc)_" << tagS.simpleType() << "_toString,         /*tp_str*/\n"
   "    0,                         /*tp_getattro*/\n"
   "    0,                         /*tp_setattro*/\n"
   "    0,                         /*tp_as_buffer*/\n"
   "    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/\n"
   "    \"hddm_" << classPrefix << " " << tagS << 
   " element\",  /* tp_doc */\n"
   "    0,                         /* tp_traverse */\n"
   "    0,                         /* tp_clear */\n"
   "    0,                         /* tp_richcompare */\n"
   "    0,                         /* tp_weaklistoffset */\n"
   "    0,                         /* tp_iter */\n"
   "    0,                         /* tp_iternext */\n"
   "    _" << tagS.simpleType() << "_methods,          /* tp_methods */\n"
   "    _" << tagS.simpleType() << "_members,          /* tp_members */\n"
   "    _" << tagS.simpleType() << "_getsetters,       /* tp_getset */\n"
   "    &_HDDM_Element_type,       /* tp_base */\n"
   "    0,                         /* tp_dict */\n"
   "    0,                         /* tp_descr_get */\n"
   "    0,                         /* tp_descr_set */\n"
   "    0,                         /* tp_dictoffset */\n"
   "    (initproc)_" << tagS.simpleType() << "_init,   /* tp_init */\n"
   "    0,                         /* tp_alloc */\n"
   "    _" << tagS.simpleType() << "_new,              /* tp_new */\n"
   "};\n\n"
   ;
}

/* Generate class declarations for this tag and its descendants;
 * this function calls itself recursively
 */

void CodeBuilder::constructGroup(DOMElement* el)
{
   XtString tagS(el->getTagName());
   parentList_t::iterator piter;
   parents[tagS].insert(parents[tagS].begin(),
                        parentList.begin(),parentList.end());
   std::vector<DOMElement*>::iterator iter;
   for (iter = tagList.begin(); iter != tagList.end(); iter++)
   {
      DOMElement* targEl = *iter;
      XtString targS(targEl->getTagName());
      if (tagS == targS)
      {
         checkConsistency(el,targEl);
         return;
      }
   }

   parentList.push_back(el);
   DOMNodeList* contList = el->getChildNodes();
   size_t contLength = contList->getLength();
   for (int c = 0; c < contLength; c++)
   {
      DOMNode* cont = contList->item(c);
      short type = cont->getNodeType();
      if (type == DOMNode::ELEMENT_NODE)
      {
         DOMElement* contEl = (DOMElement*) cont;
         XtString contS(contEl->getTagName());
         children[tagS].push_back(contEl);
         constructGroup(contEl);
      }
   }
   parentList.pop_back();

   tagList.push_back(el);

   if (tagS == "HDDM")
   {
      std::vector<DOMElement*>::iterator iter;
      for (iter = tagList.begin(); iter != tagList.end(); iter++)
      {
         writeClassdef(*iter);
      }
   }
}

/* Write method implementation of the classes for this tag */

void CodeBuilder::writeClassimp(DOMElement* el)
{
}

/* Generate implementation code for data model classes */

void CodeBuilder::constructMethods(DOMElement* el)
{
   std::vector<DOMElement*>::iterator iter;
   for (iter = tagList.begin(); iter != tagList.end(); iter++)
   {
      writeClassimp(*iter);
   }
}

/* Generate methods for serializing classes to a stream and back again */

void CodeBuilder::writeStreamers(DOMElement* el)
{
}

void CodeBuilder::constructStreamers(DOMElement* el)
{
   std::vector<DOMElement*>::iterator iter;
   for (iter = tagList.begin(); iter != tagList.end(); ++iter)
   {
      writeStreamers(*iter);
   }
}
 
/* Generate methods to read from binary stream into classes */

void CodeBuilder::constructIOstreams(DOMElement* el)
{
}

/* Generate the xml template in normal form and store in a string */

void CodeBuilder::constructDocument(DOMElement* el)
{
   static int indent = 0;
   pyFile << "\"";
   for (int n = 0; n < indent; n++)
   {
      pyFile << "  ";
   }
   
   XtString tagS(el->getTagName());
   pyFile << "<" << tagS;
   DOMNamedNodeMap* attrList = el->getAttributes();
   size_t attrListLength = attrList->getLength();
   for (int a = 0; a < attrListLength; a++)
   {
      DOMNode* node = attrList->item(a);
      XtString nameS(node->getNodeName());
      XtString valueS(node->getNodeValue());
      pyFile << " " << nameS << "=\\\"" << valueS << "\\\"";
   }

   DOMNodeList* contList = el->getChildNodes();
   size_t contListLength = contList->getLength();
   if (contListLength > 0)
   {
      pyFile << ">\\n\"" << std::endl;
      indent++;
      for (int c = 0; c < contListLength; c++)
      {
         DOMNode* node = contList->item(c);
         if (node->getNodeType() == DOMNode::ELEMENT_NODE)
         {
            DOMElement* contEl = (DOMElement*) node;
            constructDocument(contEl);
         }
      }
      indent--;
      pyFile << "\"";
      for (int n = 0; n < indent; n++)
      {
         pyFile << "  ";
      }
      pyFile << "</" << tagS << ">\\n\"" << std::endl;
   }
   else
   {
      pyFile << " />\\n\"" << std::endl;
   }
}

std::string guessType(const std::string &literal)
{
   const char *str = literal.c_str();
   char *endptr;
   errno=0;
   long long int llvalue = strtoll(str,&endptr,0);
   if (errno == 0 && *endptr == 0) {
      errno=0;
      int lvalue = strtol(str,&endptr,0);
      if (errno == 0 && *endptr == 0 && lvalue == llvalue) {
         return "int";
      }
      else {
         return "long";
      }
   }
   errno=0;
   strtof(str,&endptr);
   if (errno == 0 && *endptr == 0) {
      return "float";
   }
   errno=0;
   strtod(str,&endptr);
   if (errno == 0 && *endptr == 0) {
      return "double";
   }
   if (literal == "true" || literal == "false") {
      return "boolean";
   }
   if ((int)lookupParticle(literal) != 0) {
      return "Particle_t";
   }
   if (XMLUri::isValidURI(false,X(literal))) {
      return "anyURI";
   }
   return "string";
}

Particle_t lookupParticle(const std::string &name)
{
   for (int p=0; p<100; ++p) {
      if (ParticleType((Particle_t)p) == name) {
         return (Particle_t)p;
      }
   }
   return UnknownParticle;
}
