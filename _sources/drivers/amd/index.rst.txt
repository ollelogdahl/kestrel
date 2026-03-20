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


AMD (& afaik NV) support predication in multiple ways. In AMD, we can both skip over commands in the CP, but also
many packets can read from the predication register. Skipping commands is done for arbitrary va atomic ops (i believe),
and the register is for stuff like DRAW_VISIBLE. I believe it would be awesome to support an api like::

    auto x = kes_malloc(dev, size, 4, KesMemoryDefault);
    auto pred = kes_malloc(dev, 1, 4, KesMemoryDefault);

    auto l1 = kes_start_recording(compute);

    kes_cmd_conditional_begin(l1, pred, KesCondOpEqual);
    kes_cmd_memset(l1, x.gpu, size, 2);
    kes_cmd_memcpy(l1, y.gpu, x.gpu, size);
    kes_cmd_conditional_end(l1);

    kes_submit(compute, l1);

This would be awesome, as it would actually expose this stuff without weird extensions like VK_conditional_rendering.

.. toctree::
    bugs

.. toctree::
    :maxdepth: 1
    :caption: Links

    GPUOpen PAL Drivers <https://github.com/GPUOpen-Drivers/pal>
    Mesa3d drivers <https://docs.mesa3d.org/drivers/radv.html>
