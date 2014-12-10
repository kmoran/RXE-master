RXE
===

OFED Soft RoCE Providers - Software Implementation of RDMA over Converged
Ethernet

compat-rdma-3.12
================

OFED compat-rdma directory tree with the soft RoCE kernel provider "rxe"
folded into the build environment.  Prior to the first build you will need
to execute the configure script to set the prefix, kernel-version, kernel-
sources, modules-dir and the kernel modules to be built.  It is expected
that OFED-3.12 without RXE has been installed.

librxe-1.0.0
============

OFED librxe user provider directory tree for soft RoCE user-space provider.
