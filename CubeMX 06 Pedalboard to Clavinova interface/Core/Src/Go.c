#include "stdio.h"
#include "main.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"
#include "stm32f7xx_nucleo_144.h"
#include "usbh_core.h"
#include "usbh_conf.h"
#include "usbh_MIDI.h"
#include "MIDI_application.h"
#include "ApplicationConfiguration.h"
#include "Go.h"

// State of pedal notes:
uint8_t PedalNotePressed_C0 = 0;
uint8_t PedalNotePressed_D0 = 0;
uint8_t PedalNotePressed_E0 = 0;
uint8_t PedalNotePressed_G2 = 0;

// Organ stops:
uint8_t OrganStop_16 = 1;
uint8_t OrganStop_8 = 0;
uint8_t OrganStop_4 = 0;
uint8_t OrganStop_BeASustainingPedal = 0;
extern uint8_t StopSetActive;
extern uint8_t StopSetActive_StopAllNotes;

// Other:
extern USBH_HandleTypeDef hUsbHostFS;
extern HCD_HandleTypeDef hhcd_USB_OTG_FS;

///////////////////////////////////////////////////////////////////////////////
// PedalboardReceiver
#define PedalboardReceiver_InputBufferSize (128)
static int32_t PedalboardReceiver_InputBufferWritePosition = 0;
static int32_t PedalboardReceiver_InputBufferReadPosition = 0;
static uint8_t PedalboardReceiver_InputBuffer[PedalboardReceiver_InputBufferSize];
static uint8_t PedalboardReceiver_DisplayBufferOverrunMessage = 0;

int32_t PedalboardReceiver_GetNumBytesInBuffer()
{
  int32_t Result;

  Result = PedalboardReceiver_InputBufferWritePosition - PedalboardReceiver_InputBufferReadPosition;
  if (Result < 0)
    Result += PedalboardReceiver_InputBufferSize;

  return Result;
}

void PedalboardReceiver_InputBufferWritePosition_Advance(int32_t Value)
{
  PedalboardReceiver_InputBufferWritePosition += Value;

  if (PedalboardReceiver_InputBufferWritePosition >= PedalboardReceiver_InputBufferSize)
  {
    PedalboardReceiver_InputBufferWritePosition -= PedalboardReceiver_InputBufferSize;
  }
}

void PedalboardReceiver_InputBufferReadPosition_Advance(int32_t Value)
{
  PedalboardReceiver_InputBufferReadPosition += Value;

  if (PedalboardReceiver_InputBufferReadPosition >= PedalboardReceiver_InputBufferSize)
  {
    PedalboardReceiver_InputBufferReadPosition -= PedalboardReceiver_InputBufferSize;
  }
}

void PedalboardReceiver_ReceiveISR()
{
  uint32_t isrflags = READ_REG(huart7.Instance->ISR);

  if (isrflags & (uint32_t)USART_ISR_ORE)
  {
    __HAL_UART_CLEAR_FLAG(&huart7, UART_CLEAR_OREF);
    PedalboardReceiver_DisplayBufferOverrunMessage = 1;
    return;
  }

  if (isrflags & (uint32_t)USART_ISR_PE)
  {
    __HAL_UART_CLEAR_FLAG(&huart7, UART_CLEAR_PEF);
    // Error not reported!!!
    return;
  }

  if (isrflags & (uint32_t)USART_ISR_FE)
  {
    __HAL_UART_CLEAR_FLAG(&huart7, UART_CLEAR_FEF);
    // Error not reported!!!
    return;
  }

  if (isrflags & (uint32_t)USART_ISR_NE)
  {
    __HAL_UART_CLEAR_FLAG(&huart7, UART_CLEAR_NEF);
    // Error not reported!!!
    return;
  }

  uint32_t NumBytesInBuffer = PedalboardReceiver_GetNumBytesInBuffer();
  if (NumBytesInBuffer < PedalboardReceiver_InputBufferSize)
  {
    PedalboardReceiver_InputBuffer[PedalboardReceiver_InputBufferWritePosition] = (uint8_t)(huart7.Instance->RDR);
    PedalboardReceiver_InputBufferWritePosition_Advance(1);
  }
}

///////////////////////////////////////////////////////////////////////////////

typedef struct
{
  uint8_t Pitch;
  uint8_t Pressed;
} Note_t;

void SendDebugInfo()
{
  printf("Debug: Start\n");
  printf("hUsbHostFS.gState: %08X\n", (unsigned int)hUsbHostFS.gState);
  printf("hUsbHostFS.RequestState: %08X\n", (unsigned int)hUsbHostFS.RequestState);
  printf("hUsbHostFS.Timer: %u\n", (unsigned int)hUsbHostFS.Timer);
  printf("hUsbHostFS.id: %d\n", hUsbHostFS.id);

  if (hUsbHostFS.pActiveClass)
  {
    MIDI_HandleTypeDef *MIDI_Handle;
    MIDI_Handle = hUsbHostFS.pActiveClass->pData;

    printf("MIDI_Handle->data_rx_state: %08X\n", (unsigned int)MIDI_Handle->data_rx_state);
    printf("MIDI_Handle->data_tx_state: %08X\n", (unsigned int)MIDI_Handle->data_tx_state);
  }

  USB_OTG_GlobalTypeDef *pUSBGlobal = hhcd_USB_OTG_FS.Instance;

  printf("pUSBGlobal->GOTGCTL: %08X\n", (unsigned int)pUSBGlobal->GOTGCTL);
  printf("pUSBGlobal->GOTGINT: %08X\n", (unsigned int)pUSBGlobal->GOTGINT);
  printf("pUSBGlobal->GAHBCFG: %08X\n", (unsigned int)pUSBGlobal->GAHBCFG);
  printf("pUSBGlobal->GUSBCFG: %08X\n", (unsigned int)pUSBGlobal->GUSBCFG);
  printf("pUSBGlobal->GRSTCTL: %08X\n", (unsigned int)pUSBGlobal->GRSTCTL);
  printf("pUSBGlobal->GINTSTS: %08X\n", (unsigned int)pUSBGlobal->GINTSTS);
  printf("pUSBGlobal->GINTMSK: %08X\n", (unsigned int)pUSBGlobal->GINTMSK);
  printf("pUSBGlobal->GRXSTSR: %08X\n", (unsigned int)pUSBGlobal->GRXSTSR);
  // This breaks USB TX&RX! printf("pUSBGlobal->GRXSTSP: %08X\n", (unsigned int)pUSBGlobal->GRXSTSP);
  printf("pUSBGlobal->GRXFSIZ: %08X\n", (unsigned int)pUSBGlobal->GRXFSIZ);
  printf("pUSBGlobal->DIEPTXF0_HNPTXFSIZ: %08X\n", (unsigned int)pUSBGlobal->DIEPTXF0_HNPTXFSIZ);
  printf("pUSBGlobal->HNPTXSTS: %08X\n", (unsigned int)pUSBGlobal->HNPTXSTS);
  printf("pUSBGlobal->GCCFG: %08X\n", (unsigned int)pUSBGlobal->GCCFG);
  printf("pUSBGlobal->CID: %08X\n", (unsigned int)pUSBGlobal->CID);
  printf("pUSBGlobal->GHWCFG3: %08X\n", (unsigned int)pUSBGlobal->GHWCFG3);
  printf("pUSBGlobal->GLPMCFG: %08X\n", (unsigned int)pUSBGlobal->GLPMCFG);
  printf("pUSBGlobal->GDFIFOCFG: %08X\n", (unsigned int)pUSBGlobal->GDFIFOCFG);
  printf("pUSBGlobal->HPTXFSIZ: %08X\n", (unsigned int)pUSBGlobal->HPTXFSIZ);
  for (int TransmitFIFOSizeRegisterIndex = 0; TransmitFIFOSizeRegisterIndex < 8; ++TransmitFIFOSizeRegisterIndex)
  {
    printf("pUSBGlobal->DIEPTXF[%d]: %08X\n", TransmitFIFOSizeRegisterIndex, (unsigned int)pUSBGlobal->DIEPTXF[TransmitFIFOSizeRegisterIndex]);
  }

  uint32_t USBx_BASE = (uint32_t)(hhcd_USB_OTG_FS.Instance);
  //
  printf("USBx_HOST->HCFG: %08X\n", (unsigned int)USBx_HOST->HCFG);
  printf("USBx_HOST->HFIR: %08X\n", (unsigned int)USBx_HOST->HFIR);
  printf("USBx_HOST->HFNUM: %08X\n", (unsigned int)USBx_HOST->HFNUM);
  printf("USBx_HOST->HPTXSTS: %08X\n", (unsigned int)USBx_HOST->HPTXSTS);
  printf("USBx_HOST->HAINT: %08X\n", (unsigned int)USBx_HOST->HAINT);
  printf("USBx_HOST->HAINTMSK: %08X\n", (unsigned int)USBx_HOST->HAINTMSK);
  printf("HPRT: %08X\n", (unsigned int)USBx_HPRT0);
  for (int ChannelIndex = 0; ChannelIndex < 12; ++ChannelIndex)
  {
    USB_OTG_HostChannelTypeDef *pChannel = USBx_HC(ChannelIndex);
    printf("Channel: %d\n", ChannelIndex);
    printf("HCCHAR: %08X\n", (unsigned int)pChannel->HCCHAR);
    printf("HCDMA: %08X\n", (unsigned int)pChannel->HCDMA);
    printf("HCINT: %08X\n", (unsigned int)pChannel->HCINT);
    printf("HCINTMSK: %08X\n", (unsigned int)pChannel->HCINTMSK);
    printf("HCSPLT: %08X\n", (unsigned int)pChannel->HCSPLT);
    printf("HCTSIZ: %08X\n", (unsigned int)pChannel->HCTSIZ);
  }

  printf("Debug: End\n");
}

static uint8_t MIDI_TX_Buffer[64];

///////////////////////////////////////////////////////////////////////////////

static bool IsUsbActive()
{
  return hUsbHostFS.pActiveClass;
}

static bool IsUsbIdle()
{
  if (hUsbHostFS.pActiveClass)
  {
    MIDI_HandleTypeDef *MIDI_Handle;

    MIDI_Handle = hUsbHostFS.pActiveClass->pData;
    if (MIDI_Handle && (MIDI_Handle->data_tx_state == MIDI_IDLE))
      return true;
  }

  return false;
}

void Go()
{
  uint8_t UserButtonPressed = BSP_PB_GetState(BUTTON_USER);

  // Try to connect to USBdevice.
  bool UseUSB = false;
  uint32_t Tick_Start = HAL_GetTick();
  {
    while (HAL_GetTick() - Tick_Start < 1000U)
      MX_USB_HOST_Process();

    UseUSB = IsUsbActive();
  }

  while (1)
  {
    if (UseUSB)
    {
      MX_USB_HOST_Process();
      MIDI_Application();
    }

    if (BSP_PB_GetState(BUTTON_USER))
    {
      if (!UserButtonPressed)
      {
        UserButtonPressed = 1;

        SendDebugInfo();
      }
    }
    else
      UserButtonPressed = 0;

    if (PedalboardReceiver_DisplayBufferOverrunMessage)
    {
      PedalboardReceiver_DisplayBufferOverrunMessage = 0;
      printf("Pedalboard receiver: Buffer overrun\n");
    }

    if (!UseUSB || IsUsbIdle())
    {
      uint8_t MIDI_TX_BufferIndex = 0;

      if (StopSetActive)
      {
        if (StopSetActive_StopAllNotes)
        {
          if (MCU_to_Piano_Interface == mpiUSB)
            MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 9; // Cable?
          MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0xB0;
          MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0x7B; // All notes off.
          MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0;

          if (MCU_to_Piano_Interface == mpiUSB)
            MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 9; // Cable?
          MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0xB0;
          MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0x40; // Sustain.
          MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0; // Off

          printf("All notes off.\n");

          StopSetActive_StopAllNotes = 0;
        }
      }
      else
      {
        uint32_t PedalboardReceiver_NumBytesInBuffer = PedalboardReceiver_GetNumBytesInBuffer();
        if (PedalboardReceiver_NumBytesInBuffer >= 3)
        {
          uint8_t FirstByte = PedalboardReceiver_InputBuffer[PedalboardReceiver_InputBufferReadPosition];
          FirstByte &= 0xF0; // Zero channel info. Added 18/2/2022! Reason: Pedalboard controller started sending 0x91 for pedal-down (due to dip switch being flipped inadvertently).
          if ((FirstByte != 0x80) && (FirstByte != 0x90))
          {
            PedalboardReceiver_InputBufferReadPosition_Advance(1); // Try to resync.
          }
          else
          {
            PedalboardReceiver_InputBufferReadPosition_Advance(1);
            uint8_t SecondByte = PedalboardReceiver_InputBuffer[PedalboardReceiver_InputBufferReadPosition];
            PedalboardReceiver_InputBufferReadPosition_Advance(2); // Skip velocity byte.

            Note_t PedalNote;
            PedalNote.Pitch = SecondByte;
            PedalNote.Pressed = FirstByte == 0x90 ? 1 : 0;

            printf("Pedal note: Pitch=%d Pressed=%d\n", PedalNote.Pitch, PedalNote.Pressed);

            if (PedalNote.Pressed)
              BSP_LED_On(LED_GREEN);
            else
              BSP_LED_Off(LED_GREEN);

            switch (PedalNote.Pitch)
            {
              case 24: // C0
                PedalNotePressed_C0 = PedalNote.Pressed;
                break;
              case 26: // D0
                PedalNotePressed_D0 = PedalNote.Pressed;
                break;
              case 28: // E0
                PedalNotePressed_E0 = PedalNote.Pressed;
                break;
              case 55: // G2
                PedalNotePressed_G2 = PedalNote.Pressed;
                break;
            }

            if (!StopSetActive)
            {
              uint8_t DisplaySustainPedalInfo = 0;

              if (OrganStop_BeASustainingPedal)
              {
                if (MCU_to_Piano_Interface == mpiUSB)
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 9; // Cable?
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0xB0;
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0x40; // Sustain.
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 127 * PedalNote.Pressed;
                DisplaySustainPedalInfo = 1;
              }

              if (OrganStop_16)
              {
                if (MCU_to_Piano_Interface == mpiUSB)
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 9; // Cable?
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0x90; // Keydown.
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = PedalNote.Pitch;
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 127 * PedalNote.Pressed;
              }

              if (OrganStop_8)
              {
                if (MCU_to_Piano_Interface == mpiUSB)
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 9; // Cable?
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0x90; // Keydown.
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = PedalNote.Pitch + 12;
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 127 * PedalNote.Pressed;
              }

              if (OrganStop_4)
              {
                if (MCU_to_Piano_Interface == mpiUSB)
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 9; // Cable?
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 0x90; // Keydown.
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = PedalNote.Pitch + 24;
                MIDI_TX_Buffer[MIDI_TX_BufferIndex++] = 127 * PedalNote.Pressed;
              }

              if (DisplaySustainPedalInfo)
              {
                if (PedalNote.Pressed)
                {
                  printf("Sustain pedal pressed\n");
                }
                else
                {
                  printf("Sustain pedal released\n");
                }
              }
            }
          }
        }

        if (MIDI_TX_BufferIndex != 0)
        {
          switch(MCU_to_Piano_Interface)
          {
            case mpiSerial:
              HAL_UART_Transmit(&huart6, MIDI_TX_Buffer, MIDI_TX_BufferIndex, 1000);
              break;

            case mpiUSB:
              USBH_MIDI_Transmit(&hUsbHostFS, MIDI_TX_Buffer, MIDI_TX_BufferIndex);
              break;

            default:
              Error_Handler();
          }
        }
      }
    }
  }
}
