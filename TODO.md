# Shader Compiler
Infinite number of issues and to be fixed.

# Shader API
[] Before we actually support spir-v or something similar, we
   need a way for applications to specify shader code. I like the
   GIR api and I believe it would be epic if we could expose something
   akin to that to the user; this simplifies things like JIT-GPU code,
   I cannot see why you wouldn't allow this really.

# Semaphores
I think there are two levels of semaphores inside mesa; both internal driver
semaphores and also user-provided ones. I think none of them work, we want both
to work :^)

# Crashes & Hangs
[] I manage to hang my machine easily by writing incorrect shaders. I have not 
   experienced this before in Vulkan; i believe the driver detects a hang and
   restarts the device. However, we are doing something wrong, so my machine
   fails to restart.
