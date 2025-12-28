RDNA
====

Vulkan driver for AMD RDNA GPUs.

- *CP* : Command Processor
- *SDMA*: System DMA
  Transfers data CPU<->GPU or GPU<->GPU for textures.
  Can swizzle into optimal layouts (Morton order)
- *PM4*:
- *SI_DMA*: Old (Southern Islands / GCN 1.0) DMA engine. Not supported.
- *PM4*: Packet Microcode 4: command language used by CP
- *PKT3*: Packet type (within PM4) for most rings.
- *GFX1030*: RDNA 2
- *VCN*: Video Core Next (replacer for old UVD and VCE)
- *MEC*: MicroEngine Compute
- *ACE*: Async Compute Engine

DRM (Direct Rendering Manager):

- *BO*: Buffer Object (allocation)
- *HW_IP*: each different 'chip' basically (HW_IP_GFX, HW_IP_SDMA).
- *IB*: Indirect Buffer: Allocated on BO, can be submitted to a HW_IP.
- *CS*: Command Stream

.. toctree::
    :maxdepth: 1
    :caption: Links

    GPUOpen PAL Drivers <https://github.com/GPUOpen-Drivers/pal>
    Mesa3d drivers <https://docs.mesa3d.org/drivers/radv.html>
