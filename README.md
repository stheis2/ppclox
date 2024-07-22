# ppclox

C++ re-implementation of toy language Lox (from the book "Crafting Interpreters" by Robert Nystrom: https://github.com/munificent/craftinginterpreters). Follows the approach of "clox", the C bytecode intepreter.

Currently missing final chapter 30 on Optimization, though I may include that eventually.

## Why does this exist

I though it would be interesting to port the C interpreter to C++ since it seemed it could benefit from C++ idioms and the C++ standard library.

NOTE! I'm *not* a C++ developer in my day job (I typically work in other languages like C# and TypeScript). I'm sure there are many ways this code could be improved, though I don't plan to spend further time on it at this time. I did this as an exercise to try to expand my knowledge.

## Notable features/changes from clox:

Utilizes C++ standard library types and C++ idioms in place of hand-rolled C wherever it made sense. Including but not limited to:

* std:vector in place of C dynamic arrays or linked lists
* std::unordered_map in place of the hand-written C hash table
* std::string_view and std::string in place of char* arrays where possible

Restructured string interning to use C++ idioms (see object_string.hpp/cpp):
     
* ObjStrings own a std::string private member, and ObjStrings are de-duped via std::unordered_map and custom key class InternedStringKey.
* ObjStrings themselves can be "used as keys" in std::unordered_map via custom key class ObjStringRef which compares ObjStrings by pointer comparison and uses cached hash value
* De-duping and ObjString allocation are protected via a recursive_mutex in anticipation of supporting multi-threading. I don't plan to actually expand the entire implementation to support multi-threading, but I thought it would be interesting to consider how this part of the implementation might handle that.

Other notable items:

* VM value stack (and call stack) utilizes std::vector. Instead of pointers into the stack, indexes are used. This allows the stack to grow beyond its initial capacity if needed.
* Garbage collection is limited to objects of type Obj via overloaded new and delete (see object.hpp/object.cpp). Memory allocated by the compiler/VM for other uses (e.g. by C++ STL containers) is not involved in the VM's garbage collection and so reduces the surface area for GC bugs (though they still happened!).

## Non-goals

* Performance at least as good as Clox. This may be acheivable but it wasn't my goal. I have a full time job and so have limited free time. Since no one will actually be using this implementation for real world coding, there is limited ROI in me spending time on this. There's plenty of other things to work on and learn!
* High quality comments. In a real code base, high quality comments are important, but for a toy language implemented as an exercise, they are not so important.

## Building and running

* Code should be portable to any platform with a modern C++ compiler supporting C++23 but I've only setup builds for Visual Studio 2022 on Windows
* Can open ppclox.sln in Visual Studio 2022 and run it vie the IDE, OR open Visual Studio 2022 Developer command prompt and run "run.ps1" script via powershell: `powershell ./run`
* Currently set up to run test_file.lox script. Remove from run.ps1 or ppclox.vcxproj.user file to run the REPL.



