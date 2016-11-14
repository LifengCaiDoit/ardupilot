/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <AP_HAL/AP_HAL.h>

#include "AP_InertialSensor_LSM9DS0.h"

#include <utility>
#include <stdio.h>

#include <AP_HAL_Linux/GPIO.h>

extern const AP_HAL::HAL &hal;

#define LSM9DS0_G_WHOAMI    0xD4
#define LSM9DS0_XM_WHOAMI   0x49

#define LSM9DS0_DEBUG

////////////////////////////
// LSM9DS0 Gyro Registers //
////////////////////////////
#define WHO_AM_I_G                                    0x0F
#define CTRL_REG1_G                                   0x20
#   define CTRL_REG1_G_DR_95Hz_BW_12500mHz      (0x0 << 4)
#   define CTRL_REG1_G_DR_95Hz_BW_25Hz          (0x1 << 4)
#   define CTRL_REG1_G_DR_190Hz_BW_12500mHz     (0x4 << 4)
#   define CTRL_REG1_G_DR_190Hz_BW_25Hz         (0x5 << 4)
#   define CTRL_REG1_G_DR_190Hz_BW_50Hz         (0x6 << 4)
#   define CTRL_REG1_G_DR_190Hz_BW_70Hz         (0x7 << 4)
#   define CTRL_REG1_G_DR_380Hz_BW_20Hz         (0x8 << 4)
#   define CTRL_REG1_G_DR_380Hz_BW_25Hz         (0x9 << 4)
#   define CTRL_REG1_G_DR_380Hz_BW_50Hz         (0xA << 4)
#   define CTRL_REG1_G_DR_380Hz_BW_100Hz        (0xB << 4)
#   define CTRL_REG1_G_DR_760Hz_BW_30Hz         (0xC << 4)
#   define CTRL_REG1_G_DR_760Hz_BW_35Hz         (0xD << 4)
#   define CTRL_REG1_G_DR_760Hz_BW_50Hz         (0xE << 4)
#   define CTRL_REG1_G_DR_760Hz_BW_100Hz        (0xF << 4)
#   define CTRL_REG1_G_PD                       (0x1 << 3)
#   define CTRL_REG1_G_ZEN                      (0x1 << 2)
#   define CTRL_REG1_G_YEN                      (0x1 << 1)
#   define CTRL_REG1_G_XEN                      (0x1 << 0)
#define CTRL_REG2_G                                   0x21
#   define CTRL_REG2_G_HPM_NORMAL_RESET         (0x0 << 4)
#   define CTRL_REG2_G_HPM_REFERENCE            (0x1 << 4)
#   define CTRL_REG2_G_HPM_NORMAL               (0x2 << 4)
#   define CTRL_REG2_G_HPM_AUTORESET            (0x3 << 4)
#   define CTRL_REG2_G_HPCF_0                   (0x0 << 0)
#   define CTRL_REG2_G_HPCF_1                   (0x1 << 0)
#   define CTRL_REG2_G_HPCF_2                   (0x2 << 0)
#   define CTRL_REG2_G_HPCF_3                   (0x3 << 0)
#   define CTRL_REG2_G_HPCF_4                   (0x4 << 0)
#   define CTRL_REG2_G_HPCF_5                   (0x5 << 0)
#   define CTRL_REG2_G_HPCF_6                   (0x6 << 0)
#   define CTRL_REG2_G_HPCF_7                   (0x7 << 0)
#   define CTRL_REG2_G_HPCF_8                   (0x8 << 0)
#   define CTRL_REG2_G_HPCF_9                   (0x9 << 0)
#define CTRL_REG3_G                                   0x22
#   define CTRL_REG3_G_I1_INT1                  (0x1 << 7)
#   define CTRL_REG3_G_I1_BOOT                  (0x1 << 6)
#   define CTRL_REG3_G_H_LACTIVE                (0x1 << 5)
#   define CTRL_REG3_G_PP_OD                    (0x1 << 4)
#   define CTRL_REG3_G_I2_DRDY                  (0x1 << 3)
#   define CTRL_REG3_G_I2_WTM                   (0x1 << 2)
#   define CTRL_REG3_G_I2_ORUN                  (0x1 << 1)
#   define CTRL_REG3_G_I2_EMPTY                 (0x1 << 0)
#define CTRL_REG4_G                                   0x23
#   define CTRL_REG4_G_BDU                      (0x1 << 7)
#   define CTRL_REG4_G_BLE                      (0x1 << 6)
#   define CTRL_REG4_G_FS_245DPS                (0x0 << 4)
#   define CTRL_REG4_G_FS_500DPS                (0x1 << 4)
#   define CTRL_REG4_G_FS_2000DPS               (0x2 << 4)
#   define CTRL_REG4_G_ST_NORMAL                (0x0 << 1)
#   define CTRL_REG4_G_ST_0                     (0x1 << 1)
#   define CTRL_REG4_G_ST_1                     (0x3 << 1)
#   define CTRL_REG4_G_SIM_3WIRE                (0x1 << 0)
#define CTRL_REG5_G                                   0x24
#   define CTRL_REG5_G_BOOT                     (0x1 << 7)
#   define CTRL_REG5_G_FIFO_EN                  (0x1 << 6)
#   define CTRL_REG5_G_HPEN                     (0x1 << 4)
#   define CTRL_REG5_G_INT1_SEL_00              (0x0 << 2)
#   define CTRL_REG5_G_INT1_SEL_01              (0x1 << 2)
#   define CTRL_REG5_G_INT1_SEL_10              (0x2 << 2)
#   define CTRL_REG5_G_INT1_SEL_11              (0x3 << 2)
#   define CTRL_REG5_G_OUT_SEL_00               (0x0 << 0)
#   define CTRL_REG5_G_OUT_SEL_01               (0x1 << 0)
#   define CTRL_REG5_G_OUT_SEL_10               (0x2 << 0)
#   define CTRL_REG5_G_OUT_SEL_11               (0x3 << 0)
#define REFERENCE_G                                   0x25
#define STATUS_REG_G                                  0x27
#   define STATUS_REG_G_ZYXOR                   (0x1 << 7)
#   define STATUS_REG_G_ZOR                     (0x1 << 6)
#   define STATUS_REG_G_YOR                     (0x1 << 5)
#   define STATUS_REG_G_XOR                     (0x1 << 4)
#   define STATUS_REG_G_ZYXDA                   (0x1 << 3)
#   define STATUS_REG_G_ZDA                     (0x1 << 2)
#   define STATUS_REG_G_YDA                     (0x1 << 1)
#   define STATUS_REG_G_XDA                     (0x1 << 0)
#define OUT_X_L_G                                     0x28
#define OUT_X_H_G                                     0x29
#define OUT_Y_L_G                                     0x2A
#define OUT_Y_H_G                                     0x2B
#define OUT_Z_L_G                                     0x2C
#define OUT_Z_H_G                                     0x2D
#define FIFO_CTRL_REG_G                               0x2E
#   define FIFO_CTRL_REG_G_FM_BYPASS            (0x0 << 5)
#   define FIFO_CTRL_REG_G_FM_FIFO              (0x1 << 5)
#   define FIFO_CTRL_REG_G_FM_STREAM            (0x2 << 5)
#   define FIFO_CTRL_REG_G_FM_STREAM_TO_FIFO    (0x3 << 5)
#   define FIFO_CTRL_REG_G_FM_BYPASS_TO_STREAM  (0x4 << 5)
#   define FIFO_CTRL_REG_G_WTM_MASK                   0x1F
#define FIFO_SRC_REG_G                                0x2F
#   define FIFO_SRC_REG_G_WTM                   (0x1 << 7)
#   define FIFO_SRC_REG_G_OVRN                  (0x1 << 6)
#   define FIFO_SRC_REG_G_EMPTY                 (0x1 << 5)
#   define FIFO_SRC_REG_G_FSS_MASK                    0x1F
#define INT1_CFG_G                                    0x30
#   define INT1_CFG_G_AND_OR                    (0x1 << 7)
#   define INT1_CFG_G_LIR                       (0x1 << 6)
#   define INT1_CFG_G_ZHIE                      (0x1 << 5)
#   define INT1_CFG_G_ZLIE                      (0x1 << 4)
#   define INT1_CFG_G_YHIE                      (0x1 << 3)
#   define INT1_CFG_G_YLIE                      (0x1 << 2)
#   define INT1_CFG_G_XHIE                      (0x1 << 1)
#   define INT1_CFG_G_XLIE                      (0x1 << 0)
#define INT1_SRC_G                                    0x31
#   define INT1_SRC_G_IA                        (0x1 << 6)
#   define INT1_SRC_G_ZH                        (0x1 << 5)
#   define INT1_SRC_G_ZL                        (0x1 << 4)
#   define INT1_SRC_G_YH                        (0x1 << 3)
#   define INT1_SRC_G_YL                        (0x1 << 2)
#   define INT1_SRC_G_XH                        (0x1 << 1)
#   define INT1_SRC_G_XL                        (0x1 << 0)
#define INT1_THS_XH_G                                 0x32
#define INT1_THS_XL_G                                 0x33
#define INT1_THS_YH_G                                 0x34
#define INT1_THS_YL_G                                 0x35
#define INT1_THS_ZH_G                                 0x36
#define INT1_THS_ZL_G                                 0x37
#define INT1_DURATION_G                               0x38
#   define INT1_DURATION_G_WAIT                 (0x1 << 7)
#   define INT1_DURATION_G_D_MASK                     0x7F

//////////////////////////////////////////
// LSM9DS0 Accel/Magneto (XM) Registers //
//////////////////////////////////////////
#define OUT_TEMP_L_XM                                 0x05
#define OUT_TEMP_H_XM                                 0x06
#define STATUS_REG_M                                  0x07
#   define STATUS_REG_M_ZYXMOR                  (0x1 << 7)
#   define STATUS_REG_M_ZMOR                    (0x1 << 6)
#   define STATUS_REG_M_YMOR                    (0x1 << 5)
#   define STATUS_REG_M_XMOR                    (0x1 << 4)
#   define STATUS_REG_M_ZYXMDA                  (0x1 << 3)
#   define STATUS_REG_M_ZMDA                    (0x1 << 2)
#   define STATUS_REG_M_YMDA                    (0x1 << 1)
#   define STATUS_REG_M_XMDA                    (0x1 << 0)
#define OUT_X_L_M                                     0x08
#define OUT_X_H_M                                     0x09
#define OUT_Y_L_M                                     0x0A
#define OUT_Y_H_M                                     0x0B
#define OUT_Z_L_M                                     0x0C
#define OUT_Z_H_M                                     0x0D
#define WHO_AM_I_XM                                   0x0F
#define INT_CTRL_REG_M                                0x12
#   define INT_CTRL_REG_M_XMIEN                 (0x1 << 7)
#   define INT_CTRL_REG_M_YMIEN                 (0x1 << 6)
#   define INT_CTRL_REG_M_ZMIEN                 (0x1 << 5)
#   define INT_CTRL_REG_M_PP_OD                 (0x1 << 4)
#   define INT_CTRL_REG_M_IEA                   (0x1 << 3)
#   define INT_CTRL_REG_M_IEL                   (0x1 << 2)
#   define INT_CTRL_REG_M_4D                    (0x1 << 1)
#   define INT_CTRL_REG_M_MIEN                  (0x1 << 0)
#define INT_SRC_REG_M                                 0x13
#   define INT_SRC_REG_M_M_PTH_X                (0x1 << 7)
#   define INT_SRC_REG_M_M_PTH_Y                (0x1 << 6)
#   define INT_SRC_REG_M_M_PTH_Z                (0x1 << 5)
#   define INT_SRC_REG_M_M_NTH_X                (0x1 << 4)
#   define INT_SRC_REG_M_M_NTH_Y                (0x1 << 3)
#   define INT_SRC_REG_M_M_NTH_Z                (0x1 << 2)
#   define INT_SRC_REG_M_MROI                   (0x1 << 1)
#   define INT_SRC_REG_M_MINT                   (0x1 << 0)
#define INT_THS_L_M                                   0x14
#define INT_THS_H_M                                   0x15
#define OFFSET_X_L_M                                  0x16
#define OFFSET_X_H_M                                  0x17
#define OFFSET_Y_L_M                                  0x18
#define OFFSET_Y_H_M                                  0x19
#define OFFSET_Z_L_M                                  0x1A
#define OFFSET_Z_H_M                                  0x1B
#define REFERENCE_X                                   0x1C
#define REFERENCE_Y                                   0x1D
#define REFERENCE_Z                                   0x1E
#define CTRL_REG0_XM                                  0x1F
#   define CTRL_REG0_XM_B00T                    (0x1 << 7)
#   define CTRL_REG0_XM_FIFO_EN                 (0x1 << 6)
#   define CTRL_REG0_XM_WTM_EN                  (0x1 << 5)
#   define CTRL_REG0_XM_HP_CLICK                (0x1 << 2)
#   define CTRL_REG0_XM_HPIS1                   (0x1 << 1)
#   define CTRL_REG0_XM_HPIS2                   (0x1 << 0)
#define CTRL_REG1_XM                                  0x20
#   define CTRL_REG1_XM_AODR_POWERDOWN          (0x0 << 4)
#   define CTRL_REG1_XM_AODR_3125mHz            (0x1 << 4)
#   define CTRL_REG1_XM_AODR_6250mHz            (0x2 << 4)
#   define CTRL_REG1_XM_AODR_12500mHz           (0x3 << 4)
#   define CTRL_REG1_XM_AODR_25Hz               (0x4 << 4)
#   define CTRL_REG1_XM_AODR_50Hz               (0x5 << 4)
#   define CTRL_REG1_XM_AODR_100Hz              (0x6 << 4)
#   define CTRL_REG1_XM_AODR_200Hz              (0x7 << 4)
#   define CTRL_REG1_XM_AODR_400Hz              (0x8 << 4)
#   define CTRL_REG1_XM_AODR_800Hz              (0x9 << 4)
#   define CTRL_REG1_XM_AODR_1600Hz             (0xA << 4)
#   define CTRL_REG1_XM_BDU                     (0x1 << 3)
#   define CTRL_REG1_XM_AZEN                    (0x1 << 2)
#   define CTRL_REG1_XM_AYEN                    (0x1 << 1)
#   define CTRL_REG1_XM_AXEN                    (0x1 << 0)
#define CTRL_REG2_XM                                  0x21
#   define CTRL_REG2_XM_ABW_773Hz               (0x0 << 6)
#   define CTRL_REG2_XM_ABW_194Hz               (0x1 << 6)
#   define CTRL_REG2_XM_ABW_362Hz               (0x2 << 6)
#   define CTRL_REG2_XM_ABW_50Hz                (0x3 << 6)
#   define CTRL_REG2_XM_AFS_2G                  (0x0 << 3)
#   define CTRL_REG2_XM_AFS_4G                  (0x1 << 3)
#   define CTRL_REG2_XM_AFS_6G                  (0x2 << 3)
#   define CTRL_REG2_XM_AFS_8G                  (0x3 << 3)
#   define CTRL_REG2_XM_AFS_16G                 (0x4 << 3)
#   define CTRL_REG2_XM_AST_NORMAL              (0x0 << 1)
#   define CTRL_REG2_XM_AST_POSITIVE            (0x1 << 1)
#   define CTRL_REG2_XM_AST_NEGATIVE            (0x2 << 1)
#   define CTRL_REG2_XM_SIM_3WIRE               (0x1 << 0)
#define CTRL_REG3_XM                                  0x22
#   define CTRL_REG3_XM_P1_BOOT                 (0x1 << 7)
#   define CTRL_REG3_XM_P1_TAP                  (0x1 << 6)
#   define CTRL_REG3_XM_P1_INT1                 (0x1 << 5)
#   define CTRL_REG3_XM_P1_INT2                 (0x1 << 4)
#   define CTRL_REG3_XM_P1_INTM                 (0x1 << 3)
#   define CTRL_REG3_XM_P1_DRDYA                (0x1 << 2)
#   define CTRL_REG3_XM_P1_DRDYM                (0x1 << 1)
#   define CTRL_REG3_XM_P1_EMPTY                (0x1 << 0)
#define CTRL_REG4_XM                                  0x23
#   define CTRL_REG4_XM_P2_TAP                  (0x1 << 7)
#   define CTRL_REG4_XM_P2_INT1                 (0x1 << 6)
#   define CTRL_REG4_XM_P2_INT2                 (0x1 << 5)
#   define CTRL_REG4_XM_P2_INTM                 (0x1 << 4)
#   define CTRL_REG4_XM_P2_DRDYA                (0x1 << 3)
#   define CTRL_REG4_XM_P2_DRDYM                (0x1 << 2)
#   define CTRL_REG4_XM_P2_OVERRUN              (0x1 << 1)
#   define CTRL_REG4_XM_P2_WTM                  (0x1 << 0)
#define CTRL_REG5_XM                                  0x24
#   define CTRL_REG5_XM_TEMP_EN                 (0x1 << 7)
#   define CTRL_REG5_XM_M_RES_LOW               (0x0 << 5)
#   define CTRL_REG5_XM_M_RES_HIGH              (0x3 << 5)
#   define CTRL_REG5_XM_ODR_3125mHz             (0x0 << 2)
#   define CTRL_REG5_XM_ODR_6250mHz             (0x1 << 2)
#   define CTRL_REG5_XM_ODR_12500mHz            (0x2 << 2)
#   define CTRL_REG5_XM_ODR_25Hz                (0x3 << 2)
#   define CTRL_REG5_XM_ODR_50Hz                (0x4 << 2)
#   define CTRL_REG5_XM_ODR_100Hz               (0x5 << 2)
#   define CTRL_REG5_XM_LIR2                    (0x1 << 1)
#   define CTRL_REG5_XM_LIR1                    (0x1 << 0)
#define CTRL_REG6_XM                                  0x25
#   define CTRL_REG6_XM_MFS_2Gs                 (0x0 << 5)
#   define CTRL_REG6_XM_MFS_4Gs                 (0x1 << 5)
#   define CTRL_REG6_XM_MFS_8Gs                 (0x2 << 5)
#   define CTRL_REG6_XM_MFS_12Gs                (0x3 << 5)
#define CTRL_REG7_XM                                  0x26
#   define CTRL_REG7_XM_AHPM_NORMAL_RESET       (0x0 << 6)
#   define CTRL_REG7_XM_AHPM_REFERENCE          (0x1 << 6)
#   define CTRL_REG7_XM_AHPM_NORMAL             (0x2 << 6)
#   define CTRL_REG7_XM_AHPM_AUTORESET          (0x3 << 6)
#   define CTRL_REG7_XM_AFDS                    (0x1 << 5)
#   define CTRL_REG7_XM_MLP                     (0x1 << 2)
#   define CTRL_REG7_XM_MD_CONTINUOUS           (0x0 << 0)
#   define CTRL_REG7_XM_MD_SINGLE               (0x1 << 0)
#   define CTRL_REG7_XM_MD_POWERDOWN            (0x2 << 0)
#define STATUS_REG_A                                  0x27
#   define STATUS_REG_A_ZYXAOR                  (0x1 << 7)
#   define STATUS_REG_A_ZAOR                    (0x1 << 6)
#   define STATUS_REG_A_YAOR                    (0x1 << 5)
#   define STATUS_REG_A_XAOR                    (0x1 << 4)
#   define STATUS_REG_A_ZYXADA                  (0x1 << 3)
#   define STATUS_REG_A_ZADA                    (0x1 << 2)
#   define STATUS_REG_A_YADA                    (0x1 << 1)
#   define STATUS_REG_A_XADA                    (0x1 << 0)
#define OUT_X_L_A                                     0x28
#define OUT_X_H_A                                     0x29
#define OUT_Y_L_A                                     0x2A
#define OUT_Y_H_A                                     0x2B
#define OUT_Z_L_A                                     0x2C
#define OUT_Z_H_A                                     0x2D
#define FIFO_CTRL_REG                                 0x2E
#   define FIFO_CTRL_REG_FM_BYPASS              (0x0 << 5)
#   define FIFO_CTRL_REG_FM_FIFO                (0x1 << 5)
#   define FIFO_CTRL_REG_FM_STREAM              (0x2 << 5)
#   define FIFO_CTRL_REG_FM_STREAM_TO_FIFO      (0x3 << 5)
#   define FIFO_CTRL_REG_FM_BYPASS_TO_STREAM    (0x4 << 5)
#   define FIFO_CTRL_REG_FTH_MASK                     0x1F
#define FIFO_SRC_REG                                  0x2F
#   define FIFO_SRC_REG_WTM                     (0x1 << 7)
#   define FIFO_SRC_REG_OVRN                    (0x1 << 6)
#   define FIFO_SRC_REG_EMPTY                   (0x1 << 5)
#   define FIFO_SRC_REG_FSS_MASK                      0x1F
#define INT_GEN_1_REG                                 0x30
#   define INT_GEN_1_REG_AOI                    (0x1 << 7)
#   define INT_GEN_1_REG_6D                     (0x1 << 6)
#   define INT_GEN_1_REG_ZHIE_ZUPE              (0x1 << 5)
#   define INT_GEN_1_REG_ZLIE_ZDOWNE            (0x1 << 4)
#   define INT_GEN_1_REG_YHIE_YUPE              (0x1 << 3)
#   define INT_GEN_1_REG_YLIE_YDOWNE            (0x1 << 2)
#   define INT_GEN_1_REG_XHIE_XUPE              (0x1 << 1)
#   define INT_GEN_1_REG_XLIE_XDOWNE            (0x1 << 0)
#define INT_GEN_1_SRC                                 0x31
#   define INT_GEN_1_SRC_IA                     (0x1 << 6)
#   define INT_GEN_1_SRC_ZH                     (0x1 << 5)
#   define INT_GEN_1_SRC_ZL                     (0x1 << 4)
#   define INT_GEN_1_SRC_YH                     (0x1 << 3)
#   define INT_GEN_1_SRC_YL                     (0x1 << 2)
#   define INT_GEN_1_SRC_XH                     (0x1 << 1)
#   define INT_GEN_1_SRC_XL                     (0x1 << 0)
#define INT_GEN_1_THS                                 0x32
#define INT_GEN_1_DURATION                            0x33
#define INT_GEN_2_REG                                 0x34
#   define INT_GEN_2_REG_AOI                    (0x1 << 7)
#   define INT_GEN_2_REG_6D                     (0x1 << 6)
#   define INT_GEN_2_REG_ZHIE_ZUPE              (0x1 << 5)
#   define INT_GEN_2_REG_ZLIE_ZDOWNE            (0x1 << 4)
#   define INT_GEN_2_REG_YHIE_YUPE              (0x1 << 3)
#   define INT_GEN_2_REG_YLIE_YDOWNE            (0x1 << 2)
#   define INT_GEN_2_REG_XHIE_XUPE              (0x1 << 1)
#   define INT_GEN_2_REG_XLIE_XDOWNE            (0x1 << 0)
#define INT_GEN_2_SRC                                 0x35
#   define INT_GEN_2_SRC_IA                     (0x1 << 6)
#   define INT_GEN_2_SRC_ZH                     (0x1 << 5)
#   define INT_GEN_2_SRC_ZL                     (0x1 << 4)
#   define INT_GEN_2_SRC_YH                     (0x1 << 3)
#   define INT_GEN_2_SRC_YL                     (0x1 << 2)
#   define INT_GEN_2_SRC_XH                     (0x1 << 1)
#   define INT_GEN_2_SRC_XL                     (0x1 << 0)
#define INT_GEN_2_THS                                 0x36
#define INT_GEN_2_DURATION                            0x37
#define CLICK_CFG                                     0x38
#   define CLICK_CFG_ZD                         (0x1 << 5)
#   define CLICK_CFG_ZS                         (0x1 << 4)
#   define CLICK_CFG_YD                         (0x1 << 3)
#   define CLICK_CFG_YS                         (0x1 << 2)
#   define CLICK_CFG_XD                         (0x1 << 1)
#   define CLICK_CFG_XS                         (0x1 << 0)
#define CLICK_SRC                                     0x39
#   define CLICK_SRC_IA                         (0x1 << 6)
#   define CLICK_SRC_DCLICK                     (0x1 << 5)
#   define CLICK_SRC_SCLICK                     (0x1 << 4)
#   define CLICK_SRC_SIGN                       (0x1 << 3)
#   define CLICK_SRC_Z                          (0x1 << 2)
#   define CLICK_SRC_Y                          (0x1 << 1)
#   define CLICK_SRC_X                          (0x1 << 0)
#define CLICK_THS                                     0x3A
#define TIME_LIMIT                                    0x3B
#define TIME_LATENCY                                  0x3C
#define TIME_WINDOW                                   0x3D
#define ACT_THS                                       0x3E
#define ACT_DUR                                       0x3F

#define LSM9DS0_SAMPLE_SIZE 6
#define LSM9DS0_MAX_FIFO_SAMPLES 8
#define MAX_DATA_READ (LSM9DS0_MAX_FIFO_SAMPLES * LSM9DS0_SAMPLE_SIZE)

AP_InertialSensor_LSM9DS0::AP_InertialSensor_LSM9DS0(AP_InertialSensor &imu,
                                                     AP_HAL::OwnPtr<AP_HAL::SPIDevice> dev_gyro,
                                                     AP_HAL::OwnPtr<AP_HAL::SPIDevice> dev_accel,
                                                     enum Rotation rotation_a,
                                                     enum Rotation rotation_g)
    : AP_InertialSensor_Backend(imu)
    , _dev_gyro(std::move(dev_gyro))
    , _dev_accel(std::move(dev_accel))
    , _rotation_a(rotation_a)
    , _rotation_g(rotation_g)
{
}

AP_InertialSensor_LSM9DS0::~AP_InertialSensor_LSM9DS0()
{
    if (_fifo_buffer != nullptr) {
        delete[] _fifo_buffer;
    }
}

AP_InertialSensor_Backend *AP_InertialSensor_LSM9DS0::probe(AP_InertialSensor &_imu,
                                                            AP_HAL::OwnPtr<AP_HAL::SPIDevice> dev_gyro,
                                                            AP_HAL::OwnPtr<AP_HAL::SPIDevice> dev_accel,
                                                            enum Rotation rotation_a,
                                                            enum Rotation rotation_g)
{
    if (!dev_gyro || !dev_accel) {
        return nullptr;
    }
    AP_InertialSensor_LSM9DS0 *sensor =
        new AP_InertialSensor_LSM9DS0(_imu, std::move(dev_gyro), std::move(dev_accel),
                                      rotation_a, rotation_g);
    if (!sensor || !sensor->_init_sensor()) {
        delete sensor;
        return nullptr;
    }

    return sensor;
}

bool AP_InertialSensor_LSM9DS0::_init_sensor()
{
    /*
     * Same semaphore for both since they necessarily share the same bus (with
     * different CS)
     */
    _spi_sem = _dev_gyro->get_semaphore();

    bool success = _hardware_init();

    return success;
}

bool AP_InertialSensor_LSM9DS0::_hardware_init()
{
    if (!_spi_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) {
        return false;
    }

    uint8_t whoami;
    uint8_t tries;

    // set flag for reading registers
    _dev_gyro->set_read_flag(0x80);
    _dev_accel->set_read_flag(0x80);
    
    whoami = _register_read_g(WHO_AM_I_G);
    if (whoami != LSM9DS0_G_WHOAMI) {
        hal.console->printf("LSM9DS0: unexpected gyro WHOAMI 0x%x\n", (unsigned)whoami);
        goto fail_whoami;
    }

    whoami = _register_read_xm(WHO_AM_I_XM);
    if (whoami != LSM9DS0_XM_WHOAMI) {
        hal.console->printf("LSM9DS0: unexpected acc/mag  WHOAMI 0x%x\n", (unsigned)whoami);
        goto fail_whoami;
    }

    // setup for register checking
    _dev_gyro->setup_checked_registers(6);
    _dev_accel->setup_checked_registers(6);
        
    for (tries = 0; tries < 5; tries++) {
        _dev_gyro->set_speed(AP_HAL::Device::SPEED_LOW);
        _dev_accel->set_speed(AP_HAL::Device::SPEED_LOW);

        _gyro_init();
        _accel_init();

        _dev_gyro->set_speed(AP_HAL::Device::SPEED_HIGH);
        _dev_accel->set_speed(AP_HAL::Device::SPEED_HIGH);

        hal.scheduler->delay(10);
        if (_accel_data_ready() && _gyro_data_ready()) {
            break;
        }
    }
    if (tries == 5) {
        hal.console->println("Failed to boot LSM9DS0 5 times\n");
        goto fail_tries;
    }

    _spi_sem->give();

    return true;

fail_tries:
fail_whoami:
    _spi_sem->give();
    return false;
}


/*
  start the sensor going
 */
void AP_InertialSensor_LSM9DS0::start(void)
{
    _gyro_instance = _imu.register_gyro(760, _dev_gyro->get_bus_id_devtype(DEVTYPE_GYR_L3GD20));
    _accel_instance = _imu.register_accel(1600, _dev_accel->get_bus_id_devtype(DEVTYPE_ACC_LSM303D));

    set_gyro_orientation(_gyro_instance, _rotation_g);
    set_accel_orientation(_accel_instance, _rotation_a);
    
    _set_accel_max_abs_offset(_accel_instance, 5.0f);

    // allocate fifo buffer
    _fifo_buffer = new uint8_t[MAX_DATA_READ];
    if (_fifo_buffer == nullptr) {
        AP_HAL::panic("LSM9DS0: Unable to allocate FIFO buffer");
    }
    
    /* start the timer process to read samples */
    _dev_gyro->register_periodic_callback(1250, FUNCTOR_BIND_MEMBER(&AP_InertialSensor_LSM9DS0::_poll_data, bool));
}


uint8_t AP_InertialSensor_LSM9DS0::_register_read_xm(uint8_t reg)
{
    uint8_t val = 0;

    _dev_accel->read_registers(reg, &val, 1);

    return val;
}

uint8_t AP_InertialSensor_LSM9DS0::_register_read_g(uint8_t reg)
{
    uint8_t val = 0;

    _dev_gyro->read_registers(reg, &val, 1);

    return val;
}

void AP_InertialSensor_LSM9DS0::_register_write_xm(uint8_t reg, uint8_t val, bool checked)
{
    _dev_accel->write_register(reg, val, checked);
}

void AP_InertialSensor_LSM9DS0::_register_write_g(uint8_t reg, uint8_t val, bool checked)
{
    _dev_gyro->write_register(reg, val, checked);
}

void AP_InertialSensor_LSM9DS0::_gyro_disable_i2c()
{
    uint8_t retries = 10;
    while (retries--) {
        // add retries
        uint8_t a = _register_read_g(0x05);
        _register_write_g(0x05, (0x20 | a));
        if (_register_read_g(0x05) == (a | 0x20)) {
            return;
        }
    }
    AP_HAL::panic("LSM9DS0_G: Unable to disable I2C");
}

void AP_InertialSensor_LSM9DS0::_accel_disable_i2c()
{
    uint8_t a = _register_read_xm(0x02);
    _register_write_xm(0x02, (0x10 | a));
    a = _register_read_xm(0x02);
    _register_write_xm(0x02, (0xF7 & a));
    a = _register_read_xm(0x15);
    _register_write_xm(0x15, (0x80 | a));
    a = _register_read_xm(0x02);
    _register_write_xm(0x02, (0xE7 & a));
}

void AP_InertialSensor_LSM9DS0::_gyro_init()
{
    _gyro_disable_i2c();
    hal.scheduler->delay(1);

    _register_write_g(CTRL_REG1_G,
                      CTRL_REG1_G_DR_760Hz_BW_50Hz |
                      CTRL_REG1_G_PD |
                      CTRL_REG1_G_ZEN |
                      CTRL_REG1_G_YEN |
                      CTRL_REG1_G_XEN, true);
    hal.scheduler->delay(1);

    _register_write_g(CTRL_REG2_G, 0x00, true);
    hal.scheduler->delay(1);

    _register_write_g(CTRL_REG3_G, 0, true);
    hal.scheduler->delay(1);

    _register_write_g(CTRL_REG4_G,
                      CTRL_REG4_G_BDU |
                      CTRL_REG4_G_FS_2000DPS, true);
    _set_gyro_scale(G_SCALE_2000DPS);
    hal.scheduler->delay(1);

    _register_write_g(CTRL_REG5_G, CTRL_REG5_G_FIFO_EN, true);
    hal.scheduler->delay(1);

    _register_write_g(FIFO_CTRL_REG_G, FIFO_CTRL_REG_G_FM_STREAM | 0x1F, true);
    hal.scheduler->delay(1);
}

void AP_InertialSensor_LSM9DS0::_accel_init()
{
    // _accel_disable_i2c();
    hal.scheduler->delay(1);

    _register_write_xm(FIFO_CTRL_REG, FIFO_CTRL_REG_FM_STREAM | 0x1F, true);
    hal.scheduler->delay(1);

    _register_write_xm(CTRL_REG1_XM,
                       CTRL_REG1_XM_AODR_1600Hz |
                       CTRL_REG1_XM_BDU |
                       CTRL_REG1_XM_AZEN |
                       CTRL_REG1_XM_AYEN |
                       CTRL_REG1_XM_AXEN, true);
    hal.scheduler->delay(1);

    _register_write_xm(CTRL_REG2_XM,
                       CTRL_REG2_XM_ABW_194Hz |
                       CTRL_REG2_XM_AFS_16G, true);
    _set_accel_scale(A_SCALE_16G);
    hal.scheduler->delay(1);

    _register_write_xm(CTRL_REG5_XM,
                       CTRL_REG5_XM_TEMP_EN);
    
    _register_write_xm(CTRL_REG3_XM, 0, true);
    hal.scheduler->delay(1);

    _register_write_xm(CTRL_REG7_XM, 0, true);
    hal.scheduler->delay(1);

    _register_write_xm(CTRL_REG0_XM,
                       CTRL_REG0_XM_FIFO_EN, true);
    hal.scheduler->delay(1);

}

void AP_InertialSensor_LSM9DS0::_set_gyro_scale(gyro_scale scale)
{
    /* scales values from datasheet in mdps/digit */
    switch (scale) {
    case G_SCALE_245DPS:
        _gyro_scale = 8.75;
        break;
    case G_SCALE_500DPS:
        _gyro_scale = 17.50;
        break;
    case G_SCALE_2000DPS:
        _gyro_scale = 70;
        break;
    }

    /* convert mdps/digit to dps/digit */
    _gyro_scale /= 1000;
    /* convert dps/digit to (rad/s)/digit */
    _gyro_scale *= DEG_TO_RAD;
}

void AP_InertialSensor_LSM9DS0::_set_accel_scale(accel_scale scale)
{
    /*
     * Possible accelerometer scales (and their register bit settings) are:
     * 2 g (000), 4g (001), 6g (010) 8g (011), 16g (100). Here's a bit of an
     * algorithm to calculate g/(ADC tick) based on that 3-bit value:
     */
    _accel_scale = (((float) scale + 1.0f) * 2.0f) / 32768.0f;
    if (scale == A_SCALE_16G) {
        /* the datasheet shows an exception for +-16G */
        _accel_scale = 0.000732;
    }
    /* convert to G/LSB to (m/s/s)/LSB */
    _accel_scale *= GRAVITY_MSS;
}

/**
 * Timer process to poll for new data from the LSM9DS0.
 */
bool AP_InertialSensor_LSM9DS0::_poll_data()
{
    _read_data_transaction_g();
    _read_data_transaction_a();

    if (_reg_check_counter++ == 10) {
        _reg_check_counter = 0;
        // check next register value for correctness
        if (!_dev_gyro->check_next_register()) {
            _inc_gyro_error_count(_gyro_instance);
        }
        if (!_dev_accel->check_next_register()) {
            _inc_accel_error_count(_accel_instance);
        }
    }
    
    return true;
}

bool AP_InertialSensor_LSM9DS0::_accel_data_ready()
{
    uint8_t status = _register_read_xm(STATUS_REG_A);
    return status & STATUS_REG_A_ZYXADA;
}

bool AP_InertialSensor_LSM9DS0::_gyro_data_ready()
{
    uint8_t status = _register_read_g(STATUS_REG_G);
    return status & STATUS_REG_G_ZYXDA;
}

void AP_InertialSensor_LSM9DS0::_read_data_transaction_a()
{
    uint8_t fifo_src;
    
    // read fifo status register
    if (!_dev_accel->read_registers(FIFO_SRC_REG, &fifo_src, 1)) {
        return;
    }

    uint8_t n_samples = (fifo_src & FIFO_SRC_REG_FSS_MASK);
    if (fifo_src & FIFO_SRC_REG_EMPTY) {
        n_samples = 0;
    }
    while (n_samples > 0) {
        uint8_t n = MIN(n_samples, 1);
        if (!_dev_accel->read_registers(OUT_X_L_A | 0x40, _fifo_buffer, n * LSM9DS0_SAMPLE_SIZE)) {
            break;
        }
        const struct sensor_raw_data *raw_data = (const struct sensor_raw_data *)_fifo_buffer;
        for (uint8_t i=0; i<n; i++) {
            Vector3f accel_data(raw_data[i].x, -raw_data[i].y, -raw_data[i].z);

            accel_data *= _accel_scale;

            _rotate_and_correct_accel(_accel_instance, accel_data);
            _notify_new_accel_raw_sample(_accel_instance, accel_data);
        }
        n_samples -= n;

    }

#ifdef LSM9DS0_DEBUG
    static uint32_t last_print_ms;
    static uint32_t sample_count;
    static uint32_t call_count;
    call_count++;
    sample_count += (fifo_src & FIFO_SRC_REG_FSS_MASK);
    
    uint32_t now = AP_HAL::millis();
    if (now - last_print_ms > 1000) {
        last_print_ms = now;
        printf("A: call_count %u sample_count %u fifo_src 0x%02x\n",
               (unsigned)call_count, (unsigned)sample_count,
               (unsigned)fifo_src);
        call_count = 0;
        sample_count = 0;
    }
#endif
}

/*
 *  read from the data registers and update filtered data
 */
void AP_InertialSensor_LSM9DS0::_read_data_transaction_g()
{
    uint8_t fifo_src;
    
    // read fifo status register
    if (!_dev_gyro->read_registers(FIFO_SRC_REG_G, &fifo_src, 1)) {
        return;
    }

    uint8_t n_samples = (fifo_src & FIFO_SRC_REG_G_FSS_MASK);
    if (fifo_src & FIFO_SRC_REG_G_EMPTY) {
        n_samples = 0;
    }
    while (n_samples > 0) {
        uint8_t n = MIN(n_samples, LSM9DS0_MAX_FIFO_SAMPLES);
        if (!_dev_gyro->read_registers(OUT_X_L_G | 0x40, _fifo_buffer, n * LSM9DS0_SAMPLE_SIZE)) {
            break;
        }
        const struct sensor_raw_data *raw_data = (const struct sensor_raw_data *)_fifo_buffer;
        for (uint8_t i=0; i<n; i++) {
            Vector3f gyro_data(raw_data[i].x, -raw_data[i].y, -raw_data[i].z);
            gyro_data *= _gyro_scale;

            _rotate_and_correct_gyro(_gyro_instance, gyro_data);
            _notify_new_gyro_raw_sample(_gyro_instance, gyro_data);
        }
        n_samples -= n;
    }

#ifdef LSM9DS0_DEBUG
    static uint32_t last_print_ms;
    static uint32_t sample_count;
    static uint32_t call_count;

    sample_count += (fifo_src & FIFO_SRC_REG_FSS_MASK);
    call_count++;
        
    uint32_t now = AP_HAL::millis();
    if (now - last_print_ms > 1000) {
        last_print_ms = now;
        printf("G: call_count %u sample_count %u fifo_src 0x%02x\n",
               (unsigned)call_count, (unsigned)sample_count,
               (unsigned)fifo_src);
        call_count = 0;
        sample_count = 0;
    }
#endif
}

bool AP_InertialSensor_LSM9DS0::update()
{
    update_gyro(_gyro_instance);
    update_accel(_accel_instance);

    return true;
}

