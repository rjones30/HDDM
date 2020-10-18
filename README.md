# HDDM
## Hierarchical Document Data Model

HDDM is a tool for automatic building of a full-featured C++ library for representation of highly structured scientific data in memory, complete with a performant i/o library for integrated storage and retrieval of unlimited amounts of repetitive data with associated metadata. Starting from a plain text file where the user describes that data values and relationships to be expressed, the HDDM tools automatically generate C++ header and source files that define the C++ classes for building the model in memory, storing it in a standard format in disk files for retrieval later, and efficient browsing/manipulation of the data in memory using familiar object-oriented semantics. All of the following features that a user expects for a data modeling and i/o library are supported by HDDM.

- uses standard c++11 language features, compiles with g++ 4.8.5 -std-c++11
- python support through automatic generation of a C++ extension module
- stl list container iteration semantics in C++ for repeated data
- standard python list iteration semantics for repeated data
- efficient handling of sparse lists and tables
- configurable on-the-fly compression / decompression during i/o
- configurable on-the-fly data integrity validation during i/o
- browsable data representation on disk, choice between HDF5 or native formats
- standard byte-ordered formats of int, IEEE float on disk
- automatic detection and conversion between standard and native formats
- multi-threaded, multi-buffered i/o for high throughput with compression

In addition to the above, this package combines the following features in a unique way.

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
