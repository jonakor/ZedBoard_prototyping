#*****************************************************************************************
# Check if script is running in correct Vivado version.
#*****************************************************************************************
set scripts_vivado_version 2019.1
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   catch {common::send_msg_id "BD_TCL-109" "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."}

   return 1
}

set projectDir $/home/hypso/Documents/vivado

#Setting the name of the project
set projectName ZedBoard_proto

#*****************************************************************************************
# Creating the project and specifying the part (xc7z020clg484-1)
#*****************************************************************************************
#create_project $projectName $projectDir/$projectName -part xc7z020clg484-1 -force
create_project ZedBoard_proto /home/hypso/Documents/vivado/ZedBoard_proto -part xc7z020clg484-1 -force


#*****************************************************************************************
# Using the ZedBoard preset
#*****************************************************************************************
set_property board_part em.avnet.com:zed:part0:1.4 [current_project]
set_property target_language VHDL [current_project]

#*****************************************************************************************
# Create Block Design
#*****************************************************************************************
create_bd_design "ZedBoard_proto"
update_compile_order -fileset sources_1

#*****************************************************************************************
# Add IPs
#*****************************************************************************************

#Zynq
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 processing_system7_0
endgroup

#axi_timer
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 axi_timer_0
endgroup

#Run block automation
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR" apply_board_preset "1" Master "Disable" Slave "Disable" }  [get_bd_cells processing_system7_0]

#Run connection automation
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0} Slave {/axi_timer_0/S_AXI} intc_ip {New AXI Interconnect} master_apm {0}}  [get_bd_intf_pins axi_timer_0/S_AXI]

#Make block diagram nice
regenerate_bd_layout

#Save Block Diagram
save_bd_design
update_compile_order -fileset sources_1

#*****************************************************************************************
# Generate Hardware files
#*****************************************************************************************

#Create HDL Wrapper
make_wrapper -files [get_files /home/hypso/Documents/vivado/Zedboard_proto/Zedboard_proto.srcs/sources_1/bd/ZedBoard_proto/ZedBoard_proto.bd] -top
add_files -norecurse /home/hypso/Documents/vivado/ZedBoard_proto/ZedBoard_proto.srcs/sources_1/bd/ZedBoard_proto/hdl/ZedBoard_proto_wrapper.vhd

#Run Synthesis
launch_runs synth_1 -jobs 8

#Run Implementation
#launch_runs impl_1 -jobs 8

#Generate Bitstream
launch_runs impl_1 -to_step write_bitstream -jobs 8

#Export Hardware to local
file mkdir /home/hypso/Documents/vivado/ZedBoard_proto/ZedBoard_proto.sdk
file copy -force /home/hypso/Documents/vivado/ZedBoard_proto/ZedBoard_proto.runs/impl_1/ZedBoard_proto_wrapper.sysdef /home/hypso/Documents/vivado/ZedBoard_proto/ZedBoard_proto.sdk/ZedBoard_proto_wrapper.hdf

#*****************************************************************************************
# Launch SDK
#*****************************************************************************************
launch_sdk -workspace /home/hypso/Documents/vivado/ZedBoard_proto/ZedBoard_proto.sdk -hwspec /home/hypso/Documents/vivado/ZedBoard_proto/ZedBoard_proto.sdk/ZedBoard_proto_wrapper.hdf
