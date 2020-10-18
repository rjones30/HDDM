# HDDM
## Hierarchical Document Data Model

HDDM is a tool for automatic building of a full-featured C++ library for the representation of highly structured scientific data in memory, complete with a performant i/o library for integrated storage and retrieval of unlimited amounts of repetitive data with associated metadata. Starting from a plain text file where the user describes that data values and relationships to be expressed, the HDDM tools automatically generate C++ header and source files that define the C++ classes for building the model in memory, storing it in a standard format in disk files for retrieval later, and efficient browsing/manipulation of the data in memory using familiar object-oriented semantics. All of the following expected features for a data modeling library are supported by the generated library.

- stl list iteration semantics for repeated data
- standard c++11 language elements, compiles with g++ 4.8.5
- python extension module wraps generated user C++ classes for python
- support for both python version 2 and version 3 modules
- efficient handling of sparse lists and tables
- on-the-fly compression / decompression during i/o
- built-in data integrity validation during i/o
- multi-threaded design for high throughput
- metadata coupled together with data, eg. a quantity and its units
- standard data representation on disk in HDF5 or native formats

In addition to the above, this package combines the following features in a unique way.

- automatic user library code generated in the user's own data language
- user's data model document can be validated against xml schema
- user's data model is compactly represented in a template that looks like a real data file
- HDDM streams are highly compactified, smaller than equivalent block-tables in an RBD
- thousands of lines of efficient C++ code generated from data model description (a few dozen lines of xml)
- general tools provided for displaying the contents of hddm records in plain-text xml

## Application Domains
HDDM was designed in response to the needs of particle physics experiments producing petabytes of data per year, but the result is of general utility for any application with large datasets consisting of highly structured data. In contrast to simple data like photographic images consisting of regular arrays of floats or color vectors, structured data consist of a large set of heterogeneous values of various types (variable-length strings, URL links, n-tuples) that are related to one another through a hierarchical graph. Data from advanced scientific instruments typically contain repeated blocks of a common pattern of relationships, with variations in the number of nodes connecting to each point in the graph from one block to the next. An xml document provides a flexible means to represent such a hierarchical graph, where the xml tags represent the data and their nesting reflects the relationships in the graph. 
