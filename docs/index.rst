Introduction
=====================

Kestrel is a modern GPGPU & Rendering API in similar vein to `Vulkan`_,
however simlifying the API though focusing on modern hardware. It is
designed for performance and developer flexibility. Instead of piling
on feature flags, many of which are present on >99% of GPUs, we
determine a minimal spec required. The API is built around device addressing
and Resizable BAR.

.. _Vulkan: https://www.vulkan.org/

Minimal spec hardware:

- Nvidia Turing (RTX 2000 series, 2018)
- AMD RDNA2 (RX 6000 series, 2020)
- Intel Alchemist / Xe1 (2022)
- Apple M1 / A14 (MacBook M1, iPhone 12, 2020)
- ARM Mali-G710 (2021)
- Qualcomm Adreno 650 (2019)
- PowerVR DXT (Pixel 10, 2025)

.. toctree::
   :maxdepth: 1
   :caption: Documentation
   :hidden:

   self
   license

.. toctree::
   :maxdepth: 1
   :caption: Developer Topics
   :hidden:

   amd/index
