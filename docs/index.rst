Kestrel Project
================

Kestrel is a modern GPGPU & Rendering API in similar vein to `Vulkan`_,
however simlifying the API though focusing on modern hardware. It is
designed for performance and developer flexibility. Instead of piling
on feature flags, many of which are present on >99% of GPUs, we
determine a minimal spec required. The API is built around device addressing.

.. _Vulkan: https://www.vulkan.org/

We believe a big hardware paradigm-shift has been going on, while the
traditional graphics APIs have been stuck in legacy-support mode; both
harming driver performance and development speed. Integrated GPUs have
UMA, and modern discrete GPUs support Resizable BAR. Buffers don't have
types, and you usually prefer to share them. Vulkan for example usually
forces the developer to write barriers between different resources.
This is not how modern GPUs work, and leads to the driver inneficiencies.

This project has taken the good parts from Mesa3D, modernized them
into a sleek User-Mode Driver (:term:`UMD`), and develops a new API.

Minimal spec hardware:

- Nvidia Turing (RTX 2000 series, 2018)
- AMD RDNA2 (RX 6000 series, 2020)
- Intel Alchemist / Xe1 (2022)
- Apple M1 / A14 (MacBook M1, iPhone 12, 2020)
- ARM Mali-G710 (2021)
- Qualcomm Adreno 650 (2019)
- PowerVR DXT (Pixel 10, 2025)

.. toctree::
   :hidden:

   motivation
   usage/getting_started
   usage/examples
   license
   glossary

.. toctree::
   :hidden:

   api/index
   spec/index

.. toctree::
   :hidden:

   drivers/index


:ref:`genindex`
