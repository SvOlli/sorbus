CC65 Support
============

While the Sorbus Computer is relying on the ca65 assembler as a build tool, the
support of something written in C for cc65 has not been taken care of so far.

In this directory are a few concepts of how support could be implemented. The
big issue will be how to tell CMake how to do all this.

- copy a reference library (cc65 is suggesting supervision.lib) for expansion
- expand the library with hardware related functions (e.g. read/write)
- provide one or more linker config files

While the best way to support the Sorbus Computer within cc65 will be
contributing to the project, this will take quite some time. First of all to
come up with an implementation, continues with the changes to be integrated
into a new release version, and ends with the Linux distribution to pick up
that release for their stable distros.

So the development will be started here for fast integration, and then ported
over to the cc65 repo. Once it hits Debian stable, this preliminary
implementation here will be straightend out.

