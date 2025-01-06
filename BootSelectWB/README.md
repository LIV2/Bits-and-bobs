# BootSelectWB

This module allows for you to have two Workbench partitions that will be selected based on the current Kickstart version

It requires that you create two partitions, `WB_1.3` and `WB_2.X` just like the Amiga 3000 SuperKickstart

When booting from Kickstart 1.3, `WB_2.X` will be marked as non-bootable, when booting from Kickstart 2.0 or higher, `WB_1.3` will be marked as non bootable

# Installation

This module must be added to a custom kickstart image