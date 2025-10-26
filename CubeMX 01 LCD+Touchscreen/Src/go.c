#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "main.h"
#include "stm32f7xx_hal.h"
#include "JSB_ILI9341.h"
#include "JSB_XPT2046.h"
#include "go.h"
#include "gfxfont.h"
#include "FreeSans9pt7b.h"
#include "FreeSans12pt7b.h"

#define Bar_X 60
#define Bar_MaxLength 180
#define Bar_Height 28
#define Bar_Min_dB -80

#define Channel_Separation 4

#define Button_XPosition 40
#define Button_YPosition 290
#define Button_Width 160
#define Button_Height 30

void DrawChannel(char *ChannelName, uint16_t Y, float Level_Linear)
{
  char S[128];
  uint16_t BarLength, BarEnd, TextY;
  float Level_dB;

  ILI9341_SetFont(&FreeSans12pt7b);

  if (Level_Linear > 0)
    Level_dB = 20.0f * log10(Level_Linear);
  else
    Level_dB = -200.0f;

  TextY = Y + ILI9341_GetFontYSpacing() - 8;

  sprintf(S, "%s:", ChannelName);
  ILI9341_DrawTextAtXY(S, 0, TextY, tpLeft);

  BarLength = round((1.0f - (Level_dB / Bar_Min_dB)) * Bar_MaxLength);
  BarEnd = Bar_X + BarLength;
  ILI9341_DrawBar(Bar_X, Y, BarLength, Bar_Height, ILI9341_COLOR_DARKGREEN);
  ILI9341_DrawBar(BarEnd, Y, Bar_MaxLength - BarLength, Bar_Height, ILI9341_COLOR_DARKERGREY);

  sprintf(S, "%1.2f dB", Level_dB);
  TextDrawMode_t TextDrawMode = ILI9341_SetTextDrawMode(tdmMergeWithExistingPixels);
  ILI9341_DrawTextAtXY(S, (Bar_X + Bar_MaxLength) / 2, TextY, tpCentre);
  ILI9341_SetTextDrawMode(TextDrawMode);
}

void DrawClearScreenButton(uint16_t Y)
{
  ILI9341_SetFont(&FreeSans12pt7b);
  ILI9341_DrawBar(Button_XPosition, Y, Button_Width, Button_Height, ILI9341_COLOR_MAROON);
  uint16_t TextBackgroundColor = ILI9341_SetTextBackgroundColor(ILI9341_COLOR_MAROON);
  ILI9341_DrawTextAtXY("Clear screen", Button_XPosition + Button_Width / 2, Y + 23, tpCentre);
  ILI9341_SetTextBackgroundColor(TextBackgroundColor);
}

uint8_t IsInClearScreenButton(uint16_t X, uint16_t Y)
{
  if ((X < Button_XPosition) || (X >= Button_XPosition + Button_Width))
    return 0;
  if ((Y < Button_YPosition) || (Y >= Button_YPosition + Button_Height))
    return 0;

  return 1;
}

void Go()
{
  float Level_X, Level_Y;
  uint8_t ButtonPressed = 0;

  ILI9341_SetFont(&FreeSans12pt7b);
  ILI9341_SetTextColor(ILI9341_COLOR_WHITE);
  ILI9341_SetTextBackgroundColor(ILI9341_COLOR_BLACK);
  ILI9341_SetTextDrawMode(tdmAnyCharBar); // Slower but enables flicker free update.

  ILI9341_Clear(0x0000);
  DrawClearScreenButton(Button_YPosition);

  while (1)
  {
    Level_X = (float) random() / RAND_MAX;
    Level_Y = (float) random() / RAND_MAX;

    char S[128];
    int16_t Touch_RawX, Touch_RawY, Touch_RawZ;
    int16_t Touch_X, Touch_Y;
    uint8_t Touched = XPT2046_Sample(&Touch_RawX, &Touch_RawY, &Touch_RawZ);

    XPT2046_ConvertRawToScreen(Touch_RawX, Touch_RawY, &Touch_X, &Touch_Y);

    DrawChannel("Left", 0, Level_X);
    DrawChannel("Right", ILI9341_GetFontYSpacing() + Channel_Separation, Level_Y);

    ILI9341_SetFont(&FreeSans9pt7b);

    sprintf(S, "Touched: %s   ", Touched ? "Yes" : "No");
    ILI9341_DrawTextAtXY(S, 0, 105, tpLeft);
    //
    sprintf(S, "X: %d      ", Touch_RawX);
    ILI9341_DrawTextAtXY(S, 0, 125, tpLeft);
    //
    sprintf(S, "Y: %d      ", Touch_RawY);
    ILI9341_DrawTextAtXY(S, 0, 145, tpLeft);
    //
    sprintf(S, "Z: %d      ", Touch_RawZ);
    ILI9341_DrawTextAtXY(S, 0, 165, tpLeft);

    if (Touched)
    {
      if (IsInClearScreenButton(Touch_X, Touch_Y))
      {
        if (!ButtonPressed)
        {
          ILI9341_Clear(0x0000);
          DrawClearScreenButton(Button_YPosition);
          ButtonPressed = 1;
        }
      }
      else
        ILI9341_DrawPixel(Touch_X, Touch_Y, ILI9341_COLOR_WHITE, 1);
    }

    if (!Touched)
      ButtonPressed = 0;
  }
}
