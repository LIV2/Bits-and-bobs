# BootSelectWB

This kickstartmodule allows for you to have multiple Workbench partitions that will be selected based on the current Kickstart version

It requires that you name the partitions to match the Kickstart version

|Kickstart|Partition name|
|---------|--------------|
|1.3|WB_1.3|
|2.x|WB_2_X|
|3.0|WB_3.0|
|3.1|WB_3.1|
|3.14|WB_3.14|
|3.2|WB_3.2|

For example, imagine you have two partitions: `WB_1.3` and `WB_3.1`  
If you boot from Kickstart 3.1, the `WB_1.3` partition will be made non-bootable and if you boot from Kickstart 1.3 `WB_3.1` will be made unbootable

# Installation

This module must be added to a custom kickstart image

# Acknowledgements
Thank you to [luvwagn](https://github.com/luvwagn) for extending this to support more kickstart versions.
