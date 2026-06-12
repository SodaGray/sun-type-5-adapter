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
#include "sun_io.h"
#include "sun_protocol.h"
#include "sun_keymap.h"
#include "hid_keyboard.h"
#include "hid_extra.h"
#include "usb_mode.h"
#include "settings.h"
#include "storage.h"
#include "registry.h"
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
osThreadId_t sunKeyboardTaskHandle;
const osThreadAttr_t sunKeyboardTask_attributes = {
  .name = "sunKbd",
  .stack_size = 2048,
  .priority = osPriorityAboveNormal,
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
void StartSunKeyboardTask(void *argument);
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
  sunKeyboardTaskHandle = osThreadNew(StartSunKeyboardTask, NULL, &sunKeyboardTask_attributes);
  if (!sun_io_init(sunKeyboardTaskHandle)) {
    /* sun_io_init 失败 —— HAL_UART_Receive_IT 没起来
     * 这里可以 Error_Handler() 或 LED 闪烁告警
     * 目前先什么都不做，sunKeyboardTask 会一直 wait 没有通知 */
  }
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

  storage_mount();        /* 挂载 flash 对象存储（开机仅一次） */
  registry_init();        /* 把持久化设置读进 RAM 缓存 */
  usb_mode_init();        /* 据此设定当前模式 */
  tusb_init(0, &dev_init);

  USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBDEN;
  HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
  /* RTOS forever loop */
  for(;;)
  {
    tud_task();
  }
}

/**
 * @brief One-shot timer callback. Runs in the RTOS timer service task, NOT in
 * the keyboard task — so it only signals via a thread flag and touches
 * no shared state. The keyboard task does the actual mode change.
 *
 */
static void longpress_cb(void *arg)
{
  (void) arg;
  osThreadFlagsSet(sunKeyboardTaskHandle, NOTIFY_SETTINGS_ENTER);
}

static void settings_blink_error(void)
{
  bool sound = (registry()->feedback_sound != 0);
  for (int i = 0; i < 4; i++) {
    sun_io_set_raw_led(0x00); osDelay(50);
    if (sound) sun_io_bell_off();
    sun_io_set_raw_led(0x0F); osDelay(50);   /* 收在全亮，回到设置模式常态 */
    if (i != 3)  if (sound) sun_io_bell_on();
  }
}

/**
 * @brief Start the keyboard task
 *
 * Drains the ring buffer.
 */
void StartSunKeyboardTask(void *argument)
{
  (void)argument;
  sun_protocol_t parser;
  hid_keyboard_state_t kbd;
  bool settings_mode = false;
  bool sound = (registry()->feedback_sound != 0);
  osTimerId_t longpress_timer = osTimerNew(longpress_cb, osTimerOnce, NULL, NULL);

  sun_protocol_init(&parser);
  hid_keyboard_init(&kbd);

  for (;;) {
    uint32_t flags = osThreadFlagsWait(
        SUN_IO_NOTIFY_RX_AVAILABLE | SUN_IO_NOTIFY_LED_PENDING | SUN_IO_NOTIFY_RESET_PENDING
        | NOTIFY_SETTINGS_ENTER,
        osFlagsWaitAny,
        osWaitForever);

    if (flags & NOTIFY_SETTINGS_ENTER)
    {
      settings_mode = true;
      sun_io_set_raw_led(0x0F);   /* all four keyboard LEDs on = in settings mode */
    }

    if (flags & NOTIFY_SETTINGS_ENTER) {
      settings_mode = true;
      settings_enter();              /* 复位状态机到顶层 */
      sun_io_set_raw_led(0x0F);
    }

    if (flags & SUN_IO_NOTIFY_RX_AVAILABLE)
    {
      uint8_t byte;
      while (sun_io_get_byte(&byte)) {
        sun_event_t ev = sun_protocol_decode_byte(&parser, byte);
        switch (ev.type) {

        case SUN_EVENT_MAKE: {
            sun_key_t k = sun_keymap_lookup(ev.data);

            if (settings_mode) {
              switch (settings_key(k)) {
              case SETTINGS_CONTINUE:
                break;                              /* 留下，无反馈 */
              case SETTINGS_ERROR:
                settings_blink_error();             /* 闪三次，留下 */
                break;
              case SETTINGS_DONE:
                sun_io_set_raw_led(0x00);
                osDelay(250);
                sun_io_set_raw_led(0x0F);
                if (sound) sun_io_bell_on();
                osDelay(750);
                if (sound) sun_io_bell_off();
                settings_mode = false;
                sun_io_flush_led();                  /* 恢复主机 LED */
                break;
              case SETTINGS_CANCEL:
                settings_mode = false;
                sun_io_flush_led();
                break;
              }
              break;                                   /* 不落入正常按键分发 */
            }

            switch (k.kind) {
            case SUN_KEY_KIND_KEYBOARD:
              hid_keyboard_press_key(&kbd, (uint8_t)k.code);
              break;
            case SUN_KEY_KIND_MODIFIER:
              hid_keyboard_press_modifier(&kbd, (uint8_t)k.code);
              break;
            case SUN_KEY_KIND_CONSUMER:
              hid_keyboard_press_consumer(&kbd, (uint16_t)k.code);
              break;
            case SUN_KEY_KIND_SYSTEM:
              hid_keyboard_press_system(&kbd, (uint8_t)k.code);
              break;
            case SUN_KEY_KIND_INTERNAL:
              osTimerStart(longpress_timer, 500U);   /* arm long-press (~1.5s @ 1ms tick) */
              break;
            case SUN_KEY_KIND_NONE:
              break;
            }
            break;
        }

        case SUN_EVENT_BREAK: {
            sun_key_t k = sun_keymap_lookup(ev.data);

            if (settings_mode) {
              break;                          /* captured */
            }

            switch (k.kind) {
            case SUN_KEY_KIND_KEYBOARD:
              hid_keyboard_release_key(&kbd, (uint8_t)k.code);
              break;
            case SUN_KEY_KIND_MODIFIER:
              hid_keyboard_release_modifier(&kbd, (uint8_t)k.code);
              break;
            case SUN_KEY_KIND_CONSUMER:
              hid_keyboard_release_consumer(&kbd);
              break;
            case SUN_KEY_KIND_SYSTEM:
              hid_keyboard_release_system(&kbd, (uint8_t)k.code);
              break;
            case SUN_KEY_KIND_INTERNAL:
              osTimerStop(longpress_timer);   /* released within 1.5s = short press, cancel */
              break;
            case SUN_KEY_KIND_NONE:
              break;
            }
            break;
        }

        case SUN_EVENT_ALL_KEYS_UP:
          hid_keyboard_reset(&kbd);
          hid_keyboard_reset_consumer(&kbd);
          hid_keyboard_reset_system(&kbd);
          break;

        case SUN_EVENT_RESET:
          hid_keyboard_reset(&kbd);
          hid_keyboard_reset_consumer(&kbd);
          hid_keyboard_reset_system(&kbd);
          click_init();    /* 键盘复位会忘掉点击状态，重下发我们存的 */
          break;
        case SUN_EVENT_UNKNOWN:
        case SUN_EVENT_NONE:
          break;
        }
      }
    }

    if ((flags & SUN_IO_NOTIFY_LED_PENDING) && !settings_mode)
    {
      sun_io_flush_led();
    }

    if (flags & SUN_IO_NOTIFY_RESET_PENDING)
    {
      sun_io_flush_reset();
      hid_keyboard_reset(&kbd);
    }
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

void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  /* Don't turn off LD2 on suspend - device is still mounted, just idle.
   * Only umount should turn off LD2. */
}

void tud_resume_cb(void)
{
  (void) 0;  /* LD2 stays on - was already on from mount */
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

  if (report_type == HID_REPORT_TYPE_OUTPUT && bufsize >= 1) {
    sun_io_request_led(buffer[0]);
  }
}

/* USER CODE END Application */

