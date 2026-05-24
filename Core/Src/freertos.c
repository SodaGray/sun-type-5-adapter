/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tusb.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* USB device task: handles all USB events and drives TinyUSB stack */
osThreadId_t usbTaskHandle;
const osThreadAttr_t usbTask_attributes = {
  .name = "usbTask",
  .stack_size = 1024 * 4,                       /* 4 KB stack */
  .priority = (osPriority_t) osPriorityRealtime,/* Highest - USB has strict timing */
};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartUsbTask(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* creation of usbTask */
  usbTaskHandle = osThreadNew(StartUsbTask, NULL, &usbTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/*-----------------------------------------------------------*/
/* USB device task                                           */
/*-----------------------------------------------------------*/

/**
  * @brief  USB device task. Initializes TinyUSB, then processes events forever.
  *         Must run at highest priority. tud_task() blocks on internal
  *         semaphore until USB events arrive, so no vTaskDelay is needed.
  */
void StartUsbTask(void *argument)
{
  (void) argument;

  /* DIAGNOSTIC: blink LD2 3 times to prove task is scheduled */
  for (int i = 0; i < 3; i++) {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    osDelay(200);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    osDelay(200);
  }

  /* Initialize TinyUSB device stack on roothub port 0 */
  tusb_rhport_init_t dev_init = {
    .role  = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(0, &dev_init);

  USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBDEN;

  /* RTOS forever loop */
  for(;;)
  {
    tud_task();
  }
}

/*-----------------------------------------------------------*/
/* USB device state callbacks (optional, used as visual cue) */
/*-----------------------------------------------------------*/

/* Invoked when device is mounted (host enumeration complete) */
void tud_mount_cb(void)
{
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
}

/* Invoked when device is unmounted */
void tud_umount_cb(void)
{
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}

/* Invoked when USB bus is suspended (host puts device into low power) */
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}

/* Invoked when USB bus is resumed */
void tud_resume_cb(void)
{
  if (tud_mounted()) {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  }
}

/*-----------------------------------------------------------*/
/* HID class callbacks (required by TinyUSB - no weak defaults) */
/*-----------------------------------------------------------*/

/* Invoked when received GET_REPORT control request.
 * Application must fill buffer report's content and return its length.
 * Returning zero will cause the stack to STALL the request.
 * For a basic keyboard we don't support host-initiated report queries. */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen)
{
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;  /* STALL - not implemented */
}

/* Invoked when received SET_REPORT control request,
 * or received data on OUT endpoint (Report ID = 0, Type = 0).
 * For a keyboard, this is how host sends LED state (Caps/Num/Scroll Lock). */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;

  /* TODO: parse buffer[0] for keyboard LED bitmap and forward to
   * Sun keyboard via USART1 (Sun command 0x0E followed by LED bitmap). */
}

/* USER CODE END Application */

