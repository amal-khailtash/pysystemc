"""Python port of Accellera risc_cpu instruction decoder (Apache-2.0).

Preserves source instruction timing, register writeback and placeholder opcodes.
See upstream LICENSE/NOTICE. Each simulation requires a fresh interpreter.
"""

from pathlib import Path
from amal.eda.systemc import sc_core as sc
from exec import signed32


class Decode(sc.sc_module):
    """Own architectural registers and the native clocked decoder ports."""

    def __init__(self, name):
        """Load register.img and retain the decoder callback until shutdown."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.resetin = sc.sc_in["bool"]("resetin")
        self.instruction = sc.sc_in["unsigned"]("instruction")
        self.pred_instruction = sc.sc_in["unsigned"]("pred_instruction")
        self.instruction_valid = sc.sc_in["bool"]("instruction_valid")
        self.pred_inst_valid = sc.sc_in["bool"]("pred_inst_valid")
        self.destreg_write = sc.sc_in["bool"]("destreg_write")
        self.destreg_write_src = sc.sc_in["unsigned"]("destreg_write_src")
        self.alu_dataout = sc.sc_in["int"]("alu_dataout")
        self.dram_dataout = sc.sc_in["int"]("dram_dataout")
        self.dram_rd_valid = sc.sc_in["bool"]("dram_rd_valid")
        self.dram_write_src = sc.sc_in["unsigned"]("dram_write_src")
        self.fpu_dout = sc.sc_in["int"]("fpu_dout")
        self.fpu_valid = sc.sc_in["bool"]("fpu_valid")
        self.fpu_destout = sc.sc_in["unsigned"]("fpu_destout")
        self.clear_branch = sc.sc_in["bool"]("clear_branch")
        self.display_done = sc.sc_in["bool"]("display_done")
        self.pc = sc.sc_in["unsigned"]("pc")
        self.pred_on = sc.sc_in["bool"]("pred_on")
        self.br_instruction_address = sc.sc_out["unsigned"]("br_instruction_address")
        self.next_pc = sc.sc_out["bool"]("next_pc")
        self.branch_valid = sc.sc_out["bool"]("branch_valid")
        self.branch_target_address = sc.sc_out["unsigned"]("branch_target_address")
        self.mem_access = sc.sc_out["bool"]("mem_access")
        self.mem_address = sc.sc_out["unsigned"]("mem_address")
        self.alu_op = sc.sc_out["int"]("alu_op")
        self.mem_write = sc.sc_out["bool"]("mem_write")
        self.alu_src = sc.sc_out["unsigned"]("alu_src")
        self.reg_write = sc.sc_out["bool"]("reg_write")
        self.src_A = sc.sc_out["int"]("src_A")
        self.src_B = sc.sc_out["int"]("src_B")
        self.forward_A = sc.sc_out["bool"]("forward_A")
        self.forward_B = sc.sc_out["bool"]("forward_B")
        self.stall_fetch = sc.sc_out["bool"]("stall_fetch")
        self.decode_valid = sc.sc_out["bool"]("decode_valid")
        self.float_valid = sc.sc_out["bool"]("float_valid")
        self.mmx_valid = sc.sc_out["bool"]("mmx_valid")
        self.pid_valid = sc.sc_out["bool"]("pid_valid")
        self.pid_data = sc.sc_out["int"]("pid_data")
        self.CLK = sc.sc_in[bool]("CLK")
        self.cpu_reg = [signed32(int(v, 16)) for v in Path("register.img").read_text().split()]
        self.cpu_reg += [0] * (32 - len(self.cpu_reg))
        self.pc_reg = self.jalpc_reg = 0
        self.vcpu_reg = [0] * 32
        self.cpu_reg_lock = [False] * 32
        print("** ALERT ** ID: initialize Architectural Registers")
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def run(self):
        """Decode all upstream opcode cases with source cycle boundaries."""
        for valid in (self.branch_valid, self.decode_valid, self.float_valid, self.mmx_valid):
            valid.write(False)
        sc.wait(2)
        while True:
            for valid, dest, data, label in (
                (self.destreg_write, self.destreg_write_src, self.alu_dataout, "ALU"),
                (self.dram_rd_valid, self.dram_write_src, self.dram_dataout, "MemLd"),
                (self.fpu_valid, self.fpu_destout, self.fpu_dout, "MMX"),
            ):
                if valid.read():
                    index, value = dest.read(), data.read()
                    self.cpu_reg[index] = value
                    detail = "" if label == "MMX" else f"({value})"
                    print("\t\t\t-------------------------------")
                    print(
                        f"\t\t\tID: R{index}=0x{value & 0xFFFFFFFF:x}{detail} fr {label} at CSIM {sc.sc_time_stamp()}"
                    )
                    print("\t\t\t-------------------------------")
            if self.instruction_valid.read():
                self.pc_reg = self.pc.read()
                if self.clear_branch.read():
                    print(f"\t\t\tID: clear branch at CSIM {sc.sc_time_stamp()}")
                    self.branch_valid.write(False)
                instr_tmp = self.instruction.read()
                opcode_tmp = instr_tmp >> 24
                regC_tmp, regA_tmp, regB_tmp = (instr_tmp >> 20) & 15, (instr_tmp >> 16) & 15, (instr_tmp >> 12) & 15
                imm_tmp = instr_tmp & 65535
                offset_tmp = instr_tmp & 4095
                label_tmp = imm_tmp if imm_tmp < 32768 else imm_tmp - 65536
                longlabel_tmp = instr_tmp & 0xFFFFFF
                srcA_tmp, srcB_tmp, srcC_tmp = self.cpu_reg[regA_tmp], self.cpu_reg[regB_tmp], self.cpu_reg[regC_tmp]
                sc.wait()
                print("\t\t\t-------------------------------")
                if opcode_tmp == 0x0:
                    print(
                        f"\n\n\t\t\t*******************************\n\t\t\tID: REGISTERS DUMP at CSIM {sc.sc_time_stamp()}"
                    )
                    print("\t\t\t*******************************")
                    print("REG :==================================================================")
                    for row in range(0, 32, 4):
                        print("".join(f"  R{i:2d}({self.cpu_reg[i] & 0xFFFFFFFF:08x})  " for i in range(row, row + 4)))
                    print("=======================================================================\n")
                    sc.wait()
                    sc.wait()
                elif opcode_tmp == 0x01:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}= R{regA_tmp:d}(={srcA_tmp:d})+R{regB_tmp:d}(={srcB_tmp:d})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x02:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}= R{regA_tmp:d}(={srcA_tmp:d})+{imm_tmp:d}",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(imm_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x03:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=  R{regA_tmp:d}(={srcA_tmp:d})+R{regB_tmp:d}(={srcB_tmp:d})+C",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(1)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x04:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp:d})-R{regB_tmp:d}(={srcB_tmp:d})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(4)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x05:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp:d})-{imm_tmp:d}",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(imm_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(4)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x06:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp:d})-R{regB_tmp:d}(={srcB_tmp:d})-C",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(2)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x07:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp:d})*R{regB_tmp:d}(={srcB_tmp:d})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(5)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x08:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp:d})/R{regB_tmp:d}(={srcB_tmp:d})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(6)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x09:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp & 4294967295:x}) nand R{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(7)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x0A:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp & 4294967295:x}) and R{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(8)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x0B:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp & 4294967295:x}) or R{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(9)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x0C:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp & 4294967295:x}) xor R{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(10)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x0D:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}= NOT R{regA_tmp:d}(={srcA_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(0)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(11)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x0E:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}= R{regA_tmp:d}(={srcA_tmp & 4294967295:x}) mod R{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(14)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x4D:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}<=mem[R{regA_tmp:d}=({srcA_tmp:d})+{offset_tmp:d}]",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.mem_access.write(True)
                    self.mem_write.write(False)
                    self.alu_src.write(regC_tmp)
                    offset_tmp = (offset_tmp + srcA_tmp) & 0xFFFFFFFF
                    self.mem_address.write(offset_tmp)
                    sc.wait()
                    self.mem_access.write(False)
                    sc.wait()
                elif opcode_tmp == 0x4E:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=>mem[R{regA_tmp:d}(={srcA_tmp:d}) + {offset_tmp:d}]",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.mem_access.write(True)
                    self.mem_write.write(True)
                    offset_tmp = (offset_tmp + srcA_tmp) & 0xFFFFFFFF
                    self.mem_address.write(offset_tmp)
                    sc.wait()
                    self.mem_access.write(False)
                    self.mem_write.write(False)
                    sc.wait()
                elif opcode_tmp == 0x0F:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}=R{regA_tmp:d}(={srcA_tmp:d})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(0)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x10:
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    if srcC_tmp == srcA_tmp:
                        self.branch_target_address.write((self.pc_reg + label_tmp) & 0xFFFFFFFF)
                        self.br_instruction_address.write(instr_tmp)
                        self.branch_valid.write(True)
                        print(
                            f"\t\t\tID: beq R{regC_tmp:d}(={srcC_tmp:d}), R{regA_tmp:d}(={srcA_tmp:d}), pc+=({label_tmp:d}).\n",
                            end="",
                        )
                    else:
                        print(
                            f"\t\t\tID: beq R{regC_tmp:d}(={srcC_tmp:d}) != R{regA_tmp:d}(={srcA_tmp:d}),pc++.\n",
                            end="",
                        )
                    print("\t\t\tID: at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    self.branch_target_address.write(self.pc_reg + 1)
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x11:
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    if srcC_tmp != srcA_tmp:
                        self.branch_target_address.write((self.pc_reg + label_tmp) & 0xFFFFFFFF)
                        self.br_instruction_address.write(instr_tmp)
                        self.branch_valid.write(True)
                        print(
                            f"\t\t\tID: bne R{regC_tmp:d}(={srcC_tmp:d}), R{regA_tmp:d}(={srcA_tmp:d}), pc+=({label_tmp:d}).\n",
                            end="",
                        )
                    else:
                        print(
                            f"\t\t\tID: bne R{regC_tmp:d}(={srcC_tmp:d}) = R{regA_tmp:d}(={srcA_tmp:d}),pc++.\n",
                            end="",
                        )
                    print("\t\t\tID: at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    self.branch_target_address.write(self.pc_reg + 1)
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x12:
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    if srcC_tmp > srcA_tmp:
                        self.branch_target_address.write((self.pc_reg + label_tmp) & 0xFFFFFFFF)
                        self.br_instruction_address.write(instr_tmp)
                        self.branch_valid.write(True)
                        print(
                            f"\t\t\tID: bgt R{regC_tmp:d}(={srcC_tmp:d})>R{regA_tmp:d}(={srcA_tmp:d}), pc+=({label_tmp:d}).\n",
                            end="",
                        )
                    else:
                        print(
                            f"\t\t\tID: bgt R{regC_tmp:d}(={srcC_tmp:d}) <= R{regA_tmp:d}(={srcA_tmp:d}),pc++.\n",
                            end="",
                        )
                    print("\t\t\tID: at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    self.branch_target_address.write(self.pc_reg + 1)
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x13:
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    if srcC_tmp >= srcA_tmp:
                        self.branch_target_address.write((self.pc_reg + label_tmp) & 0xFFFFFFFF)
                        self.br_instruction_address.write(instr_tmp)
                        self.branch_valid.write(True)
                        print(
                            f"\t\t\tID: bge R{regC_tmp:d}(={srcC_tmp:d})>=R{regA_tmp:d}(={srcA_tmp:d}), pc+=({label_tmp:d}).\n",
                            end="",
                        )
                    else:
                        print(
                            f"\t\t\tID: bge R{regC_tmp:d}(={srcC_tmp:d}) < R{regA_tmp:d}(={srcA_tmp:d}),pc++.\n",
                            end="",
                        )
                    print("\t\t\tID: at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    self.branch_target_address.write(self.pc_reg + 1)
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x14:
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    if srcC_tmp < srcA_tmp:
                        self.branch_target_address.write((self.pc_reg + label_tmp) & 0xFFFFFFFF)
                        self.br_instruction_address.write(instr_tmp)
                        self.branch_valid.write(True)
                        print(
                            f"\t\t\tID: blt R{regC_tmp:d}(={srcC_tmp:d})<R{regA_tmp:d}(={srcA_tmp:d}), pc+=({label_tmp:d}).\n",
                            end="",
                        )
                    else:
                        print(
                            f"\t\t\tID: blt R{regC_tmp:d}(={srcC_tmp:d}) >= R{regA_tmp:d}(={srcA_tmp:d}), pc++.\n",
                            end="",
                        )
                    print("\t\t\tID: at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    self.branch_target_address.write(self.pc_reg + 1)
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x15:
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    if srcC_tmp <= srcA_tmp:
                        self.branch_target_address.write((self.pc_reg + label_tmp) & 0xFFFFFFFF)
                        self.br_instruction_address.write(instr_tmp)
                        self.branch_valid.write(True)
                        print(
                            f"\t\t\tID: ble R{regC_tmp:d}(={srcC_tmp:d})<=R{regA_tmp:d}(={srcA_tmp:d}), pc+=({label_tmp:d}).\n",
                            end="",
                        )
                    else:
                        print(
                            f"\t\t\tID: ble R{regC_tmp:d}(={srcC_tmp:d})>R{regA_tmp:d}(={srcA_tmp:d}), pc++.\n",
                            end="",
                        )
                    print("\t\t\tID: at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    self.branch_target_address.write(self.pc_reg + 1)
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x16:
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    self.branch_target_address.write(longlabel_tmp)
                    self.br_instruction_address.write(instr_tmp)
                    self.branch_valid.write(True)
                    print(f"\t\t\tID: pc jump to => ({longlabel_tmp:d}).", end="")
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    self.branch_target_address.write(self.pc_reg + 1)
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x17:
                    print(f"\t\t\tID: j pc({longlabel_tmp:d}) and sp <- pc", end="")
                    print(" at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    sc.wait()
                elif opcode_tmp == 0x18:
                    print("\t\t\tID: jr and pc <- sp\n", end="")
                    print(" at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    sc.wait()
                    sc.wait()
                elif opcode_tmp == 0x29:
                    print(
                        f"\t\t\tID: FR{regC_tmp:d}=FR{regA_tmp:d}(={srcA_tmp & 4294967295:x})+FR{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(3)
                    self.float_valid.write(True)
                    sc.wait()
                    self.float_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x2A:
                    print(
                        f"\t\t\tID: FR{regC_tmp:d}=FR{regA_tmp:d}(={srcA_tmp & 4294967295:x})-FR{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(4)
                    self.float_valid.write(True)
                    sc.wait()
                    self.float_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x2B:
                    print(
                        f"\t\t\tID: FR{regC_tmp:d}=FR{regA_tmp:d}(={srcA_tmp & 4294967295:x})*FR{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(5)
                    self.float_valid.write(True)
                    sc.wait()
                    self.float_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x2C:
                    print(
                        f"\t\t\tID: FR{regC_tmp:d}=FR{regA_tmp:d}(={srcA_tmp & 4294967295:x})/FR{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(6)
                    self.float_valid.write(True)
                    sc.wait()
                    self.float_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x31:
                    print(
                        f"\t\t\tID: MR{regC_tmp:d}=MR{regA_tmp:d}(={srcA_tmp & 4294967295:x})+MR{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(3)
                    self.mmx_valid.write(True)
                    sc.wait()
                    self.mmx_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x32:
                    print(
                        f"\t\t\tID: MR{regC_tmp:d}=MR{regA_tmp:d}(={srcA_tmp & 4294967295:x})+MR{regB_tmp:d}(={srcB_tmp & 4294967295:x}) Sat",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(4)
                    self.mmx_valid.write(True)
                    sc.wait()
                    self.mmx_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x33:
                    print(
                        f"\t\t\tID: MR{regC_tmp:d}=MR{regA_tmp:d}(={srcA_tmp & 4294967295:x})-MR{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(5)
                    self.mmx_valid.write(True)
                    sc.wait()
                    self.mmx_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x34:
                    print(
                        f"\t\t\tID: MR{regC_tmp:d}=MR{regA_tmp:d}(={srcA_tmp & 4294967295:x})-MR{regB_tmp:d}(={srcB_tmp & 4294967295:x}) Sat",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(6)
                    self.mmx_valid.write(True)
                    sc.wait()
                    self.mmx_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x35:
                    print(
                        f"\t\t\tID: MR{regC_tmp:d}=MR{regA_tmp:d}(={srcA_tmp & 4294967295:x}) ,MR{regB_tmp:d}(={srcB_tmp & 4294967295:x}) PMADD",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(7)
                    self.mmx_valid.write(True)
                    sc.wait()
                    self.mmx_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x36:
                    print(
                        f"\t\t\tID: MR{regC_tmp:d}=MR{regA_tmp:d}(={srcA_tmp & 4294967295:x}) pack MR{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(8)
                    self.mmx_valid.write(True)
                    sc.wait()
                    self.mmx_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0x37:
                    print(
                        f"\t\t\tID: MR{regC_tmp:d}=MR{regA_tmp:d}(={srcA_tmp & 4294967295:x}) mmxck MR{regB_tmp:d}(={srcB_tmp & 4294967295:x})",
                        end="",
                    )
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(srcA_tmp)
                    self.src_B.write(srcB_tmp)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(9)
                    self.mmx_valid.write(True)
                    sc.wait()
                    self.mmx_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0xE0:
                    print("\t\t\tID: flush all registers", end="")
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(0)
                    self.cpu_reg = [0] * 32
                    sc.wait()
                    sc.wait()
                elif opcode_tmp == 0xF0:
                    print(f"\t\t\tID: ld pid ={longlabel_tmp:d}", end="")
                    print(" \n\t\t\t  : at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(0)
                    self.src_B.write(0)
                    self.alu_src.write(0)
                    self.alu_op.write(0)
                    self.pid_valid.write(True)
                    self.decode_valid.write(False)
                    self.float_valid.write(False)
                    self.mmx_valid.write(False)
                    self.pid_data.write(longlabel_tmp)
                    sc.wait()
                    self.pid_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0xF1:
                    print(
                        f"\t\t\tID: R{regC_tmp:d}={imm_tmp:d}",
                        end="",
                    )
                    print(" at CSIM " + str(sc.sc_time_stamp()))
                    print("\t\t\t-------------------------------")
                    self.src_A.write(imm_tmp)
                    self.src_B.write(0)
                    self.alu_src.write(regC_tmp)
                    self.alu_op.write(3)
                    self.decode_valid.write(True)
                    sc.wait()
                    self.decode_valid.write(False)
                    sc.wait()
                elif opcode_tmp == 0xFF:
                    print("\t\t\tID: - SHUTDOWN - ", end="")
                    print("at CSIM " + str(sc.sc_time_stamp()))
                    self.decode_valid.write(False)
                    self.float_valid.write(False)
                    self.mmx_valid.write(False)
                    sc.wait()
                    print("\t\t\tID: - PLEASE WAIT ...... - \n", end="")
                    print("\t\t\t-------------------------------")
                    sc.sc_stop()
                    print(
                        "\n\n\n////////////////////////////////////////////////////////////////////////////////\n",
                        end="",
                    )
                    sc.wait()
                    sc.wait()
                else:
                    print(f"\t\t\tID: INVALID OPCODE \n\t\t\t  : at CSIM {sc.sc_time_stamp()}")
                    sc.wait()
            self.next_pc.write(True)
            sc.wait()
