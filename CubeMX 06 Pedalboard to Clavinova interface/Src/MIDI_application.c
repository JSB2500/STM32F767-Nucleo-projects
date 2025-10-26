/*
 * MIDI_application.c
 *
 *  Created on: 6 déc. 2014
 *      Author: Xavier Halgand
 *
 *	Modified on: 9/12/16 by C.P. to handle the MIDI_IDLE state properly, and 
 *	added required code to be compatible with "NucleoSynth"
 *
 *	11/29/17 by C.P.: Version 0.8.0
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "MIDI_application.h"
#include "usbh_core.h"
#include "usbh_MIDI.h"
#include "usb_host.h"

MIDI_ApplicationTypeDef MIDI_Appli_state = MIDI_APPLICATION_READY;
uint8_t StopSetActive = 0;
uint8_t StopSetActive_StopAllNotes = 0;

extern ApplicationTypeDef Appli_state;
extern USBH_HandleTypeDef hUsbHostFS;

extern void SendToUSART(uint8_t);

extern uint8_t PedalNotePressed_C0;
extern uint8_t PedalNotePressed_D0;
extern uint8_t PedalNotePressed_E0;
extern uint8_t PedalNotePressed_G2;
//
extern uint8_t OrganStop_16;
extern uint8_t OrganStop_8;
extern uint8_t OrganStop_4;
extern uint8_t OrganStop_BeASustainingPedal;

/* Private define ------------------------------------------------------------*/

#define RX_BUFF_SIZE 64
static uint8_t MIDI_RX_Buffer[RX_BUFF_SIZE];

/* Private function prototypes -----------------------------------------------*/
void ProcessReceivedMidiDatas(void);

/*-----------------------------------------------------------------------------*/

void MIDI_Application(void)
{
	if(Appli_state == APPLICATION_READY)
	{
		if(MIDI_Appli_state == MIDI_APPLICATION_READY)
		{
			USBH_MIDI_Receive(&hUsbHostFS, MIDI_RX_Buffer, RX_BUFF_SIZE); // just once at the beginning, start the first reception
			MIDI_Appli_state = MIDI_APPLICATION_RUNNING;
		}
	}
	if(Appli_state == APPLICATION_DISCONNECT)
	{
		MIDI_Appli_state = MIDI_APPLICATION_READY;
		USBH_MIDI_Stop(&hUsbHostFS);
	}
}

void ProcessReceivedMidiDatas(void)
{
	uint16_t numberOfPackets;
	uint8_t *ptr = MIDI_RX_Buffer;
	midi_package_t pack;

	numberOfPackets = USBH_MIDI_GetLastReceivedDataSize(&hUsbHostFS) >> 2; //each USB midi package is 4 bytes long

	if (numberOfPackets != 0) 
	{
		while(numberOfPackets--)
		{
			pack.cin_cable = *ptr; ++ptr;
			pack.evnt0 = *ptr; ++ptr;
			pack.evnt1 = *ptr; ++ptr;
			pack.evnt2 = *ptr; ++ptr;
			
			// uint8_t Channel = pack.evnt0 & 0x0F;
			uint8_t Message = pack.evnt0 & 0xF0;
      uint8_t Pitch = pack.evnt1;
			uint8_t Velocity = pack.evnt2;

			if (Message && (Message != 0xF0)) // Exclude zero and blank system messages.
			{
			  printf("Received: Message=%d Pitch=%d Velocity=%d\n", Message, Pitch, Velocity);
			}
			
      if (Message == 0x90)
      {
        if (Pitch == 21) // Bottom A on piano.
        {
          if (Velocity)
          {
            StopSetActive = 1;
            StopSetActive_StopAllNotes = 1;
          }
          else
          {
            OrganStop_16 = PedalNotePressed_C0;
            OrganStop_8 = PedalNotePressed_D0;
            OrganStop_4 = PedalNotePressed_E0;
            OrganStop_BeASustainingPedal = PedalNotePressed_G2;

            printf("OrganStop_16: %d\n", OrganStop_16);
            printf("OrganStop_8: %d\n", OrganStop_8);
            printf("OrganStop_4: %d\n", OrganStop_4);
            printf("OrganStop_BeASustainingPedal: %d\n", OrganStop_BeASustainingPedal);

            StopSetActive = 0;
          }
        }
      }
		}
	}
}

/*-----------------------------------------------------------------------------*/
/**
 * @brief  MIDI data receive callback.
 * @param  phost: Host handle
 * @retval None
 */
void USBH_MIDI_ReceiveCallback(USBH_HandleTypeDef *phost)
{
	ProcessReceivedMidiDatas();
	USBH_MIDI_Receive(&hUsbHostFS, MIDI_RX_Buffer, RX_BUFF_SIZE); // start a new reception
}
