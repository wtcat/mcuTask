/*
 * Copyright 2024 wtcat
 */

#define pr_fmt(fmt) "[w25q]: "
#define TX_USE_BOARD_PRIVATE

#include <errno.h>
#include "tx_user.h"
#include "basework/log.h"

#define QSPI_FLASH_FastReadQuad_IO 0xEB
#define QSPI_FLASH_ReadStatus_REG1 0X05
#define QSPI_FLASH_REG1_BUSY 0x01
#define QSPI_FLASH_EnableReset 0x66
#define QSPI_FLASH_ResetDevice 0x99
#define QSPI_FLASH_ID          0xef4017

static void stm32_qspi_pincfg(void) {
    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /*
    * PB2     ------> QUADSPI_CLK
    * PB10    ------> QUADSPI_BK1_NCS
    * PD11    ------> QUADSPI_BK1_IO0
    * PD12    ------> QUADSPI_BK1_IO1
    * PE2     ------> QUADSPI_BK1_IO2
    * PD13    ------> QUADSPI_BK1_IO3
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

static int stm32_qspi_init(QSPI_HandleTypeDef *qspi) {
	HAL_QSPI_DeInit(qspi);

	qspi->Init.ClockPrescaler = 1;
	qspi->Init.FifoThreshold = 32;
	qspi->Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
	qspi->Init.FlashSize = 22;
	qspi->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
	qspi->Init.ClockMode = QSPI_CLOCK_MODE_3;
	qspi->Init.FlashID = QSPI_FLASH_ID_1;
	qspi->Init.DualFlash = QSPI_DUALFLASH_DISABLE;
    stm32_qspi_pincfg();
	return (int)HAL_QSPI_Init(qspi);
}

static int stm32_qspi_autopolling(QSPI_HandleTypeDef *qspi) {
	QSPI_CommandTypeDef s_command;
	QSPI_AutoPollingTypeDef s_config;

	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.Instruction = QSPI_FLASH_ReadStatus_REG1;

	s_config.Match = 0;
	s_config.MatchMode = QSPI_MATCH_MODE_AND;
	s_config.Interval = 0x10;
	s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
	s_config.StatusBytesSize = 1;
	s_config.Mask = QSPI_FLASH_REG1_BUSY;

	if (HAL_QSPI_AutoPolling(qspi, &s_command, &s_config,
							 HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return -1;
	}
	return 0;
}

static int stm32_w25qxx_reset(QSPI_HandleTypeDef *qspi) {
	QSPI_CommandTypeDef s_command;

	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.Instruction = QSPI_FLASH_EnableReset;

	if (HAL_QSPI_Command(qspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return -1;

	if (stm32_qspi_autopolling(qspi))
		return -2;
	
	s_command.Instruction = QSPI_FLASH_ResetDevice;
	if (HAL_QSPI_Command(qspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return -3;

	if (stm32_qspi_autopolling(qspi))
		return -4;

	return 0;
}

static int stm32_w25qxx_mmap(QSPI_HandleTypeDef *qspi) {
	QSPI_CommandTypeDef s_command;
	QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;

	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.AddressSize = QSPI_ADDRESS_24_BITS;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	s_command.AddressMode = QSPI_ADDRESS_4_LINES;
	s_command.DataMode = QSPI_DATA_4_LINES;
	s_command.DummyCycles = 6;
	s_command.Instruction = QSPI_FLASH_FastReadQuad_IO;

	s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
	s_mem_mapped_cfg.TimeOutPeriod = 0;

	int err = stm32_w25qxx_reset(qspi);
    if (!err)
	    return HAL_QSPI_MemoryMapped(qspi, &s_command, &s_mem_mapped_cfg);
    return err;
}

static uint32_t stm32_w25qxx_readid(QSPI_HandleTypeDef *qspi) {
	QSPI_CommandTypeDef s_command;
	uint8_t	QSPI_ReceiveBuff[3];
	uint32_t W25Qxx_ID = 0;

	s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
	s_command.AddressMode		= QSPI_ADDRESS_NONE;
	s_command.DataMode			= QSPI_DATA_1_LINE;
	s_command.DummyCycles 		= 0;
	s_command.NbData 		    = 3;
	s_command.Instruction 		= 0x9F;

	if (HAL_QSPI_Command(qspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return -1;

	if (HAL_QSPI_Receive(qspi, QSPI_ReceiveBuff, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return -1;

	W25Qxx_ID = (QSPI_ReceiveBuff[0] << 16) | (QSPI_ReceiveBuff[1] << 8 ) | QSPI_ReceiveBuff[2];
	return W25Qxx_ID;
}

int stm32_w25qxx_init(void) {
    QSPI_HandleTypeDef qspi = {0};

    qspi.Instance = QUADSPI;
    stm32_qspi_init(&qspi);

	int err = stm32_w25qxx_reset(&qspi);
	if (err) {
		pr_err("failed to reset w25qxx\n");
		return err;
	}

	if (stm32_w25qxx_readid(&qspi) != QSPI_FLASH_ID) {
		pr_err("failed to read chip ID\n");
		return -EIO;
	}

    return stm32_w25qxx_mmap(&qspi);
}
