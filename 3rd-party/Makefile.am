#
# Copyright (c) 2023-2024 Amazon.com, Inc. or its affiliates. All rights reserved.
#
# See LICENSE.txt for license information
#

EXTRA_DIST = \
	boost \
	efa-dp-direct \
	nccl

if ENABLE_GDAKI
# Build the pinned efa-dp-direct host implementation as a GDAKI-only,
# internal library. It is not installed because it is a private dependency of
# the OFI plugin, not a consumer-facing API.
noinst_LTLIBRARIES = libefa_cuda_dp.la

libefa_cuda_dp_la_SOURCES = \
	efa-dp-direct/CUDA/host/efa_cuda_dp.cpp

libefa_cuda_dp_la_CPPFLAGS = \
	$(CUDA_CPPFLAGS) \
	-I$(abs_top_srcdir)/3rd-party/efa-dp-direct/CUDA/common \
	-I$(abs_top_srcdir)/3rd-party/efa-dp-direct/CUDA/host

# The pinned host source compares its `int` loop index with a `size_t` length.
# Scope this temporary warning-as-error exception to third-party code so
# plugin-owned sources remain subject to the project warning policy.
libefa_cuda_dp_la_CXXFLAGS = \
	-Wno-error=sign-compare
endif
