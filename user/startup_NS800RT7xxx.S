/****************************************************************************
 * @file     startup_NS800RT7xxx.s
 * @author   MCD Application Team
 * @brief    CMSIS-Core(M) Device Startup File for
 *           Device NOVOSENSE
 ******************************************************************************/
/*
 * Copyright (c) 2025 Novosense Limited.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

                .syntax  unified
                .arch    armv7e-m

                .section .vectors
                .align   2
                .globl   __Vectors
                .globl   __Vectors_End
                .globl   __Vectors_Size
__Vectors:
                /* Internal Interrupts */
                .long    __StackTop                         /* Top of Stack */
                .long    Reset_Handler                      /* -15 Reset Handler */
                .long    NMI_Handler                        /* -14 NMI Handler */
                .long    HardFault_Handler                  /* -13 Hard Fault Handler */
                .long    MemManage_Handler                  /* -12 MPU Fault Handler */
                .long    BusFault_Handler                   /* -11 Bus Fault Handler */
                .long    UsageFault_Handler                 /* -10 Usage Fault Handler */
                .long    0                                  /* -9  Reserved */
                .long    0                                  /* -8  Reserved */
                .long    0                                  /* -7  Reserved */
                .long    0                                  /* -6  Reserved */
                .long    SVC_Handler                        /* -5  SVCall Handler */
                .long    DebugMon_Handler                   /* -4  Debug Monitor Handler */
                .long    0                                  /* -3  Reserved */
                .long    PendSV_Handler                     /* -2  PendSV Handler */
                .long    SysTick_Handler                    /* -1  SysTick Handler */


                /* External Interrupts */
                .long    WWDG_IRQHandler                    /* 0   WWDG_IRQn */
                .long    IWDG1_IRQHandler                   /* 1   IWDG1_IRQn */
                .long    IWDG2_IRQHandler                   /* 2   IWDG2_IRQn */
                .long    LVD_IRQHandler                     /* 3   LVD_IRQn */
                .long    FLASH_IRQHandler                   /* 4   FLASH_IRQn */
                .long    FLASH_ECCERR_IRQHandler            /* 5   FLASH_ECCERR_IRQn */
                .long    RAM_ECCERR_IRQHandler              /* 6   RAM_ECCERR_IRQn */
                .long    RCC_IRQHandler                     /* 7   RCC_IRQn */
                .long    EXTI3_0_IRQHandler                 /* 8   EXTI3_0_IRQn */
                .long    EXTI7_4_IRQHandler                 /* 9   EXTI7_4_IRQn */
                .long    EXTI11_8_IRQHandler                /* 10  EXTI11_8_IRQn */
                .long    EXTI15_12_IRQHandler               /* 11  EXTI15_12_IRQn */
                .long    EXTI_CMP_IRQHandler                /* 12  EXTI_CMP_IRQn */
                .long    EXTI_XINT2_1_IRQHandler            /* 13  EXTI_XINT2_1_IRQn */
                .long    EXTI_XINT5_3_IRQHandler            /* 14  EXTI_XINT5_3_IRQn */
                .long    EDMA1_CH0_IRQHandler               /* 15  EDMA1_CH0_IRQn */
                .long    EDMA1_CH1_IRQHandler               /* 16  EDMA1_CH1_IRQn */
                .long    EDMA1_CH2_IRQHandler               /* 17  EDMA1_CH2_IRQn */
                .long    EDMA1_CH3_IRQHandler               /* 18  EDMA1_CH3_IRQn */
                .long    EDMA1_CH4_IRQHandler               /* 19  EDMA1_CH4_IRQn */
                .long    EDMA1_CH5_IRQHandler               /* 20  EDMA1_CH5_IRQn */
                .long    EDMA1_CH6_IRQHandler               /* 21  EDMA1_CH6_IRQn */
                .long    EDMA1_CH7_IRQHandler               /* 22  EDMA1_CH7_IRQn */
                .long    EDMA1_CH8_IRQHandler               /* 23  EDMA1_CH8_IRQn */
                .long    EDMA1_CH9_IRQHandler               /* 24  EDMA1_CH9_IRQn */
                .long    EDMA1_CH10_IRQHandler              /* 25  EDMA1_CH10_IRQn */
                .long    EDMA1_CH11_IRQHandler              /* 26  EDMA1_CH11_IRQn */
                .long    EDMA1_CH12_IRQHandler              /* 27  EDMA1_CH12_IRQn */
                .long    EDMA1_CH13_IRQHandler              /* 28  EDMA1_CH13_IRQn */
                .long    EDMA1_CH14_IRQHandler              /* 29  EDMA1_CH14_IRQn */
                .long    EDMA1_CH15_IRQHandler              /* 30  EDMA1_CH15_IRQn */
                .long    EDMA1_CH16_IRQHandler              /* 31  EDMA1_CH16_IRQn */
                .long    EDMA1_CH17_IRQHandler              /* 32  EDMA1_CH17_IRQn */
                .long    EDMA1_CH18_IRQHandler              /* 33  EDMA1_CH18_IRQn */
                .long    EDMA1_CH19_IRQHandler              /* 34  EDMA1_CH19_IRQn */
                .long    EDMA1_CH20_IRQHandler              /* 35  EDMA1_CH20_IRQn */
                .long    EDMA1_CH21_IRQHandler              /* 36  EDMA1_CH21_IRQn */
                .long    EDMA1_CH22_IRQHandler              /* 37  EDMA1_CH22_IRQn */
                .long    EDMA1_CH23_IRQHandler              /* 38  EDMA1_CH23_IRQn */
                .long    EDMA1_CH24_IRQHandler              /* 39  EDMA1_CH24_IRQn */
                .long    EDMA1_CH25_IRQHandler              /* 40  EDMA1_CH25_IRQn */
                .long    EDMA1_CH26_IRQHandler              /* 41  EDMA1_CH26_IRQn */
                .long    EDMA1_CH27_IRQHandler              /* 42  EDMA1_CH27_IRQn */
                .long    EDMA1_CH28_IRQHandler              /* 43  EDMA1_CH28_IRQn */
                .long    EDMA1_CH29_IRQHandler              /* 44  EDMA1_CH29_IRQn */
                .long    EDMA1_CH30_IRQHandler              /* 45  EDMA1_CH30_IRQn */
                .long    EDMA1_CH31_IRQHandler              /* 46  EDMA1_CH31_IRQn */
                .long    EDMA1_ERR_IRQHandler               /* 47  EDMA1_ERR_IRQn */
                .long    EDMA2_CH0_IRQHandler               /* 48  EDMA2_CH0_IRQn */
                .long    EDMA2_CH1_IRQHandler               /* 49  EDMA2_CH1_IRQn */
                .long    EDMA2_CH2_IRQHandler               /* 50  EDMA2_CH2_IRQn */
                .long    EDMA2_CH3_IRQHandler               /* 51  EDMA2_CH3_IRQn */
                .long    EDMA2_CH4_IRQHandler               /* 52  EDMA2_CH4_IRQn */
                .long    EDMA2_CH5_IRQHandler               /* 53  EDMA2_CH5_IRQn */
                .long    EDMA2_CH6_IRQHandler               /* 54  EDMA2_CH6_IRQn */
                .long    EDMA2_CH7_IRQHandler               /* 55  EDMA2_CH7_IRQn */
                .long    EDMA2_CH8_IRQHandler               /* 56  EDMA2_CH8_IRQn */
                .long    EDMA2_CH9_IRQHandler               /* 57  EDMA2_CH9_IRQn */
                .long    EDMA2_CH10_IRQHandler              /* 58  EDMA2_CH10_IRQn */
                .long    EDMA2_CH11_IRQHandler              /* 59  EDMA2_CH11_IRQn */
                .long    EDMA2_CH12_IRQHandler              /* 60  EDMA2_CH12_IRQn */
                .long    EDMA2_CH13_IRQHandler              /* 61  EDMA2_CH13_IRQn */
                .long    EDMA2_CH14_IRQHandler              /* 62  EDMA2_CH14_IRQn */
                .long    EDMA2_CH15_IRQHandler              /* 63  EDMA2_CH15_IRQn */
                .long    EDMA2_CH16_IRQHandler              /* 64  EDMA2_CH16_IRQn */
                .long    EDMA2_CH17_IRQHandler              /* 65  EDMA2_CH17_IRQn */
                .long    EDMA2_CH18_IRQHandler              /* 66  EDMA2_CH18_IRQn */
                .long    EDMA2_CH19_IRQHandler              /* 67  EDMA2_CH19_IRQn */
                .long    EDMA2_CH20_IRQHandler              /* 68  EDMA2_CH20_IRQn */
                .long    EDMA2_CH21_IRQHandler              /* 69  EDMA2_CH21_IRQn */
                .long    EDMA2_CH22_IRQHandler              /* 70  EDMA2_CH22_IRQn */
                .long    EDMA2_CH23_IRQHandler              /* 71  EDMA2_CH23_IRQn */
                .long    EDMA2_CH24_IRQHandler              /* 72  EDMA2_CH24_IRQn */
                .long    EDMA2_CH25_IRQHandler              /* 73  EDMA2_CH25_IRQn */
                .long    EDMA2_CH26_IRQHandler              /* 74  EDMA2_CH26_IRQn */
                .long    EDMA2_CH27_IRQHandler              /* 75  EDMA2_CH27_IRQn */
                .long    EDMA2_CH28_IRQHandler              /* 76  EDMA2_CH28_IRQn */
                .long    EDMA2_CH29_IRQHandler              /* 77  EDMA2_CH29_IRQn */
                .long    EDMA2_CH30_IRQHandler              /* 78  EDMA2_CH30_IRQn */
                .long    EDMA2_CH31_IRQHandler              /* 79  EDMA2_CH31_IRQn */
                .long    EDMA2_ERR_IRQHandler               /* 80  EDMA2_ERR_IRQn */
                .long    ADCA_CONV_IRQHandler               /* 81  ADCA_CONV_IRQn */
                .long    ADCA_EVENT_IRQHandler              /* 82  ADCA_EVENT_IRQn */
                .long    ADCB_CONV_IRQHandler               /* 83  ADCB_CONV_IRQn */
                .long    ADCB_EVENT_IRQHandler              /* 84  ADCB_EVENT_IRQn */
                .long    ADCC_CONV_IRQHandler               /* 85  ADCC_CONV_IRQn */
                .long    ADCC_EVENT_IRQHandler              /* 86  ADCC_EVENT_IRQn */
                .long    0                                  /* 87  Reserved */
                .long    0                                  /* 87  Reserved */
                .long    ADC_SAFE_CHK_IRQHandler            /* 89  ADC_SAFE_CHK_IRQn */
                .long    EPWM1_TZINT_IRQHandler             /* 90  EPWM1_TZINT_IRQn */
                .long    EPWM2_TZINT_IRQHandler             /* 91  EPWM2_TZINT_IRQn */
                .long    EPWM3_TZINT_IRQHandler             /* 92  EPWM3_TZINT_IRQn */
                .long    EPWM4_TZINT_IRQHandler             /* 93  EPWM4_TZINT_IRQn */
                .long    EPWM5_TZINT_IRQHandler             /* 94  EPWM5_TZINT_IRQn */
                .long    EPWM6_TZINT_IRQHandler             /* 95  EPWM6_TZINT_IRQn */
                .long    EPWM7_TZINT_IRQHandler             /* 96  EPWM7_TZINT_IRQn */
                .long    EPWM8_TZINT_IRQHandler             /* 97  EPWM8_TZINT_IRQn */
                .long    EPWM9_TZINT_IRQHandler             /* 98  EPWM9_TZINT_IRQn */
                .long    EPWM10_TZINT_IRQHandler            /* 99  EPWM10_TZINT_IRQn */
                .long    EPWM11_TZINT_IRQHandler            /* 100 EPWM11_TZINT_IRQn */
                .long    EPWM12_TZINT_IRQHandler            /* 101 EPWM12_TZINT_IRQn */
                .long    EPWM13_TZINT_IRQHandler            /* 102 EPWM13_TZINT_IRQn */
                .long    EPWM14_TZINT_IRQHandler            /* 103 EPWM14_TZINT_IRQn */
                .long    EPWM15_TZINT_IRQHandler            /* 104 EPWM15_TZINT_IRQn */
                .long    EPWM16_TZINT_IRQHandler            /* 105 EPWM16_TZINT_IRQn */
                .long    EPWM17_TZINT_IRQHandler            /* 106 EPWM17_TZINT_IRQn */
                .long    EPWM18_TZINT_IRQHandler            /* 107 EPWM18_TZINT_IRQn */
                .long    EPWM1_INT_IRQHandler               /* 108 EPWM1_INT_IRQn */
                .long    EPWM2_INT_IRQHandler               /* 109 EPWM2_INT_IRQn */
                .long    EPWM3_INT_IRQHandler               /* 110 EPWM3_INT_IRQn */
                .long    EPWM4_INT_IRQHandler               /* 111 EPWM4_INT_IRQn */
                .long    EPWM5_INT_IRQHandler               /* 112 EPWM5_INT_IRQn */
                .long    EPWM6_INT_IRQHandler               /* 113 EPWM6_INT_IRQn */
                .long    EPWM7_INT_IRQHandler               /* 114 EPWM7_INT_IRQn */
                .long    EPWM8_INT_IRQHandler               /* 115 EPWM8_INT_IRQn */
                .long    EPWM9_INT_IRQHandler               /* 116 EPWM9_INT_IRQn */
                .long    EPWM10_INT_IRQHandler              /* 117 EPWM10_INT_IRQn */
                .long    EPWM11_INT_IRQHandler              /* 118 EPWM11_INT_IRQn */
                .long    EPWM12_INT_IRQHandler              /* 119 EPWM12_INT_IRQn */
                .long    EPWM13_INT_IRQHandler              /* 120 EPWM13_INT_IRQn */
                .long    EPWM14_INT_IRQHandler              /* 121 EPWM14_INT_IRQn */
                .long    EPWM15_INT_IRQHandler              /* 122 EPWM15_INT_IRQn */
                .long    EPWM16_INT_IRQHandler              /* 123 EPWM16_INT_IRQn */
                .long    EPWM17_INT_IRQHandler              /* 124 EPWM17_INT_IRQn */
                .long    EPWM18_INT_IRQHandler              /* 125 EPWM18_INT_IRQn */
                .long    ECAP1_IRQHandler                   /* 126 ECAP1_IRQn */
                .long    ECAP2_IRQHandler                   /* 127 ECAP2_IRQn */
                .long    ECAP3_IRQHandler                   /* 128 ECAP3_IRQn */
                .long    ECAP4_IRQHandler                   /* 129 ECAP4_IRQn */
                .long    ECAP5_IRQHandler                   /* 130 ECAP5_IRQn */
                .long    ECAP6_IRQHandler                   /* 131 ECAP6_IRQn */
                .long    ECAP7_IRQHandler                   /* 132 ECAP7_IRQn */
                .long    EQEP1_IRQHandler                   /* 133 EQEP1_IRQn */
                .long    EQEP2_IRQHandler                   /* 134 EQEP2_IRQn */
                .long    EQEP3_IRQHandler                   /* 135 EQEP3_IRQn */
                .long    EQEP4_IRQHandler                   /* 136 EQEP4_IRQn */
                .long    EQEP5_IRQHandler                   /* 137 EQEP5_IRQn */
                .long    EQEP6_IRQHandler                   /* 138 EQEP6_IRQn */
                .long    EPG_IRQHandler                     /* 139 EPG_IRQn */
                .long    SDFM1_DR_1_IRQHandler              /* 140 SDFM1_DR_1_IRQn */
                .long    SDFM1_DR_2_IRQHandler              /* 141 SDFM1_DR_2_IRQn */
                .long    SDFM1_DR_3_IRQHandler              /* 142 SDFM1_DR_3_IRQn */
                .long    SDFM1_DR_4_IRQHandler              /* 143 SDFM1_DR_4_IRQn */
                .long    SDFM1_ERR_IRQHandler               /* 144 SDFM1_ERR_IRQn */
                .long    SDFM2_DR_1_IRQHandler              /* 145 SDFM2_DR_1_IRQn */
                .long    SDFM2_DR_2_IRQHandler              /* 146 SDFM2_DR_2_IRQn */
                .long    SDFM2_DR_3_IRQHandler              /* 147 SDFM2_DR_3_IRQn */
                .long    SDFM2_DR_4_IRQHandler              /* 148 SDFM2_DR_4_IRQn */
                .long    SDFM2_ERR_IRQHandler               /* 149 SDFM2_ERR_IRQn */
                .long    SDFM3_DR_1_IRQHandler              /* 150 SDFM3_DR_1_IRQn */
                .long    SDFM3_DR_2_IRQHandler              /* 151 SDFM3_DR_2_IRQn */
                .long    SDFM3_DR_3_IRQHandler              /* 152 SDFM3_DR_3_IRQn */
                .long    SDFM3_DR_4_IRQHandler              /* 153 SDFM3_DR_4_IRQn */
                .long    SDFM3_ERR_IRQHandler               /* 154 SDFM3_ERR_IRQn */
                .long    SDFM4_DR_1_IRQHandler              /* 155 SDFM4_DR_1_IRQn */
                .long    SDFM4_DR_2_IRQHandler              /* 156 SDFM4_DR_2_IRQn */
                .long    SDFM4_DR_3_IRQHandler              /* 157 SDFM4_DR_3_IRQn */
                .long    SDFM4_DR_4_IRQHandler              /* 158 SDFM4_DR_4_IRQn */
                .long    SDFM4_ERR_IRQHandler               /* 159 SDFM4_ERR_IRQn */
                .long    LPTIM_IRQHandler                   /* 160 LPTIM_IRQn */
                .long    CAN1_1_IRQHandler                  /* 161 CAN1_1_IRQn */
                .long    CAN1_2_IRQHandler                  /* 162 CAN1_2_IRQn */
                .long    CAN1_ECC_IRQHandler                /* 163 CAN1_ECC_IRQn */
                .long    CAN1_WKUP_IRQHandler               /* 164 CAN1_WKUP_IRQn */
                .long    CANFD1_1_IRQHandler                /* 165 CANFD1_1_IRQn */
                .long    CANFD1_2_IRQHandler                /* 166 CANFD1_2_IRQn */
                .long    CANFD1_ECC_IRQHandler              /* 167 CANFD1_ECC_IRQn */
                .long    CANFD1_WKUP_TS_IRQHandler          /* 168 CANFD1_WKUP_TS_IRQn */
                .long    CANFD2_1_IRQHandler                /* 169 CANFD2_1_IRQn */
                .long    CANFD2_2_IRQHandler                /* 170 CANFD2_2_IRQn */
                .long    CANFD2_ECC_IRQHandler              /* 171 CANFD2_ECC_IRQn */
                .long    CANFD2_WKUP_TS_IRQHandler          /* 172 CANFD2_WKUP_TS_IRQn */
                .long    BTIM1_IRQHandler                   /* 173 BTIM1_IRQn */
                .long    BTIM2_IRQHandler                   /* 174 BTIM2_IRQn */
                .long    STIM1_IRQHandler                   /* 175 STIM1_IRQn */
                .long    STIM2_IRQHandler                   /* 176 STIM2_IRQn */
                .long    STIM3_IRQHandler                   /* 177 STIM3_IRQn */
                .long    TIM1_IRQHandler                    /* 178 TIM1_IRQn */
                .long    TIM2_IRQHandler                    /* 179 TIM2_IRQn */
                .long    AWK_IRQHandler                     /* 180 AWK_IRQn */
                .long    LIN1_IRQHandler                    /* 181 LIN1_IRQn */
                .long    LIN2_IRQHandler                    /* 182 LIN2_IRQn */
                .long    I2C1_MST_IRQHandler                /* 183 I2C1_MST_IRQn */
                .long    I2C1_SLV_IRQHandler                /* 184 I2C1_SLV_IRQn */
                .long    I2C2_MST_IRQHandler                /* 185 I2C2_MST_IRQn */
                .long    I2C2_SLV_IRQHandler                /* 186 I2C2_SLV_IRQn */
                .long    0                                  /* 187 Reserved */
                .long    PMBUS_IRQHandler                   /* 188 PMBUS_IRQn */
                .long    SPI1_RX_IRQHandler                 /* 189 SPI1_RX_IRQn */
                .long    SPI1_TX_IRQHandler                 /* 190 SPI1_TX_IRQn */
                .long    SPI2_RX_IRQHandler                 /* 191 SPI2_RX_IRQn */
                .long    SPI2_TX_IRQHandler                 /* 192 SPI2_TX_IRQn */
                .long    SPI3_RX_IRQHandler                 /* 193 SPI3_RX_IRQn */
                .long    SPI3_TX_IRQHandler                 /* 194 SPI3_TX_IRQn */
                .long    SPI4_RX_IRQHandler                 /* 195 SPI4_RX_IRQn */
                .long    SPI4_TX_IRQHandler                 /* 196 SPI4_TX_IRQn */
                .long    UART1_TX_IRQHandler                /* 197 UART1_TX_IRQn */
                .long    UART1_RX_IRQHandler                /* 198 UART1_RX_IRQn */
                .long    UART2_TX_IRQHandler                /* 199 UART2_TX_IRQn */
                .long    UART2_RX_IRQHandler                /* 200 UART2_RX_IRQn */
                .long    UART3_TX_IRQHandler                /* 201 UART3_TX_IRQn */
                .long    UART3_RX_IRQHandler                /* 202 UART3_RX_IRQn */
                .long    UART4_TX_IRQHandler                /* 203 UART4_TX_IRQn */
                .long    UART4_RX_IRQHandler                /* 204 UART4_RX_IRQn */
                .long    AESHASH_IRQHandler                 /* 205 AES_HASH_IRQn */
                .long    TRNG_IRQHandler                    /* 206 TRNG_IRQn */
                .long    QSPI_IRQHandler                    /* 207 QSPI_IRQn */
                .long    EMATH_IRQHandler                   /* 208 EMATH_IRQn */
                .long    DCC1_IRQHandler                    /* 209 DCC1_IRQn */
                .long    DCC2_IRQHandler                    /* 210 DCC2_IRQn */
                .long    DCC3_IRQHandler                    /* 211 DCC3_IRQn */
                .long    MCM_IRQHandler                     /* 212 MCM_IRQn */
                .long    VDDRDY_IRQHandler                  /* 213 VDDRDY_IRQn */
                .long    FMU_CFG_IRQHandler                 /* 214 FMU_CFG_IRQn */
                .long    FMU_EVT_IRQHandler                 /* 215 FMU_EVT_IRQn */
                .long    CLB1_IRQHandler                    /* 216 CLB1_CFG_IRQn */
                .long    CLB2_IRQHandler                    /* 217 CLB2_CFG_IRQn */
                .long    CLB3_IRQHandler                    /* 218 CLB3_CFG_IRQn */
                .long    CLB4_IRQHandler                    /* 219 CLB4_CFG_IRQn */
                .long    CLB5_IRQHandler                    /* 220 CLB5_CFG_IRQn */
                .long    CLB6_IRQHandler                    /* 221 CLB6_CFG_IRQn */
                .long    EMIF_IRQHandler                    /* 222 EMIF_IRQn */
                .long    UPP_IRQHandler                     /* 223 UPP_IRQn */
                .long    MU_IRQHandler                      /* 224 MU_IRQn */
                .long    MMATH_IRQHandler                   /* 225 MMATH_IRQn */
                .long    EXTI_TXEV_IRQHandler               /* 226 EXTI_TXEV_IRQn */


__Vectors_End:
                .equ     __Vectors_Size, __Vectors_End - __Vectors
                .size    __Vectors, . - __Vectors

                .thumb
                .section .text
                .align   2

                .thumb_func
                .type    Reset_Handler, %function
                .globl   Reset_Handler
                .fnstart
                .equ     VTOR, 0xE000ED08
Reset_Handler:
                cpsid    i                                  /* Mask interrupts */
                ldr      r0, =VTOR
                ldr      r1, =__Vectors
                str      r1, [r0]                           /* Set VTOR */
                ldr      r2, [r1]
                msr      msp, r2                            /* Set MSP */
                ldr      r0, =SystemInit
                blx      r0
                cpsie    i                                  /* Unmask interrupts */

                ldr      r4, =__copy_table_start__
                ldr      r5, =__copy_table_end__

.copy_start:
                cmp      r4, r5
                bge      .copy_done
                ldr      r1, [r4]                           /* Source address */
                ldr      r2, [r4, #4]                       /* Destination address */
                ldr      r3, [r4, #8]                       /* Word count */
                lsls     r3, r3, #2                         /* Byte count */

.copy_one_data:
                subs     r3, #4                             /* Decrement byte count */
                ittt     ge
                ldrge    r0, [r1, r3]
                strge    r0, [r2, r3]
                bge      .copy_one_data

                adds     r4, #12
                b        .copy_start
.copy_done:

                ldr      r3, =__zero_table_start__
                ldr      r4, =__zero_table_end__

.zero_start:
                cmp      r3, r4
                bge      .zero_done
                ldr      r1, [r3]                           /* Destination address */
                ldr      r2, [r3, #4]                       /* Word count */
                lsls     r2, r2, #2                         /* Byte count */
                movs     r0, 0

.zero_one_data:
                subs     r2, #4                             /* Decrement byte count */
                itt      ge
                strge    r0, [r1, r2]
                bge      .zero_one_data

                adds     r3, #8
                b        .zero_start
.zero_done:

                /*
                 * RT-Thread BSP 入口：
                 * 复位后必须进入 entry -> rtthread_startup()，由内核完成
                 * heap、timer、scheduler 和 main 线程创建，再在线程环境中
                 * 调用应用 main()。如果这里直接跳 main()，rt_malloc 等内核
                 * 服务尚未初始化，会触发 heap 断言。
                 */
                bl       entry

                .fnend
                .size    Reset_Handler, . - Reset_Handler

/*
 * The default macro is not used for HardFault_Handler.
 * Because this results in a poor debug illusion.
 */
                .thumb_func
                .type    HardFault_Handler, %function
                .weak    HardFault_Handler
                .fnstart
HardFault_Handler:
                b        .
                .fnend
                .size    HardFault_Handler, . - HardFault_Handler

/* The default handler */
                .thumb_func
                .type    Default_Handler, %function
                .weak    Default_Handler
                .fnstart
Default_Handler:
                b        .
                .fnend
                .size    Default_Handler, . - Default_Handler

/*
 * Macro to define default exception/interrupt handlers.
 * Default handler are weak symbols with an endless loop,
 * which will be overwritten by SDK handlers.
 * Users can read the SDK handlers,
 * and learn how to write their own exception/interrupt handlers.
 */
                .macro   Set_Default_Handler  Handler_Name
                .weak    \Handler_Name
                .set     \Handler_Name, Default_Handler
                .endm

/* Default exception/interrupt  handler */
                Set_Default_Handler  NMI_Handler
                Set_Default_Handler  MemManage_Handler
                Set_Default_Handler  BusFault_Handler
                Set_Default_Handler  UsageFault_Handler
                Set_Default_Handler  SVC_Handler
                Set_Default_Handler  DebugMon_Handler
                Set_Default_Handler  PendSV_Handler
                Set_Default_Handler  SysTick_Handler

                Set_Default_Handler  WWDG_IRQHandler
                Set_Default_Handler  IWDG1_IRQHandler
                Set_Default_Handler  IWDG2_IRQHandler
                Set_Default_Handler  LVD_IRQHandler
                Set_Default_Handler  FLASH_IRQHandler
                Set_Default_Handler  FLASH_ECCERR_IRQHandler
                Set_Default_Handler  RAM_ECCERR_IRQHandler  
                Set_Default_Handler  RCC_IRQHandler         
                Set_Default_Handler  EXTI3_0_IRQHandler     
                Set_Default_Handler  EXTI7_4_IRQHandler     
                Set_Default_Handler  EXTI11_8_IRQHandler    
                Set_Default_Handler  EXTI15_12_IRQHandler   
                Set_Default_Handler  EXTI_CMP_IRQHandler    
                Set_Default_Handler  EXTI_XINT2_1_IRQHandler
                Set_Default_Handler  EXTI_XINT5_3_IRQHandler
                Set_Default_Handler  EDMA1_CH0_IRQHandler   
                Set_Default_Handler  EDMA1_CH1_IRQHandler   
                Set_Default_Handler  EDMA1_CH2_IRQHandler   
                Set_Default_Handler  EDMA1_CH3_IRQHandler   
                Set_Default_Handler  EDMA1_CH4_IRQHandler   
                Set_Default_Handler  EDMA1_CH5_IRQHandler   
                Set_Default_Handler  EDMA1_CH6_IRQHandler   
                Set_Default_Handler  EDMA1_CH7_IRQHandler   
                Set_Default_Handler  EDMA1_CH8_IRQHandler   
                Set_Default_Handler  EDMA1_CH9_IRQHandler   
                Set_Default_Handler  EDMA1_CH10_IRQHandler  
                Set_Default_Handler  EDMA1_CH11_IRQHandler  
                Set_Default_Handler  EDMA1_CH12_IRQHandler  
                Set_Default_Handler  EDMA1_CH13_IRQHandler  
                Set_Default_Handler  EDMA1_CH14_IRQHandler  
                Set_Default_Handler  EDMA1_CH15_IRQHandler  
                Set_Default_Handler  EDMA1_CH16_IRQHandler  
                Set_Default_Handler  EDMA1_CH17_IRQHandler  
                Set_Default_Handler  EDMA1_CH18_IRQHandler  
                Set_Default_Handler  EDMA1_CH19_IRQHandler  
                Set_Default_Handler  EDMA1_CH20_IRQHandler  
                Set_Default_Handler  EDMA1_CH21_IRQHandler  
                Set_Default_Handler  EDMA1_CH22_IRQHandler  
                Set_Default_Handler  EDMA1_CH23_IRQHandler  
                Set_Default_Handler  EDMA1_CH24_IRQHandler  
                Set_Default_Handler  EDMA1_CH25_IRQHandler  
                Set_Default_Handler  EDMA1_CH26_IRQHandler  
                Set_Default_Handler  EDMA1_CH27_IRQHandler  
                Set_Default_Handler  EDMA1_CH28_IRQHandler  
                Set_Default_Handler  EDMA1_CH29_IRQHandler  
                Set_Default_Handler  EDMA1_CH30_IRQHandler  
                Set_Default_Handler  EDMA1_CH31_IRQHandler  
                Set_Default_Handler  EDMA1_ERR_IRQHandler   
                Set_Default_Handler  EDMA2_CH0_IRQHandler   
                Set_Default_Handler  EDMA2_CH1_IRQHandler   
                Set_Default_Handler  EDMA2_CH2_IRQHandler   
                Set_Default_Handler  EDMA2_CH3_IRQHandler   
                Set_Default_Handler  EDMA2_CH4_IRQHandler   
                Set_Default_Handler  EDMA2_CH5_IRQHandler   
                Set_Default_Handler  EDMA2_CH6_IRQHandler   
                Set_Default_Handler  EDMA2_CH7_IRQHandler   
                Set_Default_Handler  EDMA2_CH8_IRQHandler   
                Set_Default_Handler  EDMA2_CH9_IRQHandler   
                Set_Default_Handler  EDMA2_CH10_IRQHandler  
                Set_Default_Handler  EDMA2_CH11_IRQHandler  
                Set_Default_Handler  EDMA2_CH12_IRQHandler  
                Set_Default_Handler  EDMA2_CH13_IRQHandler  
                Set_Default_Handler  EDMA2_CH14_IRQHandler  
                Set_Default_Handler  EDMA2_CH15_IRQHandler  
                Set_Default_Handler  EDMA2_CH16_IRQHandler  
                Set_Default_Handler  EDMA2_CH17_IRQHandler  
                Set_Default_Handler  EDMA2_CH18_IRQHandler  
                Set_Default_Handler  EDMA2_CH19_IRQHandler  
                Set_Default_Handler  EDMA2_CH20_IRQHandler  
                Set_Default_Handler  EDMA2_CH21_IRQHandler  
                Set_Default_Handler  EDMA2_CH22_IRQHandler  
                Set_Default_Handler  EDMA2_CH23_IRQHandler  
                Set_Default_Handler  EDMA2_CH24_IRQHandler  
                Set_Default_Handler  EDMA2_CH25_IRQHandler  
                Set_Default_Handler  EDMA2_CH26_IRQHandler  
                Set_Default_Handler  EDMA2_CH27_IRQHandler  
                Set_Default_Handler  EDMA2_CH28_IRQHandler  
                Set_Default_Handler  EDMA2_CH29_IRQHandler  
                Set_Default_Handler  EDMA2_CH30_IRQHandler  
                Set_Default_Handler  EDMA2_CH31_IRQHandler  
                Set_Default_Handler  EDMA2_ERR_IRQHandler   
                Set_Default_Handler  ADCA_CONV_IRQHandler   
                Set_Default_Handler  ADCA_EVENT_IRQHandler  
                Set_Default_Handler  ADCB_CONV_IRQHandler   
                Set_Default_Handler  ADCB_EVENT_IRQHandler
                Set_Default_Handler  ADCC_CONV_IRQHandler   
                Set_Default_Handler  ADCC_EVENT_IRQHandler  
                Set_Default_Handler  ADC_SAFE_CHK_IRQHandler
                Set_Default_Handler  EPWM1_TZINT_IRQHandler 
                Set_Default_Handler  EPWM2_TZINT_IRQHandler 
                Set_Default_Handler  EPWM3_TZINT_IRQHandler 
                Set_Default_Handler  EPWM4_TZINT_IRQHandler 
                Set_Default_Handler  EPWM5_TZINT_IRQHandler 
                Set_Default_Handler  EPWM6_TZINT_IRQHandler 
                Set_Default_Handler  EPWM7_TZINT_IRQHandler 
                Set_Default_Handler  EPWM8_TZINT_IRQHandler 
                Set_Default_Handler  EPWM9_TZINT_IRQHandler 
                Set_Default_Handler  EPWM10_TZINT_IRQHandler
                Set_Default_Handler  EPWM11_TZINT_IRQHandler
                Set_Default_Handler  EPWM12_TZINT_IRQHandler
                Set_Default_Handler  EPWM13_TZINT_IRQHandler
                Set_Default_Handler  EPWM14_TZINT_IRQHandler
                Set_Default_Handler  EPWM15_TZINT_IRQHandler
                Set_Default_Handler  EPWM16_TZINT_IRQHandler
                Set_Default_Handler  EPWM17_TZINT_IRQHandler
                Set_Default_Handler  EPWM18_TZINT_IRQHandler
                Set_Default_Handler  EPWM1_INT_IRQHandler   
                Set_Default_Handler  EPWM2_INT_IRQHandler   
                Set_Default_Handler  EPWM3_INT_IRQHandler   
                Set_Default_Handler  EPWM4_INT_IRQHandler   
                Set_Default_Handler  EPWM5_INT_IRQHandler   
                Set_Default_Handler  EPWM6_INT_IRQHandler   
                Set_Default_Handler  EPWM7_INT_IRQHandler   
                Set_Default_Handler  EPWM8_INT_IRQHandler   
                Set_Default_Handler  EPWM9_INT_IRQHandler   
                Set_Default_Handler  EPWM10_INT_IRQHandler  
                Set_Default_Handler  EPWM11_INT_IRQHandler  
                Set_Default_Handler  EPWM12_INT_IRQHandler  
                Set_Default_Handler  EPWM13_INT_IRQHandler  
                Set_Default_Handler  EPWM14_INT_IRQHandler  
                Set_Default_Handler  EPWM15_INT_IRQHandler  
                Set_Default_Handler  EPWM16_INT_IRQHandler  
                Set_Default_Handler  EPWM17_INT_IRQHandler  
                Set_Default_Handler  EPWM18_INT_IRQHandler  
                Set_Default_Handler  ECAP1_IRQHandler       
                Set_Default_Handler  ECAP2_IRQHandler       
                Set_Default_Handler  ECAP3_IRQHandler       
                Set_Default_Handler  ECAP4_IRQHandler       
                Set_Default_Handler  ECAP5_IRQHandler       
                Set_Default_Handler  ECAP6_IRQHandler
                Set_Default_Handler  ECAP7_IRQHandler       
                Set_Default_Handler  EQEP1_IRQHandler       
                Set_Default_Handler  EQEP2_IRQHandler       
                Set_Default_Handler  EQEP3_IRQHandler
                Set_Default_Handler  EQEP4_IRQHandler       
                Set_Default_Handler  EQEP5_IRQHandler       
                Set_Default_Handler  EQEP6_IRQHandler       
                Set_Default_Handler  EPG_IRQHandler         
                Set_Default_Handler  SDFM1_DR_1_IRQHandler  
                Set_Default_Handler  SDFM1_DR_2_IRQHandler  
                Set_Default_Handler  SDFM1_DR_3_IRQHandler  
                Set_Default_Handler  SDFM1_DR_4_IRQHandler  
                Set_Default_Handler  SDFM1_ERR_IRQHandler   
                Set_Default_Handler  SDFM2_DR_1_IRQHandler  
                Set_Default_Handler  SDFM2_DR_2_IRQHandler  
                Set_Default_Handler  SDFM2_DR_3_IRQHandler  
                Set_Default_Handler  SDFM2_DR_4_IRQHandler  
                Set_Default_Handler  SDFM2_ERR_IRQHandler
                Set_Default_Handler  SDFM3_DR_1_IRQHandler  
                Set_Default_Handler  SDFM3_DR_2_IRQHandler  
                Set_Default_Handler  SDFM3_DR_3_IRQHandler  
                Set_Default_Handler  SDFM3_DR_4_IRQHandler  
                Set_Default_Handler  SDFM3_ERR_IRQHandler   
                Set_Default_Handler  SDFM4_DR_1_IRQHandler  
                Set_Default_Handler  SDFM4_DR_2_IRQHandler  
                Set_Default_Handler  SDFM4_DR_3_IRQHandler  
                Set_Default_Handler  SDFM4_DR_4_IRQHandler  
                Set_Default_Handler  SDFM4_ERR_IRQHandler   
                Set_Default_Handler  LPTIM_IRQHandler       
                Set_Default_Handler  CAN1_1_IRQHandler      
                Set_Default_Handler  CAN1_2_IRQHandler      
                Set_Default_Handler  CAN1_ECC_IRQHandler    
                Set_Default_Handler  CAN1_WKUP_IRQHandler   
                Set_Default_Handler  CANFD1_1_IRQHandler    
                Set_Default_Handler  CANFD1_2_IRQHandler    
                Set_Default_Handler  CANFD1_ECC_IRQHandler  
                Set_Default_Handler  CANFD1_WKUP_TS_IRQHandler 
                Set_Default_Handler  CANFD2_1_IRQHandler    
                Set_Default_Handler  CANFD2_2_IRQHandler    
                Set_Default_Handler  CANFD2_ECC_IRQHandler  
                Set_Default_Handler  CANFD2_WKUP_TS_IRQHandler 
                Set_Default_Handler  BTIM1_IRQHandler       
                Set_Default_Handler  BTIM2_IRQHandler       
                Set_Default_Handler  STIM1_IRQHandler       
                Set_Default_Handler  STIM2_IRQHandler       
                Set_Default_Handler  STIM3_IRQHandler       
                Set_Default_Handler  TIM1_IRQHandler        
                Set_Default_Handler  TIM2_IRQHandler        
                Set_Default_Handler  AWK_IRQHandler
                Set_Default_Handler  LIN1_IRQHandler        
                Set_Default_Handler  LIN2_IRQHandler        
                Set_Default_Handler  I2C1_MST_IRQHandler    
                Set_Default_Handler  I2C1_SLV_IRQHandler    
                Set_Default_Handler  I2C2_MST_IRQHandler    
                Set_Default_Handler  I2C2_SLV_IRQHandler
                Set_Default_Handler  PMBUS_IRQHandler        
                Set_Default_Handler  SPI1_RX_IRQHandler     
                Set_Default_Handler  SPI1_TX_IRQHandler     
                Set_Default_Handler  SPI2_RX_IRQHandler     
                Set_Default_Handler  SPI2_TX_IRQHandler     
                Set_Default_Handler  SPI3_RX_IRQHandler     
                Set_Default_Handler  SPI3_TX_IRQHandler     
                Set_Default_Handler  SPI4_RX_IRQHandler     
                Set_Default_Handler  SPI4_TX_IRQHandler     
                Set_Default_Handler  UART1_TX_IRQHandler    
                Set_Default_Handler  UART1_RX_IRQHandler    
                Set_Default_Handler  UART2_TX_IRQHandler    
                Set_Default_Handler  UART2_RX_IRQHandler    
                Set_Default_Handler  UART3_TX_IRQHandler    
                Set_Default_Handler  UART3_RX_IRQHandler    
                Set_Default_Handler  UART4_TX_IRQHandler    
                Set_Default_Handler  UART4_RX_IRQHandler    
                Set_Default_Handler  AESHASH_IRQHandler    
                Set_Default_Handler  TRNG_IRQHandler    
                Set_Default_Handler  QSPI_IRQHandler   
                Set_Default_Handler  EMATH_IRQHandler       
                Set_Default_Handler  DCC1_IRQHandler    
                Set_Default_Handler  DCC2_IRQHandler    
                Set_Default_Handler  DCC3_IRQHandler    
                Set_Default_Handler  MCM_IRQHandler    
                Set_Default_Handler  VDDRDY_IRQHandler 
                Set_Default_Handler  FMU_CFG_IRQHandler
                Set_Default_Handler  FMU_EVT_IRQHandler
                Set_Default_Handler  CLB1_IRQHandler   
                Set_Default_Handler  CLB2_IRQHandler   
                Set_Default_Handler  CLB3_IRQHandler   
                Set_Default_Handler  CLB4_IRQHandler   
                Set_Default_Handler  CLB5_IRQHandler   
                Set_Default_Handler  CLB6_IRQHandler   
                Set_Default_Handler  EMIF_IRQHandler   
                Set_Default_Handler  UPP_IRQHandler    
                Set_Default_Handler  MU_IRQHandler     
                Set_Default_Handler  MMATH_IRQHandler  
                Set_Default_Handler  EXTI_TXEV_IRQHandler        


                .end
