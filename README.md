# HDDM - efficient i/o library for self-describing structured scientific data
## Hierarchical Document Data Model

HDDM is a tool for automatic building of a full-featured C++ library for representation of highly structured scientific data in memory, complete with a performant i/o library for integrated storage and retrieval of unlimited amounts of repetitive data with associated metadata. Starting from a structured document written in plain text, where the user describes the data values and relationships to be expressed, the HDDM tools automatically generate custom C++ header and source files that define new user classes for building an object-oriented representation of the data in memory, storing them in a standard format in disk files for retrieval later, and efficient means for browsing/manipulation of the data using familiar OO semantics in the user's C++ or python analysis application. All of the following features that a user expects fromd a big-data modeling and i/o library are supported by HDDM.

- uses standard c++11 language features, compiles with g++ 4.8.5 -std-c++11
- python support through automatic generation of custom C++ extension modules
- stl list container iteration semantics in C++ for repeated data
- standard python list iteration semantics for repeated data
- efficient handling of sparse lists and tables through nested variable-length lists
- configurable on-the-fly compression / decompression during i/o
- configurable on-the-fly data integrity validation during i/o
- browsable data representation on disk, choice at run-time between **HDF5** and native stream formats
- platform-independence through standard byte-ordered formats of int, IEEE float in streams and on disk
- automatic detection and conversion between standard and native formats
- multi-threaded, multi-buffered i/o for high throughput with compression

In addition to meeting the above requirements, this package combines the following features in a unique way.

- automatic user library code generated using the user's own terminology for class and member names
- user's data model document can be validated against xml schema using standard open-source tools
- user's data model is compactly represented in a plain-text template that looks like real data
- HDDM streams are highly compact prior to compresson, smaller than equivalent block-tables in an RBD
- thousands of lines of efficient C++ code generated froma few dozen lines of xml written by the user
- only the actual binary data values are stored in the stream, the structure and fixed metadata are saved in the header
- general tools provided for rendering the contents of hddm data streams in plain-text xml

## Applications
HDDM was designed in response to the needs of particle physics experiments producing petabytes of data per year, but nothing in the design is specific to that application. HDDM is of general utility for any application with large datasets consisting of highly structured data. In contrast to simple data, like photographic images consisting of regular arrays of floats or color vectors, structured data consist of heterogeneous values of various types (variable-length strings, variable-length lists of ntuples, lists of variable-length lists...) that are related to one another through a hierarchical graph. Data from advanced scientific instruments typically contain repeated blocks of a basic pattern of such relationships, with variations in the number of nodes connecting to each point in the graph from one block to the next.

An xml document provides a flexible means to represent such a hierarchical graph, where the xml tags represent the data and their nesting reflects the relationships in the graph. At the top of the graph is the largest repeating pattern in the dataset, also called a record. At the bottom are the individual values representing the measured data in terms of integers and floats, together with their units. In between are the intermediate nodes of the graph that represent the ways the different values come together to form a single record from the instrument. Once this graph has been written down in the form of a structured xml document, the HDDM tools read the xml and automatically generates a custom set of C++ / python classes. The user's application can then include / import these classes and use them to read the raw data from the instrument into C++ objects for subsequent storage on disk in a standard format, and for final analysis.

## Documentation
The documentation for HDDM consists of three parts: a description of the data modeling language used by the xml record template and the associated schema, a description of the user application interface in C++ and python that gives access to the generated data and i/o classes provided by the library, and instructions on how to use the tools through the examples provided with the package as a guide to users writing their own custom applications. All three of these have now been combined into The HDDM User's Guide. Instructions for building the HDDM tools from sources are found in the INSTALL file distributed with the sources.

## Dependencies
HDDM relies on the following external open-source packages. Some must be installed on the user platform before HDDM can be built, and others are optional.
- gcc/g++ compiler version 4.8.5 or above : compiler must support -std=c++11 standard language features
- python 2.7 or above : standard python installation, including shutil, distutils modules and dependencies
- apache xerces-c version 3 : standard implementation of the xerces xml library in C++, required
- apache xalan-j version 2 : standard tools for schema-based xml validation and translation, optional
- HDF5 version 1.12+ : public-domain library for standard disk representation of structured data, optional
- libbz2 and libz : compression libraries available as system devel packages for most Linux distributions
- xmllint : commandline utility in the standard libxml2-utils distribution
- cmake version 3.10 : required for building the libraries from sources
- git : required for installing hddm from github
Uncountable other dependencies exist for other features of a standard unix/linux platform environment, such as the ld link loader, standard glibc and system libraries.

## Streaming readers
Extensions are available to the core i/o functionality of the generated HDDM libraries for reading from streaming data sources using HTTP/s and XRootD protocols. The most immediate application in view is the capability to read from large hddm data files hosted on a remote server without first having to download the entire file and then read the data from local storage. If the hddm model library is built with streaming input support then substitution of a httpIstream object constructed as
- httpIstream hstream("https://my.server.org/mydatafile.hddm");

in the place of a ifstream("mydatafile.hddm"), or similarly an xrootdIstream object constructed as 
- xrootdIstream rstream("root://my.server.org/mydatafile.hddm");

is all that is required to access the streaming input capability through the C++ api. Using the python module, simply supply a url string in the place of the input filename provided to the istream constructor. Building your hddm library with streaming support requires that you check out the streaming\_input branch of HDDM instead of main. The build instructions for the streaming\_input branch are the same as main, with the following additional dependencies.
- gcc/g++ compiler version 8 or above: compiler must support -std=c++17, needed to build libcpr
- python 3.9 or above: needed to link against libraries built with c++17
- libcpr: download from https://github.com/rjones30/cpr.git and install with the usual cmake procedure
- openssl: install from standard distribution if not already present as a system package
- special cmake build options: -Dcpr\_DIR={my.path/cpr} and -DCPR\_INCLUDE\_DIR={my.path/cpr/include}
- xrootd: needed to build streaming over xrootd, only tested with xrootd version 4

## Acknowledgements
HDDM contains as a part of its source codebase a sub-package named xstream, which is a fork of an earlier open-source package that was released as xstream 2.1 by its author Claudio Valente in 1999 under the GNU LESSER GENERAL PUBLIC LICENSE. The original author and license is included unchanged under xstream/AUTHOR and xstream/COPYING. The original README written by Claudio Valente is also included. The HDDM fork of xstream 2.1 was made in 2004 in order to correct some bugs in the original v2.1 code and to add new features related to stream repositioning and multi-threaded compression/decompression. These changes made the HDDM fork of xstream no longer backward-compatible with xstream 2.1. With open acknowledgement of the important contribution of xstream 2.1 by Claudio Valente to this project, the release here of the modified xstream code under an Apache open-source license is deemed consistent with the terms of the original LGPL license that accompanied Valente's release of xstream 2.1. The original C++ xstream 2.1 package released in 1999 is apparently unrelated to a number of other currently active open-source projects named xstream, including the java project XStream by Joe Walnes et al, the javascript project xstream by Andre Staltz, among others.

The author acknowledges support from the United States National Science Foundation that has enabled the development of this package within the context of the University of Connecticut nuclear physics research group, where the author serves as a professor.

## Contact
HDDM is released as a public github project under an Apache Open-Source license by its designer and developer, Richard Jones, richard.t.jones(at)uconn.edu. On-going development of HDDM and user support is provided by the author to the GlueX Collaboration as a part of his contribution to the GlueX Experiment at Jefferson Lab in Newport News, Virginia. Support for other users of HDDM will be provided by the author on an as-able basis.
